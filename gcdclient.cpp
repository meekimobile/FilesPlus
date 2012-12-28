#include "gcdclient.h"
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QScriptEngine>
#include <QScriptValueIterator>

const QString GCDClient::consumerKey = "196573379494.apps.googleusercontent.com";
const QString GCDClient::consumerSecret = "il59cyz3dwBW6tsHBkZYGSWj";

const QString GCDClient::GCDRootFolderId = "root";

const QString GCDClient::authorizationScope = "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile https://www.googleapis.com/auth/drive";

const QString GCDClient::authorizeURI = "https://accounts.google.com/o/oauth2/auth";
const QString GCDClient::accessTokenURI = "https://accounts.google.com/o/oauth2/token";
const QString GCDClient::accountInfoURI = "https://www.googleapis.com/oauth2/v2/userinfo";
const QString GCDClient::quotaURI = "https://www.googleapis.com/drive/v2/about";

const QString GCDClient::fileGetURI = "";
const QString GCDClient::filePutURI = "";
const QString GCDClient::filesURI = "https://www.googleapis.com/drive/v2/files"; // GET with q
const QString GCDClient::propertyURI = "https://www.googleapis.com/drive/v2/files/%1"; // GET
const QString GCDClient::createFolderURI = "https://www.googleapis.com/drive/v2/files"; // POST with json.
const QString GCDClient::moveFileURI = "https://www.googleapis.com/drive/v2/files/%1"; // PATCH with partial json body.
const QString GCDClient::copyFileURI = "https://www.googleapis.com/drive/v2/files/%1/copy"; // POST with partial json body.
const QString GCDClient::deleteFileURI = "https://www.googleapis.com/drive/v2/files/%1/trash"; // POST
const QString GCDClient::renameFileURI = "https://www.googleapis.com/drive/v2/files/%1"; // PATCH with partial json body.
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

    m_propertyReplyHash = new QHash<QString, QByteArray>;
    m_filesReplyHash = new QHash<QString, QByteArray>;
}

GCDClient::~GCDClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_propertyReplyHash = 0;
    m_filesReplyHash = 0;
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

    if (accessTokenPairMap[uid].secret == "") {
        qDebug() << "GCDClient::refreshToken refreshToken is empty. Operation is aborted.";
        emit accessTokenReplySignal(nonce, -1, "Refresh token is missing", "Refresh token is missing. Please authorize GoogleDrive account again.");
        return;
    }

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
    qDebug() << "----- GCDClient::metadata -----" << uid << remoteFilePath;
    if (remoteFilePath.isEmpty()) {
        emit browseReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    property(nonce, uid, remoteFilePath, false, "metadata");
    files(nonce, uid, remoteFilePath, false, "metadata");
}

void GCDClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::browse -----" << uid << remoteFilePath;
    if (remoteFilePath.isEmpty()) {
        emit browseReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    property(nonce, uid, remoteFilePath, false, "browse");
    files(nonce, uid, remoteFilePath, false, "browse");
}

void GCDClient::createFolder(QString nonce, QString uid, QString newRemoteFolderName, QString remoteParentPath)
{
    createFolder(nonce, uid, newRemoteFolderName, remoteParentPath, false);
}

QNetworkReply * GCDClient::createFolder(QString nonce, QString uid, QString newRemoteFolderName, QString remoteParentPath, bool synchronous)
{
    qDebug() << "----- GCDClient::createFolder -----" << newRemoteFolderName << remoteParentPath;

    if (remoteParentPath.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return 0;
    }

    if (newRemoteFolderName.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return 0;
    }

    QString uri = createFolderURI;
    qDebug() << "GCDClient::createFolder uri " << uri;
/*
    {
      "title": "pets",
      "parents": [{"id":"0ADK06pfg"}]
      "mimeType": "application/vnd.google-apps.folder"
    }
*/
    QByteArray postData;
    postData.append("{");
    postData.append(" \"title\": \"" + newRemoteFolderName.toUtf8() + "\", ");
    postData.append(" \"parents\": [{\"id\":\"" + remoteParentPath.toUtf8() + "\"}], ");
    postData.append(" \"mimeType\": \"application/vnd.google-apps.folder\" ");
    postData.append("}");
    qDebug() << "GCDClient::createFolder postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(req, postData);

    // TODO Return if asynchronous.
    if (!synchronous) {
        return reply;
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

void GCDClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- GCDClient::moveFile -----" << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

    if (newRemoteFileName != "") {
        // Proceed renaming.
        renameFile(nonce, uid, remoteFilePath, newRemoteFileName);
        return;
    }

    QString uri = moveFileURI.arg(remoteFilePath);
    qDebug() << "GCDClient::moveFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"parents\": [{ \"id\": \"" + targetRemoteParentPath.toUtf8() + "\" }] }");
    qDebug() << "GCDClient::moveFile postData" << postData;

    // Insert buffer to hash.
    m_bufferHash.insert(nonce, new QBuffer());
    m_bufferHash[nonce]->open(QIODevice::WriteOnly);
    m_bufferHash[nonce]->write(postData);
    m_bufferHash[nonce]->close();

    if (m_bufferHash[nonce]->open(QIODevice::ReadOnly)) {
        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *reply = manager->sendCustomRequest(req, "PATCH", m_bufferHash[nonce]);
    }
}

