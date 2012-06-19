#include "gcpclient.h"
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QScriptEngine>
#include <QScriptValue>
#include <QScriptValueIterator>

const QString GCPClient::KeyStoreFilePath = "C:/GCPClient.dat";
const QString GCPClient::consumerKey = "196573379494.apps.googleusercontent.com";
const QString GCPClient::consumerSecret = "il59cyz3dwBW6tsHBkZYGSWj";
const QString GCPClient::authorizationScope = "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile https://www.googleapis.com/auth/cloudprint";

const QString GCPClient::authorizeURI = "https://accounts.google.com/o/oauth2/auth";
const QString GCPClient::accessTokenURI = "https://accounts.google.com/o/oauth2/token";
const QString GCPClient::accountInfoURI = "https://www.googleapis.com/oauth2/v1/userinfo";

const QString GCPClient::submitURI = "http://www.google.com/cloudprint/submit";
const QString GCPClient::jobsURI = "http://www.google.com/cloudprint/jobs";
const QString GCPClient::deletejobURI = "http://www.google.com/cloudprint/deletejob";
const QString GCPClient::printerURI = "http://www.google.com/cloudprint/printer";
const QString GCPClient::searchURI = "http://www.google.com/cloudprint/search";

GCPClient::GCPClient(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    loadParamMap();

    // Populate contentTypeHash.
    m_contentTypeHash["jpg"] = "image/jpeg";
    m_contentTypeHash["png"] = "image/png";
    m_contentTypeHash["pdf"] = "application/pdf";
    m_contentTypeHash["txt"] = "text/plain";
}

GCPClient::~GCPClient()
{
    saveParamMap();
}

void GCPClient::loadParamMap() {
    QFile file(KeyStoreFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> m_paramMap;

        qDebug() << QTime::currentTime() << "GCPClient::loadParamMap " << m_paramMap;
    }
}

void GCPClient::saveParamMap() {
    if (m_paramMap.isEmpty()) return;

    QFile file(KeyStoreFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << m_paramMap;

        qDebug() << "GCPClient::saveParamMap " << m_paramMap;
    }
}

QString GCPClient::createTimestamp() {
    qint64 seconds = QDateTime::currentMSecsSinceEpoch() / 1000;

    return QString("%1").arg(seconds);
}

QString GCPClient::createNonce() {
    QString ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString nonce;

    for(int i = 0; i <= 16; ++i)
    {
        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
    }

    return nonce;
}

QString GCPClient::createQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(key).append("=").append(sortMap[key]);
    }

    return queryString;
}

QStringList GCPClient::getStoredUidList()
{
    QStringList list;
    foreach (QString s, accessTokenPairMap.keys()) {
        list.append(s);
    }
    return list;
}

bool GCPClient::isAuthorized()
{
    return (m_paramMap.contains("access_token") &&  m_paramMap.contains("refresh_token"));
}

QMap<QString, QString> GCPClient::createMapFromJson(QString jsonText)
{
    qDebug() << "GCPClient::createMapFromJson " << jsonText;

    QMap<QString, QString> map;
    QScriptEngine engine;
    QScriptValue sc;
    sc = engine.evaluate("(" + jsonText + ")");

    QScriptValueIterator it(sc);
    while (it.hasNext()) {
        it.next();
        map[it.name()] = it.value().toString();
    }

    qDebug() << "GCPClient::createMapFromJson " << map;

    return map;
}

QHash<QString, QString> GCPClient::createHashFromScriptValue(QString parentName, QScriptValue sc) {
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

//    qDebug() << "GCPClient::createHashFromScriptValue parentName " << parentName << " hash " << hash;

    return hash;
}

QHash<QString, QString> GCPClient::createHashFromJson(QString jsonText)
{
//    qDebug() << "GCPClient::createHashFromJson " << jsonText;

    QHash<QString, QString> hash;
    QScriptEngine engine;
    QScriptValue sc;
    sc = engine.evaluate("(" + jsonText + ")");

    qDebug() << "sc " << sc.toString();

    hash = createHashFromScriptValue("", sc);

//    qDebug() << "GCPClient::createHashFromJson " << hash;

    return hash;
}

QStringList GCPClient::getStoredPrinterList()
{
    QMap<int, QString> map;
    QRegExp rx("^(printers\\.)(\\d+)(\\.id)$");

    foreach(QString key, m_printerHash.keys()) {
        if (rx.indexIn(key) != -1) {
            int i = rx.cap(2).toInt();
            QString printerName = rx.cap(1) + rx.cap(2) + ".displayName";
            map[i] = m_printerHash[printerName];
        }
    }

    qDebug() << "GCPClient::getStoredPrinterList map=" << map;

    return map.values();
}

QString GCPClient::getPrinterId(int i) {
    return m_printerHash[QString("printers.%1.id").arg(i)];
}

