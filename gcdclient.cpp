#include "gcdclient.h"
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptValueIterator>

const QString GCDClient::KeyStoreFilePath = "C:/GCDClient.dat";
const QString GCDClient::consumerKey = "196573379494.apps.googleusercontent.com";
const QString GCDClient::consumerSecret = "il59cyz3dwBW6tsHBkZYGSWj";
const QString GCDClient::authorizationScope = "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile https://www.googleapis.com/auth/drive.file";

const QString GCDClient::authorizeURI = "https://accounts.google.com/o/oauth2/auth";
const QString GCDClient::accessTokenURI = "https://accounts.google.com/o/oauth2/token";
const QString GCDClient::accountInfoURI = "https://www.googleapis.com/oauth2/v1/userinfo";

const QString GCDClient::metadataURI = "https://www.googleapis.com/drive/v1/files/%1";
const QString GCDClient::insertURI = "https://www.googleapis.com/drive/v1/files";
const QString GCDClient::patchMetadataURI = "https://www.googleapis.com/drive/v1/files/%1";
const QString GCDClient::updateMetadataURI = "https://www.googleapis.com/drive/v1/files/%1";

GCDClient::GCDClient(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    loadParamMap();

    // Populate contentTypeHash.
    m_contentTypeHash["jpg"] = "image/jpeg";
    m_contentTypeHash["png"] = "image/png";
    m_contentTypeHash["pdf"] = "application/pdf";
    m_contentTypeHash["txt"] = "text/plain";
}

GCDClient::~GCDClient()
{
    saveParamMap();
}

void GCDClient::loadParamMap() {
    QFile file(KeyStoreFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> m_paramMap;

        qDebug() << "GCDClient::loadParamMap " << m_paramMap;
    }
}

void GCDClient::saveParamMap() {
    QFile file(KeyStoreFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << m_paramMap;

//        qDebug() << "GCDClient::saveParamMap " << m_paramMap;
    }
}

QString GCDClient::createTimestamp() {
    qint64 seconds = QDateTime::currentMSecsSinceEpoch() / 1000;

    return QString("%1").arg(seconds);
}

QString GCDClient::createNonce() {
    QString ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString nonce;

    for(int i = 0; i <= 16; ++i)
    {
        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
    }

    return nonce;
}

QString GCDClient::createQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(key).append("=").append(sortMap[key]);
    }

    return queryString;
}

QStringList GCDClient::getStoredUidList()
{
    QStringList list;
    foreach (QString s, accessTokenPairMap.keys()) {
        list.append(s);
    }
    return list;
}

bool GCDClient::isAuthorized()
{
    return (m_paramMap.contains("access_token") &&  m_paramMap.contains("refresh_token"));
}

QMap<QString, QString> GCDClient::createMapFromJson(QString jsonText)
{
    qDebug() << "GCDClient::createMapFromJson " << jsonText;

    QMap<QString, QString> map;
    QScriptEngine engine;
    QScriptValue sc;
    sc = engine.evaluate("(" + jsonText + ")");

    QScriptValueIterator it(sc);
    while (it.hasNext()) {
        it.next();
        map[it.name()] = it.value().toString();
    }

    qDebug() << "GCDClient::createMapFromJson " << map;

    return map;
}

QHash<QString, QString> GCDClient::createHashFromScriptValue(QString parentName, QScriptValue sc) {
    QHash<QString, QString> hash;
    QScriptValueIterator it(sc);
    while (it.hasNext()) {
        it.next();

        // key = parentName.name...
        QString key = ((parentName == "") ? "" : (parentName + ".")) + it.name();

        if(it.value().isArray()) {
//            qDebug() << "key " << key << " isArray";
            QStringList items;
            qScriptValueToSequence(it.value(), items);
            for (int i=0; i<items.length(); i++) {
                QScriptValue v = it.value().property(QString("%1").arg(i));
                hash.unite(createHashFromScriptValue(QString("%1.%2").arg(key).arg(i) , v));
            }
        } else if (it.value().isObject() ) {
//            qDebug() << "key " << key << " isObject";
            hash.unite(createHashFromScriptValue(key, it.value()));
        } else {
            hash[key] = it.value().toString();
        }
    }

//    qDebug() << "GCDClient::createHashFromScriptValue parentName " << parentName << " hash " << hash;

    return hash;
}

QHash<QString, QString> GCDClient::createHashFromJson(QString jsonText)
{
//    qDebug() << "GCDClient::createHashFromJson " << jsonText;

    QHash<QString, QString> hash;
    QScriptEngine engine;
    QScriptValue sc;
    sc = engine.evaluate("(" + jsonText + ")");

    qDebug() << "sc " << sc.toString();

    hash = createHashFromScriptValue("", sc);

//    qDebug() << "GCDClient::createHashFromJson " << hash;

    return hash;
}