void GCDClient::renameFile(QString nonce, QString uid, QString remoteFilePath, QString newName)
{
    qDebug() << "----- GCDClient::renameFile -----" << remoteFilePath << newName;

    QString uri = renameFileURI.arg(remoteFilePath);
    qDebug() << "GCDClient::renameFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"title\": \"");
    postData.append(newName.toUtf8());
    postData.append("\" }");
    qDebug() << "GCDClient::renameFile postData" << postData;

    // Insert buffer to hash.
    m_bufferHash.insert(nonce, new QBuffer());
    m_bufferHash[nonce]->open(QIODevice::WriteOnly);
    m_bufferHash[nonce]->write(postData);
    m_bufferHash[nonce]->close();

    if (m_bufferHash[nonce]->open(QIODevice::ReadOnly)) {
        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *reply = manager->sendCustomRequest(req, "PATCH", m_bufferHash[nonce]);
    }
}

void GCDClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- GCDClient::copyFile -----" << uid << remoteFilePath << targetRemoteParentPath;

    QString uri = copyFileURI.arg(remoteFilePath);
    qDebug() << "GCDClient::copyFile uri " << uri;

    QByteArray postData;
    postData.append("{");
    if (newRemoteFileName != "") {
        postData.append(" \"title\": \"" + newRemoteFileName.toUtf8() + "\", ");
    }
    postData.append(" \"parents\": [{ \"id\": \"" + targetRemoteParentPath.toUtf8() + "\" }] ");
    postData.append("}");
    qDebug() << "GCDClient::copyFile postData" << postData;

    // Insert buffer to hash.
    m_bufferHash.insert(nonce, new QBuffer());
    m_bufferHash[nonce]->open(QIODevice::WriteOnly);
    m_bufferHash[nonce]->write(postData);
    m_bufferHash[nonce]->close();

    if (m_bufferHash[nonce]->open(QIODevice::ReadOnly)) {
        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(copyFileReplyFinished(QNetworkReply*)) );
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        QNetworkReply *reply = manager->sendCustomRequest(req, "POST", m_bufferHash[nonce]);
    }
}

void GCDClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    deleteFile(nonce, uid, remoteFilePath, false);
}

QNetworkReply * GCDClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- GCDClient::deleteFile -----";

    QString uri = deleteFileURI.arg(remoteFilePath);
    qDebug() << "GCDClient::deleteFile uri " << uri;

    QByteArray postData;
    qDebug() << "GCDClient::deleteFile postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deleteFileReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->post(req, postData);

    // TODO Return if asynchronous.
    if (!synchronous) {
        return reply;
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

void GCDClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
}

