#include "gcdclient.h"
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QScriptEngine>
#include <QScriptValueIterator>

const QString GCDClient::consumerKey = "196573379494.apps.googleusercontent.com";
const QString GCDClient::consumerSecret = "il59cyz3dwBW6tsHBkZYGSWj";

const QString GCDClient::authorizationScope = "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile https://www.googleapis.com/auth/drive";

const QString GCDClient::authorizeURI = "https://accounts.google.com/o/oauth2/auth";
const QString GCDClient::accessTokenURI = "https://accounts.google.com/o/oauth2/token";
const QString GCDClient::accountInfoURI = "https://www.googleapis.com/oauth2/v2/userinfo";
const QString GCDClient::quotaURI = "https://www.googleapis.com/drive/v2/about";

const QString GCDClient::fileGetURI = "";
const QString GCDClient::filePutURI = "";
const QString GCDClient::metadataURI = "https://www.googleapis.com/drive/v2/files";
const QString GCDClient::createFolderURI = "";
const QString GCDClient::moveFileURI = "";
const QString GCDClient::copyFileURI = "";
const QString GCDClient::deleteFileURI = "";
const QString GCDClient::renameFileURI = "";
const QString GCDClient::sharesURI = "";

const QString GCDClient::insertURI = "https://www.googleapis.com/drive/v2/files";
const QString GCDClient::uploadURI = "https://www.googleapis.com/upload/drive/v2/files";
const QString GCDClient::patchMetadataURI = "https://www.googleapis.com/drive/v2/files/%1";
const QString GCDClient::updateMetadataURI = "https://www.googleapis.com/drive/v2/files/%1";

GCDClient::GCDClient(QObject *parent) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();

    // Populate contentTypeHash.
    m_contentTypeHash["jpg"] = "image/jpeg";
    m_contentTypeHash["png"] = "image/png";
    m_contentTypeHash["pdf"] = "application/pdf";
    m_contentTypeHash["txt"] = "text/plain";
    m_contentTypeHash["patch"] = "text/plain";
    m_contentTypeHash["log"] = "text/plain";
}

GCDClient::~GCDClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();
}

QString GCDClient::createTimestamp() {
    qint64 seconds = QDateTime::currentMSecsSinceEpoch() / 1000;

    return QString("%1").arg(seconds);
}

QString GCDClient::createNormalizedQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(QUrl::toPercentEncoding(key)).append("=").append(QUrl::toPercentEncoding(sortMap[key]));
    }

    return queryString;
}

QString GCDClient::encodeURI(const QString uri) {
    // Example: https://api.dropbox.com/1/metadata/sandbox/C/B/NES/Solomon's Key (E) [!].nes
    // All non-alphanumeric except : and / must be encoded.
    return QUrl::toPercentEncoding(uri, ":/");
}

QString GCDClient::createQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(key).append("=").append(sortMap[key]);
    }

    return queryString;
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

QString GCDClient::getContentType(QString fileName) {
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,5})$");
    rx.indexIn(fileName);
    qDebug() << "GCDClient::getContentType fileName=" << fileName << " rx.captureCount()=" << rx.captureCount();
    for(int i=0; i<=rx.captureCount(); i++) {
        qDebug() << "GCDClient::getContentType i=" << i << " rx.cap=" << rx.cap(i);
    }
    QString fileExt = rx.cap(3).toLower();

    QString contentType = m_contentTypeHash[fileExt];
    if (contentType == "") {
        contentType = "application/octet-stream";
    }

    return contentType;
}

void GCDClient::authorize(QString nonce)
{
    qDebug() << "----- GCDClient::authorize -----";

    QMap<QString, QString> sortMap;
    sortMap["response_type"] = "code";
    sortMap["client_id"] = consumerKey;
    sortMap["redirect_uri"] = "urn:ietf:wg:oauth:2.0:oob";
    sortMap["scope"] = authorizationScope;
    sortMap["state"] = nonce;
    sortMap["access_type"] = "offline";
    QString queryString = createQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, "GCDClient");
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

        qDebug() << "GCDClient::parseAuthorizationCode " << m_paramMap;

        return true;
    } else {
        return false;
    }
}

void GCDClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- GCDClient::accessToken -----" << pin;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["code"] = pin;
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
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
}

void GCDClient::refreshToken(QString nonce, QString uid)
{
    qDebug() << "----- GCDClient::refreshToken -----";

    refreshTokenUid = uid;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["refresh_token"] = accessTokenPairMap[uid].secret;
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
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
}

void GCDClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- GCDClient::accountInfo -----" << uid;

    QString accessToken;
    if (uid == "") {
        // After accessToken, then uses temp access token..
        accessToken = m_paramMap["access_token"];
    } else {
        accessToken = accessTokenPairMap[uid].token;
    }

    QString uri = accountInfoURI;
    qDebug() << "GCDClient::accountInfo uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QByteArray().append("Bearer ").append(accessToken));
    QNetworkReply *reply = manager->get(req);
}

void GCDClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- GCDClient::quota ----- uid" << uid;

    QString uri = quotaURI;
    qDebug() << "GCDClient::quota uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

void GCDClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::metadata -----";

//    QString uri = accountInfoURI;

//    // Send request.
//    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
//    QNetworkRequest req = QNetworkRequest(QUrl(uri));
//    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
//    QNetworkReply *reply = manager->get(req);
//    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    //    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCDClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::browse -----" << uid << remoteFilePath;

    QString uri = metadataURI + "?key=" + consumerKey;

    qDebug() << "GCDClient::browse access_oken" << accessTokenPairMap[uid].token;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(browseReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

void GCDClient::createFolder(QString nonce, QString uid, QString newRemoteFolderName, QString remoteParentPath)
{
}

void GCDClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath)
{
}

void GCDClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath)
{
}

void GCDClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
}

void GCDClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
}

void GCDClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath)
{
    qDebug() << "----- GCDClient::fileGet -----";

//    QString uri = accountInfoURI;

//    // Send request.
//    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
//    QNetworkRequest req = QNetworkRequest(QUrl(uri));
//    req.setRawHeader("Authorization", QByteArray().append("Bearer ").append(m_paramMap["access_token"]));
//    QNetworkReply *reply = manager->get(req);
//    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
//    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void GCDClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::filePut -----";

//    QString uri = insertURI;

//    QString contentType = getContentType(localFilePath);
//    qDebug() << "contentType " << contentType;

//    QByteArray authHeader;
//    authHeader.append("Bearer ");
//    authHeader.append(m_paramMap["access_token"]);
//    qDebug() << "authHeader " << authHeader;

//    QByteArray postData;
//    postData.append("{ ");
//    postData.append("kind: \"drive#file\", ");
//    postData.append( QString("title: \"%1\", ").arg(localFilePath) );
//    postData.append( QString("mimeType: \"%1\", ").arg(contentType) );
//    postData.append( QString("description: \"%1\" ").arg(localFilePath) );
//    postData.append(" }");
//    qDebug() << "postData " << postData;

//    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
//    QNetworkRequest req = QNetworkRequest(QUrl(uri));
//    req.setRawHeader("Authorization", authHeader) ;
//    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
//    req.setHeader(QNetworkRequest::ContentLengthHeader, postData.length());
//    QNetworkReply *reply = manager->post(req, postData);
//    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
//    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));


//    QString uri = uploadURI;

//    // Requires to submit job with multipart/form-data.
//    QString boundary = "----------" + nonce;
//    QByteArray postData;
//    QMap<QString, QString> paramMap;
//    paramMap["contentType"] = contentType;
//    paramMap["title"] = remoteFilePath;
//    QFile file(localFilePath);
//    if (file.open(QIODevice::ReadOnly)) {
//        postData = encodeMultiPart(boundary, paramMap, "content", file.fileName(), file.readAll(), contentType);
//    }
//    qDebug() << "postData " << postData;

//    localSourcefile = new QFile(localFilePath);
//    if (localSourcefile->open(QIODevice::ReadOnly)) {
//        qint64 fileSize = localSourcefile->size();

//        // Send request.
//        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
//        QNetworkRequest req = QNetworkRequest(QUrl(uri));
//        req.setRawHeader("Authorization", authHeader) ;
//        //    req.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data;boundary=" + boundary);
//        //    req.setHeader(QNetworkRequest::ContentLengthHeader, postData.length());
//        req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
//        req.setHeader(QNetworkRequest::ContentLengthHeader, fileSize);
//        QNetworkReply *reply = manager->post(req, localSourcefile);
//        connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
//        connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
//    }
}

void GCDClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::accessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
/*
 * Example response
{
  "access_token" : "ya29.AHES6ZQaY6RXd94kCKwCgppM3CKWlYXQ9BSR3t_7VpbWn86DPn_FWQ",
  "token_type" : "Bearer",
  "expires_in" : 3600,
  "id_token" : "eyJhbGciOiJSUzI1NiIsImtpZCI6IjNiNDcwODgyYjMwZTU5NTk5OTc2MzY5ZDdiMzFjZjBiOTBhMmEzNzkifQ.eyJpc3MiOiJhY2NvdW50cy5nb29nbGUuY29tIiwiYXVkIjoiMTk2NTczMzc5NDk0LmFwcHMuZ29vZ2xldXNlcmNvbnRlbnQuY29tIiwiZW1haWwiOiJ0YXdlZXNhay5raXR0aXB1cmVlQGdtYWlsLmNvbSIsInRva2VuX2hhc2giOiJMTFMwbUZtb2ZuVVFSb3o2UUlHVmhRIiwiY2lkIjoiMTk2NTczMzc5NDk0LmFwcHMuZ29vZ2xldXNlcmNvbnRlbnQuY29tIiwidmVyaWZpZWRfZW1haWwiOiJ0cnVlIiwiaWQiOiIxMTIwMzIzMTI3NjEyMTM1ODkzMDgiLCJpYXQiOjEzNTY1Nzg1ODMsImV4cCI6MTM1NjU4MjQ4M30.TVZLBHkR1oRbAA2_kl-Mx7oBg8iNwUXgvGDEKZGxJCy4eZno_gSTbozePAK24N2OrJTVgQ-yPGX5d9JrY9nmdOG1KWkahgLL_Hl1vaOfjr4ELadBo5Vj8nvQTeu35meRw275NFQvMjKKdRM3aR8AYPKIgUvG4nMuHPwb9khkDwg",
  "refresh_token" : "1/iF_z2SVQwnsU7LLmQpPZHM1PkjIBG_QL1qN-bkb0_2Q"
}
 */
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        m_paramMap["access_token"] = sc.property("access_token").toString();
        m_paramMap["refresh_token"] = sc.property("refresh_token").toString();
        m_paramMap["id_token"] = sc.property("id_token").toString();
        qDebug() << "GCDClient::accessTokenReplyFinished m_paramMap " << m_paramMap;

        if (refreshTokenUid != "") {
            // Updates for refreshToken.
            accessTokenPairMap[refreshTokenUid].token = sc.property("access_token").toString();
            accessTokenPairMap[refreshTokenUid].secret = sc.property("refresh_token").toString();
            qDebug() << "GCDClient::accessTokenReplyFinished accessTokenPairMap" << accessTokenPairMap;

            // Reset refreshTokenUid.
            refreshTokenUid = "";
        }
    }

    emit accessTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::accountInfoReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
/*
 * Example response.
{
 "id": "112032312761213589308",
 "email": "taweesak.kittipuree@gmail.com",
 "verified_email": true,
 "name": "Taweesak Kittipuree",
 "given_name": "Taweesak",
 "family_name": "Kittipuree",
 "link": "https://plus.google.com/112032312761213589308",
 "gender": "male",
 "locale": "en"
}
 */
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        QString uid = sc.property("id").toString();
        QString name = sc.property("name").toString();
        QString email = sc.property("email").toString();

        if (accessTokenPairMap.contains(uid)) {
            accessTokenPairMap[uid].email = email;

            if (m_paramMap.contains("access_token")) {
                accessTokenPairMap[uid].token = m_paramMap["access_token"];
                accessTokenPairMap[uid].secret = m_paramMap["refresh_token"];

                // Reset temp. accessToken and refreshToken.
                m_paramMap.remove("access_token");
                m_paramMap.remove("refresh_token");
            }
        } else {
            TokenPair accessTokenPair;
            accessTokenPair.token = m_paramMap["access_token"];
            accessTokenPair.secret = m_paramMap["refresh_token"];
            accessTokenPair.email = email;

            if (uid != "" && accessTokenPair.token != "" && accessTokenPair.secret != "") {
                accessTokenPairMap[uid] = accessTokenPair;
            }

            // Reset temp. accessToken and refreshToken.
            m_paramMap.remove("access_token");
            m_paramMap.remove("refresh_token");
        }

//        qDebug() << "GCDClient::accountInfoReplyFinished accessTokenPairMap" << accessTokenPairMap;

        // Save account after got id and email.
        saveAccessPairMap();
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::quotaReplyFinished(QNetworkReply *reply)
{
/*
 * Example response
{ ...
 "quotaBytesTotal": "5368709120",
 "quotaBytesUsed": "4687520",
 "quotaBytesUsedAggregate": "4687520",
 "quotaBytesUsedInTrash": "4679290",
... }
 */
    qDebug() << "GCDClient::quotaReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit quotaReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll() );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::fileGetReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::fileGetReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    // TODO Collect printers into caches for further submitting.
    if (reply->error() == QNetworkReply::NoError) {
        emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), "" );
    } else {
        emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), replyBody );
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filePutReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::filePutReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    // TODO Collect printers into caches for further submitting.
    if (reply->error() == QNetworkReply::NoError) {
        emit filePutReplySignal(nonce, reply->error(), reply->errorString(), "" );
    } else {
        emit filePutReplySignal(nonce, reply->error(), reply->errorString(), replyBody );
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::metadataReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::metadataReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    // TODO Collect printers into caches for further submitting.
    if (reply->error() == QNetworkReply::NoError) {
        emit metadataReplySignal(nonce, reply->error(), reply->errorString(), replyBody );
    } else {
        emit metadataReplySignal(nonce, reply->error(), reply->errorString(), replyBody );
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::browseReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::browseReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        emit browseReplySignal(nonce, reply->error(), reply->errorString(), replyBody );
    } else {
        emit browseReplySignal(nonce, reply->error(), reply->errorString(), replyBody );
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::createFolderReplyFinished(QNetworkReply *reply)
{
}

void GCDClient::moveFileReplyFinished(QNetworkReply *reply)
{
}

void GCDClient::copyFileReplyFinished(QNetworkReply *reply)
{
}

void GCDClient::deleteFileReplyFinished(QNetworkReply *reply)
{
}

void GCDClient::shareFileReplyFinished(QNetworkReply *reply)
{
}