QByteArray GCDClient::encodeMultiPart(QString boundary, QMap<QString, QString> paramMap, QString fileParameter, QString fileName, QByteArray fileData, QString contentType) {
    //Encodes list of parameters and files for HTTP multipart format.
    QByteArray postData;
    QString CRLF = "\r\n";

    foreach (QString key, paramMap.keys()) {
        postData.append("--" + boundary).append(CRLF);
        postData.append(QString("Content-Disposition: form-data; name=\"%1\"").arg(key)).append(CRLF);
        postData.append(CRLF);
        postData.append(paramMap[key]).append(CRLF);
    }

    postData.append("--" + boundary).append(CRLF);
    postData.append(QString("Content-Disposition: form-data; name=\"%1\"; filename=\"%2\"").arg(fileParameter).arg(fileName) ).append(CRLF);
    postData.append(QString("Content-Type: %1").arg(contentType) ).append(CRLF);
    postData.append(CRLF);
    postData.append(fileData).append(CRLF);
    postData.append("--" + boundary + "--").append(CRLF);
    postData.append(CRLF);

    return postData;
}

void GCDClient::authorize()
{
    qDebug() << "----- GCDClient::authorize -----";

    QMap<QString, QString> sortMap;
    sortMap["response_type"] = "code";
    sortMap["client_id"] = consumerKey;
    sortMap["redirect_uri"] = "urn:ietf:wg:oauth:2.0:oob";
    sortMap["scope"] = "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile https://www.googleapis.com/auth/cloudprint";
    sortMap["state"] = createNonce();
    QString queryString = createQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(authorizeURI + "?" + queryString);
}

bool GCDClient::parseAuthorizationCode(QString text)
{
    qDebug() << "----- GCDClient::parseAuthorizationCode -----";

    QStringList texts = text.split(' ');
    if (texts.length() == 2 && texts.at(0) == "Success") {
        foreach (QString s, texts[1].split('&')) {
            QStringList c = s.split('=');
            m_paramMap[c.at(0)] = c.at(1);
        }
        return true;
    } else {
        return false;
    }
}

void GCDClient::accessToken()
{
    qDebug() << "----- GCDClient::accessToken -----";

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["code"] = m_paramMap["code"];
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["redirect_uri"] = "urn:ietf:wg:oauth:2.0:oob";
    sortMap["grant_type"] = "authorization_code";
    QString queryString = createQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCDClient::refreshAccessToken()
{
    qDebug() << "----- GCDClient::refreshAccessToken -----";

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["refresh_token"] = m_paramMap["refresh_token"];
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["grant_type"] = "refresh_token";
    QString queryString = createQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(refreshAccessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCDClient::accountInfo()
{
    qDebug() << "----- GCDClient::accountInfo -----";

    QString uri = accountInfoURI;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setRawHeader("Authorization", QByteArray().append("Bearer ").append(m_paramMap["access_token"]));
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

QString GCDClient::getContentType(QString fileName) {
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    rx.indexIn(fileName);
    qDebug() << "GCDClient::getContentType fileName=" << fileName << " rx.captureCount()=" << rx.captureCount();
    for(int i=0; i<=rx.captureCount(); i++) {
        qDebug() << "i=" << i << " rx.cap=" << rx.cap(i);
    }
    QString fileExt = rx.cap(3).toLower();

    return m_contentTypeHash[fileExt];
}

void GCDClient::metadata()
{
    qDebug() << "----- GCDClient::accountInfo -----";

    QString uri = accountInfoURI;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setRawHeader("Authorization", QByteArray().append("Bearer ").append(m_paramMap["access_token"]));
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCDClient::fileGet()
{
    qDebug() << "----- GCDClient::accountInfo -----";

    QString uri = accountInfoURI;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setRawHeader("Authorization", QByteArray().append("Bearer ").append(m_paramMap["access_token"]));
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCDClient::filePut()
{
    qDebug() << "----- GCDClient::accountInfo -----";

    QString uri = accountInfoURI;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setRawHeader("Authorization", QByteArray().append("Bearer ").append(m_paramMap["access_token"]));
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCDClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::accessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QMap<QString, QString> map = createMapFromJson(replyBody);
        m_paramMap["access_token"] = map["access_token"];
        m_paramMap["refresh_token"] = map["refresh_token"];
    }

    emit accessTokenReplySignal(reply->error(), reply->errorString(), replyBody );
}

void GCDClient::refreshAccessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::refreshAccessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QMap<QString, QString> map = createMapFromJson(replyBody);
        m_paramMap["access_token"] = map["access_token"];
    }

    emit refreshAccessTokenReplySignal(reply->error(), reply->errorString(), replyBody );
}

void GCDClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::accountInfoReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        m_paramMap.unite(createMapFromJson(replyBody));
    }

    emit accountInfoReplySignal(reply->error(), reply->errorString(), replyBody );
}

void GCDClient::metadataReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::searchReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    // TODO Collect printers into caches for further submitting.
    if (reply->error() == QNetworkReply::NoError) {
        emit metadataReplySignal(reply->error(), reply->errorString(), "" );
    } else {
        emit metadataReplySignal(reply->error(), reply->errorString(), replyBody );
    }
}

void GCDClient::fileGetReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::fileGetReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    // TODO Collect printers into caches for further submitting.
    if (reply->error() == QNetworkReply::NoError) {
        emit fileGetReplySignal(reply->error(), reply->errorString(), "" );
    } else {
        emit fileGetReplySignal(reply->error(), reply->errorString(), replyBody );
    }
}

void GCDClient::filePutReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::filePutReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    // TODO Collect printers into caches for further submitting.
    if (reply->error() == QNetworkReply::NoError) {
        emit filePutReplySignal(reply->error(), reply->errorString(), "" );
    } else {
        emit filePutReplySignal(reply->error(), reply->errorString(), replyBody );
    }
}