QNetworkReply * GCDClient::files(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- GCDClient::files -----" << remoteFilePath;

    QApplication::processEvents();

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["key"] = consumerKey;
    sortMap["q"] = QUrl::toPercentEncoding(QString("'%1' in parents and trashed = false").arg((remoteFilePath == "") ? GCDRootFolderId : remoteFilePath));
    QString queryString = createQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QString uri = filesURI + "?" + queryString;
    qDebug() << "GCDClient::files uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filesReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(callback));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);

    // TODO Return if asynchronous.
    if (!synchronous) {
        return reply;
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

QNetworkReply * GCDClient::property(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- GCDClient::property -----" << remoteFilePath;

    QApplication::processEvents();

    QString uri = propertyURI.arg((remoteFilePath == "") ? GCDRootFolderId : remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "GCDClient::property uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(propertyReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(callback));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);

    // TODO Return if asynchronous.
    if (!synchronous) {
        return reply;
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper().sleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
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

        if (refreshTokenUid != "" && sc.property("access_token").toString() != "") {
            // NOTE first accessTokenReply will contain both accessToken and refreshToken. But for refresh's accessTokenReply will contain only accessToken.
            // Update accessToken but retains refreshToken.
            accessTokenPairMap[refreshTokenUid].token = sc.property("access_token").toString();
            qDebug() << "GCDClient::accessTokenReplyFinished accessTokenPairMap" << accessTokenPairMap;

            // Reset refreshTokenUid.
            refreshTokenUid = "";
        }

        // NOTE uid and email will be requested to accountInfo by CloudDriveModel.accessTokenReplyFilter().
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
        qDebug() << "GCDClient::accountInfoReplyFinished m_paramMap" << m_paramMap;

        if (accessTokenPairMap.contains(uid)) {
            qDebug() << "GCDClient::accountInfoReplyFinished found existing accessToken for uid" << uid << accessTokenPairMap[uid];

            accessTokenPairMap[uid].email = email;

            if (m_paramMap.contains("access_token")) {
                accessTokenPairMap[uid].token = m_paramMap["access_token"];
                accessTokenPairMap[uid].secret = m_paramMap["refresh_token"];

                // Reset temp. accessToken and refreshToken.
                m_paramMap.remove("access_token");
                m_paramMap.remove("refresh_token");
            }
        } else {
            qDebug() << "GCDClient::accountInfoReplyFinished not found existing accessToken for uid" << uid;

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

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll() );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::browseReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::browseReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll() );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::propertyReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::propertyReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    if (reply->error() == QNetworkReply::NoError) {
        m_propertyReplyHash->insert(nonce, reply->readAll());
    } else {
        // Remove once used.
        m_propertyReplyHash->remove(nonce);
        m_filesReplyHash->remove(nonce);

        // Property is mandatory. Emit error if error occurs.
        if (callback == "browse") {
            emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());
        } else {
            qDebug() << "GCDClient::propertyReplyFinished invalid callback" << callback;
        }
    }

    if (m_propertyReplyHash->contains(nonce) && m_filesReplyHash->contains(nonce)) {
        QScriptEngine engine;
        QScriptValue sc;
        QScriptValue scProperty;
        QScriptValue scJsonStringify;
        sc = engine.evaluate("(" + QString::fromUtf8(m_filesReplyHash->value(nonce)) + ")");
        scProperty = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
        sc.setProperty("property", scProperty);
        scJsonStringify = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << sc);

        // Remove once used.
        m_propertyReplyHash->remove(nonce);
        m_filesReplyHash->remove(nonce);

        if (callback == "browse") {
            emit browseReplySignal(nonce, QNetworkReply::NoError, "", scJsonStringify.toString());
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, QNetworkReply::NoError, "", scJsonStringify.toString());
        } else {
            qDebug() << "GCDClient::propertyReplyFinished invalid callback" << callback;
        }
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filesReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::filesReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    m_filesReplyHash->insert(nonce, reply->readAll());

    if (m_propertyReplyHash->contains(nonce) && m_filesReplyHash->contains(nonce)) {
        QScriptEngine engine;
        QScriptValue sc;
        QScriptValue scProperty;
        QScriptValue scJsonStringify;
        sc = engine.evaluate("(" + QString::fromUtf8(m_filesReplyHash->value(nonce)) + ")");
        scProperty = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
        sc.setProperty("property", scProperty);
        scJsonStringify = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << sc);

        // Remove once used.
        m_propertyReplyHash->remove(nonce);
        m_filesReplyHash->remove(nonce);

        if (callback == "browse") {
            emit browseReplySignal(nonce, QNetworkReply::NoError, "", scJsonStringify.toString());
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, QNetworkReply::NoError, "", scJsonStringify.toString());
        } else {
            qDebug() << "GCDClient::filesReplyFinished invalid callback" << callback;
        }
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::createFolderReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::createFolderReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::moveFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::moveFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit moveFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // Remove request buffer.
    if (m_bufferHash.contains(nonce)) {
        m_bufferHash[nonce]->close();
        m_bufferHash.remove(nonce);
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::copyFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::copyFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // Remove request buffer.
    m_bufferHash[nonce]->close();
    m_bufferHash.remove(nonce);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::deleteFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::shareFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::shareFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}