QByteArray GCPClient::encodeMultiPart(QString boundary, QMap<QString, QString> paramMap, QString fileParameter, QString fileName, QByteArray fileData, QString contentType) {
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

void GCPClient::authorize()
{
    qDebug() << "----- GCPClient::authorize -----";

    QMap<QString, QString> sortMap;
    sortMap["response_type"] = "code";
    sortMap["client_id"] = consumerKey;
    sortMap["redirect_uri"] = "urn:ietf:wg:oauth:2.0:oob";
    sortMap["scope"] = authorizationScope;
    sortMap["state"] = createNonce();
    QString queryString = createQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(authorizeURI + "?" + queryString);
}

bool GCPClient::parseAuthorizationCode(QString text)
{
    qDebug() << "----- GCPClient::parseAuthorizationCode -----";

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

void GCPClient::accessToken()
{
    qDebug() << "----- GCPClient::accessToken -----";

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

void GCPClient::refreshAccessToken()
{
    qDebug() << "----- GCPClient::refreshAccessToken -----";

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

void GCPClient::accountInfo()
{
    qDebug() << "----- GCPClient::accountInfo -----";

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

void GCPClient::search(QString q)
{
    qDebug() << "----- GCPClient::search -----";

    QString uri = searchURI + "?q=" + q;

    qDebug() << "GCPClient::search uri " << uri;

    QByteArray authHeader;
    authHeader.append("Bearer ");  // Works!  I'd use this header as it's OAuth2.
//    authHeader.append("OAuth ");  // Works!
//    authHeader.append("GoogleLogin auth=");  // Works!
    authHeader.append(m_paramMap["access_token"]);
    qDebug() << "GCPClient::search authHeader " << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(searchReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setRawHeader("Authorization", authHeader) ;
    req.setRawHeader("X-CloudPrint-Proxy", "Chrome");
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCPClient::submit(QString printerId, QString title, QString capabilities, QString contentPath, QString contentType, QString tag)
{
    qDebug() << "----- GCPClient::submit -----";

    QString uri = submitURI;

    QByteArray authHeader;
    authHeader.append("Bearer ");
    authHeader.append(m_paramMap["access_token"]);

    // Requires to submit job with multipart/form-data.
    QString boundary = "----------" + createNonce();
    QByteArray postData;
    QMap<QString, QString> paramMap;
    paramMap["printerid"] = printerId;
    paramMap["capabilities"] = capabilities;
    paramMap["contentType"] = contentType;
    paramMap["title"] = title;
    paramMap["tag"] = tag;
    if (contentType != "url") {
        QFile file(contentPath);
        if (file.open(QIODevice::ReadOnly)) {
            postData = encodeMultiPart(boundary, paramMap, "content", file.fileName(), file.readAll(), contentType);
        }
    } else {
        postData = encodeMultiPart(boundary, paramMap, "content", contentPath, QByteArray().append(contentPath), contentType);
    }

//    qDebug() << "postData " << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(submitReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setRawHeader("Authorization", authHeader) ;
    req.setRawHeader("X-CloudPrint-Proxy", "Chrome");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data;boundary=" + boundary);
    req.setHeader(QNetworkRequest::ContentLengthHeader, postData.length());
    QNetworkReply *reply = manager->post(req, postData);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCPClient::submit(QString printerId, QString contentPath)
{
    QFile file(contentPath);
    QFileInfo fileInfo(file);
    if (file.open(QIODevice::ReadOnly)) {
        QString contentType = getContentType(fileInfo.fileName());
        if (contentType != "") {
            submit(printerId, fileInfo.fileName(), "", contentPath, contentType, "");
        } else {
            qDebug() << "GCPClient::submit file type is not supported. (" << contentPath << ")";
        }
    }
}

void GCPClient::jobs(QString printerId)
{
    qDebug() << "----- GCPClient::jobs -----";

    QString uri = jobsURI;

    QByteArray authHeader;
    authHeader.append("Bearer ");
    authHeader.append(m_paramMap["access_token"]);

    qDebug() << "GCPClient::search authHeader " << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(jobsReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setRawHeader("Authorization", authHeader) ;
    req.setRawHeader("X-CloudPrint-Proxy", "Chrome");
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

QString GCPClient::getContentType(QString fileName) {
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    rx.indexIn(fileName);
    qDebug() << "GCPClient::getContentType fileName=" << fileName << " rx.captureCount()=" << rx.captureCount();
    for(int i=0; i<=rx.captureCount(); i++) {
        qDebug() << "i=" << i << " rx.cap=" << rx.cap(i);
    }
    QString fileExt = rx.cap(3).toLower();

    return m_contentTypeHash[fileExt];
}

void GCPClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCPClient::accessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QMap<QString, QString> map = createMapFromJson(replyBody);
        m_paramMap["access_token"] = map["access_token"];
        m_paramMap["refresh_token"] = map["refresh_token"];
    }

    emit accessTokenReplySignal(reply->error(), reply->errorString(), replyBody );
}

void GCPClient::refreshAccessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCPClient::refreshAccessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QMap<QString, QString> map = createMapFromJson(replyBody);
        m_paramMap["access_token"] = map["access_token"];
    }

    emit refreshAccessTokenReplySignal(reply->error(), reply->errorString(), replyBody );
}

void GCPClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCPClient::accountInfoReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        m_paramMap.unite(createMapFromJson(replyBody));
    }

    emit accountInfoReplySignal(reply->error(), reply->errorString(), replyBody );
}

void GCPClient::searchReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCPClient::searchReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    // TODO Collect printers into caches for further submitting.
    if (reply->error() == QNetworkReply::NoError) {
        m_printerHash = createHashFromJson(replyBody);
        qDebug() << "GCPClient::searchReplyFinished m_printerHash " << m_printerHash;

        emit searchReplySignal(reply->error(), reply->errorString(), "" );
    } else {
        emit searchReplySignal(reply->error(), reply->errorString(), replyBody );
    }
}

void GCPClient::submitReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCPClient::submitReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    emit submitReplySignal(reply->error(), reply->errorString(), replyBody );
}

void GCPClient::jobsReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCPClient::jobsReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    emit jobsReplySignal(reply->error(), reply->errorString(), replyBody );
}









