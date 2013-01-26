#include "gcdclient.h"
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QScriptEngine>
#include <QScriptValueIterator>

const QString GCDClient::consumerKey = "196573379494.apps.googleusercontent.com";
const QString GCDClient::consumerSecret = "il59cyz3dwBW6tsHBkZYGSWj";

const QString GCDClient::RemoteRoot = "root";

const QString GCDClient::authorizationScope = "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile https://www.googleapis.com/auth/drive";

const QString GCDClient::authorizeURI = "https://accounts.google.com/o/oauth2/auth";
const QString GCDClient::accessTokenURI = "https://accounts.google.com/o/oauth2/token";
const QString GCDClient::accountInfoURI = "https://www.googleapis.com/oauth2/v2/userinfo";
const QString GCDClient::quotaURI = "https://www.googleapis.com/drive/v2/about";

const QString GCDClient::fileGetURI = "%1"; // Use downloadUrl from property.
const QString GCDClient::filePutURI = "https://www.googleapis.com/upload/drive/v2/files"; // POST with ?uploadType=multipart
const QString GCDClient::filesURI = "https://www.googleapis.com/drive/v2/files"; // GET with q
const QString GCDClient::propertyURI = "https://www.googleapis.com/drive/v2/files/%1"; // GET
const QString GCDClient::createFolderURI = "https://www.googleapis.com/drive/v2/files"; // POST with json.
const QString GCDClient::copyFileURI = "https://www.googleapis.com/drive/v2/files/%1/copy"; // POST with partial json body.
const QString GCDClient::deleteFileURI = "https://www.googleapis.com/drive/v2/files/%1"; // DELETE
const QString GCDClient::patchFileURI = "https://www.googleapis.com/drive/v2/files/%1"; // PATCH with partial json body.
const QString GCDClient::sharesURI = "https://www.googleapis.com/drive/v2/files/%1/permissions"; // POST to insert permission.
const QString GCDClient::trashFileURI = "https://www.googleapis.com/drive/v2/files/%1/trash"; // POST
const QString GCDClient::startResumableUploadURI = "https://www.googleapis.com/upload/drive/v2/files?uploadType=resumable"; // POST with json includes file properties.
const QString GCDClient::resumeUploadURI = "%1"; // PUT with specified URL.

const qint64 GCDClient::ChunkSize = 4194304; // 4194304=4MB, 1048576=1MB  TODO Optimize to have largest chink size which is still valid before token is expired.

GCDClient::GCDClient(QObject *parent) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();

    m_propertyReplyHash = new QHash<QString, QByteArray>;
    m_filesReplyHash = new QHash<QString, QByteArray>;

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
        emit metadataReplySignal(nonce, -1, "remoteFilePath is empty.", "");
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

void GCDClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteParentPath, newRemoteFolderName, false);
}

QString GCDClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- GCDClient::createFolder -----" << remoteParentPath << newRemoteFolderName;

    if (remoteParentPath.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
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
        return "";
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal.
    QString replyString = QString::fromUtf8(reply->readAll());
    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyString);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyString;
}

void GCDClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- GCDClient::moveFile -----" << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

    QByteArray postData;
    if (newRemoteFileName != "" && targetRemoteParentPath == "") {
        // Renaming.
        postData.append("{ \"title\": \"" + newRemoteFileName.toUtf8() + "\" }");
    } else {
        // Moving.
        postData.append("{ \"parents\": [{ \"id\": \"" + targetRemoteParentPath.toUtf8() + "\" }] }");
    }

    // Send request.
    QNetworkReply * reply = patchFile(nonce, uid, remoteFilePath, postData);

    // Proceed to slot.
    moveFileReplyFinished(reply);
}

QNetworkReply * GCDClient::patchFile(QString nonce, QString uid, QString remoteFilePath, QByteArray postData)
{
    qDebug() << "----- GCDClient::patchFile -----" << remoteFilePath << postData;

    QString uri = patchFileURI.arg(remoteFilePath);
    qDebug() << "GCDClient::patchFile uri " << uri;

    // Insert buffer to hash.
    QBuffer *postDataBuf = new QBuffer();
    postDataBuf->open(QIODevice::WriteOnly);
    postDataBuf->write(postData);
    postDataBuf->close();

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(postData));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->sendCustomRequest(req, "PATCH", postDataBuf);

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    postDataBuf->deleteLater();

    return reply;
}

QIODevice *GCDClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- GCDClient::fileGet -----" << nonce << remoteFilePath << "offset" << offset << "synchronous" << synchronous;

    QString uri = remoteFilePath;
    // TODO It should be downloadUrl because it will not be able to create connection in CloudDriveModel.fileGetReplyFilter.
    if (!remoteFilePath.startsWith("http")) {
        // remoteFilePath is not a URL. Procees getting property to get downloadUrl.
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "fileGet");
        if (propertyReply->error() == QNetworkReply::NoError) {
            // Stores property for using in fileGetReplyFinished().
            m_propertyReplyHash->insert(nonce, propertyReply->readAll());

            QScriptEngine engine;
            QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
            uri = sc.property("downloadUrl").toString();
            propertyReply->deleteLater();
        } else {
            emit fileGetReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
            propertyReply->deleteLater();
            return 0;
        }
    }
    qDebug() << "GCDClient::fileGet uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+ChunkSize-1);
        qDebug() << "GCDClient::fileGet rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    return reply;
}

QString GCDClient::fileGetReplySave(QNetworkReply *reply)
{
    qDebug() << "GCDClient::fileGetReplySave " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "GCDClient::fileGetReplySave reply bytesAvailable" << reply->bytesAvailable();

        // Stream replyBody to a file on localPath.
        qint64 totalBytes = 0;
        char buf[1024];
        QFile *localTargetFile = m_localFileHash[nonce];
        if (localTargetFile->open(QIODevice::Append)) {
            // Issue: Writing to file with QDataStream << QByteArray will automatically prepend with 4-bytes prefix(size).
            // Solution: Use QIODevice to write directly.

            // Read first buffer.
            qint64 c = reply->read(buf, sizeof(buf));
            while (c > 0) {
                localTargetFile->write(buf, c);
                totalBytes += c;

                // Tell event loop to process event before it will process time consuming task.
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

                // Read next buffer.
                c = reply->read(buf, sizeof(buf));
            }
        }

        qDebug() << "GCDClient::fileGetReplySave reply totalBytes=" << totalBytes;

        // Close target file.
        localTargetFile->close();

        return QString::fromUtf8(m_propertyReplyHash->value(nonce));
    } else {
        qDebug() << "GCDClient::fileGetReplySave nonce" << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        return QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Remove once used.
    m_localFileHash.remove(nonce);
    m_propertyReplyHash->remove(nonce);
}

QNetworkReply *GCDClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- GCDClient::filePut -----" << nonce << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString uri = filePutURI + "?uploadType=media";
    qDebug() << "GCDClient::filePut uri " << uri;

    QString contentType = getContentType(remoteFileName);

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteParentPath));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(remoteFileName));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(contentType));
    req.setHeader(QNetworkRequest::ContentLengthHeader, bytesTotal);
    QNetworkReply *reply = manager->post(req, source);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (synchronous) {
        // Close source file.
        source->close();

        if (reply->error() == QNetworkReply::NoError) {
            QScriptEngine engine;
            QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(reply->readAll()) + ")");
            QString remoteFilePath = sc.property("id").toString();

            QByteArray metadata;
            metadata.append("{");
            metadata.append(" \"title\": \"" + remoteFileName.toUtf8() + "\", ");
            metadata.append(" \"parents\": [{ \"id\": \"" + remoteParentPath.toUtf8() + "\" }] ");
            metadata.append("}");
            qDebug() << "GCDClient::filePut patch metadata" << metadata;

            reply = patchFile(nonce, uid, remoteFilePath, metadata);
        }
    }

    return reply;
}

QNetworkReply *GCDClient::filePutMulipart(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- GCDClient::filePutMulipart -----" << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString uri = filePutURI + "?uploadType=multipart";
    qDebug() << "GCDClient::filePutMulipart uri " << uri;

    if (source->open(QIODevice::ReadOnly)) {
        qint64 fileSize = source->size();
        QString contentType = getContentType(remoteFileName);
        qDebug() << "GCDClient::filePutMulipart remoteFileName" << remoteFileName << "contentType" << contentType << "fileSize" << fileSize;

        // Requires to submit job with multipart.
        QString boundary = "----------" + nonce;
        QString CRLF = "\r\n";

        QByteArray metadata;
        metadata.append("{");
        metadata.append(" \"title\": \"" + remoteFileName.toUtf8() + "\", ");
        metadata.append(" \"parents\": [{ \"id\": \"" + remoteParentPath.toUtf8() + "\" }] ");
        metadata.append("}");
        qDebug() << "GCDClient::filePutMulipart metadata " << metadata;

        QByteArray postData;
        postData.append("--" + boundary + CRLF);
        postData.append("Content-Type: application/json; charset=UTF-8" + CRLF);
        postData.append(CRLF);
        postData.append(metadata);
        postData.append(CRLF);
        postData.append("--" + boundary + CRLF);
        postData.append("Content-Type: " + contentType + CRLF);
        postData.append(CRLF);
        postData.append(source->readAll());
        postData.append(CRLF);
        postData.append("--" + boundary + "--" + CRLF);
        qDebug() << "postData size" << postData.size();

        // Insert buffer to hash.
        m_bufferHash.insert(nonce, new QBuffer());
        m_bufferHash[nonce]->open(QIODevice::WriteOnly);
        m_bufferHash[nonce]->write(postData);
        m_bufferHash[nonce]->close();

        if (m_bufferHash[nonce]->open(QIODevice::ReadOnly)) {
            // Send request.
            QNetworkAccessManager *manager = new QNetworkAccessManager(this);
            if (!synchronous) {
                connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutMultipartReplyFinished(QNetworkReply*)));
            }
            QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
            req.setAttribute(QNetworkRequest::User, QVariant(nonce));
            req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
            req.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/related; boundary=\"" + boundary + "\"");
            req.setHeader(QNetworkRequest::ContentLengthHeader, postData.length());
            QNetworkReply *reply = manager->post(req, m_bufferHash[nonce]->readAll());
            QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
            connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));

            while (synchronous && !reply->isFinished()) {
                QApplication::processEvents(QEventLoop::AllEvents, 100);
                Sleeper::msleep(100);
            }

            m_bufferHash.remove(nonce);

            return reply;
        }
    } else {
        qDebug() << "GCDClient::filePutMulipart source can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open source.", "Can't open source.");
    }

    return 0;
}

QIODevice *GCDClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- GCDClient::fileGetResume -----" << remoteFilePath << "to" << localFilePath << "offset" << offset;

    QString uri = remoteFilePath;
    // TODO It should be downloadUrl because it will not be albe to create connection in CloudDriveModel.fileGetReplyFilter.
    if (!remoteFilePath.startsWith("http")) {
        // remoteFilePath is not a URL. Procees getting property to get downloadUrl.
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "fileGetResume");
        if (propertyReply->error() == QNetworkReply::NoError) {
            // For further using in fileGetReplyFinished.
            m_propertyReplyHash->insert(nonce, propertyReply->readAll());

            QScriptEngine engine;
            QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
            uri = sc.property("downloadUrl").toString();
            propertyReply->deleteLater();
        } else {
            emit fileGetReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
            propertyReply->deleteLater();
            return 0;
        }
    }
    qDebug() << "GCDClient::fileGetResume uri " << uri;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetResumeReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+ChunkSize-1);
        qDebug() << "GCDClient::fileGetResume rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    return reply;
}

QNetworkReply *GCDClient::filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset)
{
    qDebug() << "----- GCDClient::filePutResume -----" << nonce << uid << localFilePath << remoteParentPath << remoteFileName << uploadId << offset;

    if (localFilePath.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "localFilePath is empty.", "");
        return 0;
    }

    if (remoteParentPath.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return 0;
    }

    if (remoteFileName.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "remoteFileName is empty.", "");
        return 0;
    }

    QFileInfo localFileInfo(localFilePath);

    if (uploadId == "") {
        qDebug() << "GCDClient::filePutResume redirect to filePutResumeStart" << uploadId << offset;
        filePutResumeStart(nonce, uid, remoteFileName, localFileInfo.size(), remoteParentPath, false);
    } else {
        if (offset < 0) {
            // Request latest uploading status if offset = -1.
            qDebug() << "GCDClient::filePutResume redirect to filePutResumeStatus" << uploadId << offset;
            filePutResumeStatus(nonce, uid, remoteFileName, localFileInfo.size(), uploadId, offset, false);
        } else {
            qDebug() << "GCDClient::filePutResume redirect to filePutResumeUpload" << uploadId << offset;
            m_localFileHash[nonce] = new QFile(localFilePath);
            QFile *localSourceFile = m_localFileHash[nonce];
            if (localSourceFile->open(QIODevice::ReadOnly)) {
                qint64 fileSize = localSourceFile->size();

                localSourceFile->seek(offset);

                filePutResumeUpload(nonce, uid, localSourceFile, remoteFileName, fileSize, uploadId, offset, false);
            } else {
                qDebug() << "GCDClient::filePutResumeUpload file " << localFilePath << " can't be opened.";
                emit filePutResumeReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
            }
        }
    }
}

QString GCDClient::filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous)
{
    qDebug() << "----- GCDClient::filePutResumeStart -----" << nonce << uid << fileName << bytesTotal << remoteParentPath << "synchronous" << synchronous;

    if (remoteParentPath.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return 0;
    }

    QString contentType = getContentType(fileName);

    QString uri = startResumableUploadURI;
    qDebug() << "GCDClient::filePutResumeStart uri " << uri;
/*
    {
      "title": "pets",
      "parents": [{"id":"0ADK06pfg"}]
    }
*/
    QByteArray postData;
    postData.append("{");
    postData.append(" \"title\": \"" + fileName.toUtf8() + "\", ");
    postData.append(" \"parents\": [{\"id\":\"" + remoteParentPath.toUtf8() + "\"}] ");
    postData.append("}");
    qDebug() << "GCDClient::filePutResumeStart postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setRawHeader("X-Upload-Content-Type", contentType.toAscii() );
    req.setRawHeader("X-Upload-Content-Length", QString("%1").arg(bytesTotal).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, postData.length());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(req, postData);

    // Return immediately if it's not synchronous.
    if (!synchronous) return "";

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Construct result.
    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        QString uploadUrl = reply->header(QNetworkRequest::LocationHeader).toString();
        result = QString("{ \"upload_id\": \"%1\" }").arg(uploadUrl);
    } else {
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString GCDClient::filePutResumeUpload(QString nonce, QString uid, QIODevice *source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    /*
     *NOTE
     *source must be seeked to required offset before invoking this method.
     *bytesTotal is total source file size.
     *offset is uploading offset.
     */
    qDebug() << "----- GCDClient::filePutResumeUpload -----" << nonce << uid << fileName << bytesTotal << uploadId << offset << "synchronous" << synchronous;

    QString uri = resumeUploadURI.arg(uploadId);
    QUrl url(uri);
    qDebug() << "GCDClient::filePutResumeUpload url " << url;

    qint64 chunkSize = qMin(bytesTotal-offset, ChunkSize);
    QString contentRange = QString("bytes %1-%2/%3").arg(offset).arg(offset+chunkSize-1).arg(bytesTotal);
    qDebug() << "GCDClient::filePutResumeUpload source->size()" << source->size() << "bytesTotal" << bytesTotal << "offset" << offset << "chunkSize" << chunkSize << "contentRange" << contentRange;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeUploadReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(url);
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, chunkSize);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(getContentType(fileName)));
    req.setRawHeader("Content-Range", contentRange.toAscii() );
    QNetworkReply *reply = manager->put(req, source);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));

    // Return immediately if it's not synchronous.
    if (!synchronous) return "";

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Construct result.
    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->hasRawHeader("Range")) {
            QStringList ranges = QString::fromAscii(reply->rawHeader("Range")).split("-");
            qint64 offset = ranges.at(1).toUInt() + 1;
            qDebug() << "GCDClient::filePutResumeUpload ranges" << ranges << "offset" << offset;
            result = QString("{ \"offset\": %1 }").arg(offset);
        } else {
            result = QString::fromUtf8(reply->readAll());
        }
    } else {
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString GCDClient::filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    qDebug() << "----- GCDClient::filePutResumeStatus -----" << nonce << uid << fileName << bytesTotal << uploadId << offset << "synchronous" << synchronous;

    QString uri = resumeUploadURI.arg(uploadId);
    qDebug() << "GCDClient::filePutResumeStatus uri " << uri;

    // Send request.
    // Put empty request with Content-Length: 0, Content-Range: bytes */<fileSize>.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeStatusReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, 0);
    req.setRawHeader("Content-Range", QString("bytes */%1").arg(bytesTotal).toAscii() );
    QNetworkReply *reply = manager->put(req, QByteArray());

    // Return immediately if it's not synchronous.
    if (!synchronous) return "";

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Construct result.
    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->hasRawHeader("Range")) {
            QStringList ranges = QString::fromAscii(reply->rawHeader("Range")).split("-");
            qint64 offset = ranges.at(1).toUInt() + 1;
            qDebug() << "GCDClient::filePutResumeStatus ranges" << ranges << "offset" << offset;
            result = QString("{ \"offset\": %1 }").arg(offset);
        } else {
            result = QString::fromUtf8(reply->readAll());
        }
    } else {
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString GCDClient::filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous)
{
    return "";
}

QString GCDClient::getRemoteRoot()
{
    return RemoteRoot;
}

bool GCDClient::isFilePutResumable(qint64 fileSize)
{
    return (fileSize >= ChunkSize);
}

bool GCDClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize >= ChunkSize);
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

QString GCDClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
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
//    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReply *reply = manager->deleteResource(req);

    // TODO Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return QString::fromUtf8(reply->readAll());
}

void GCDClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::shareFile -----" << uid << remoteFilePath;
    if (remoteFilePath.isEmpty()) {
        emit shareFileReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "shareFile");
    if (propertyReply->error() == QNetworkReply::NoError) {
        // For further using in shareFileReplyFinished.
        m_propertyReplyHash->insert(nonce, propertyReply->readAll());
        propertyReply->deleteLater();
    } else {
        emit shareFileReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
        propertyReply->deleteLater();
        return;
    }

    QString uri = sharesURI.arg(remoteFilePath);
    qDebug() << "GCDClient::shareFile uri " << uri;

    QByteArray postData;
    postData.append("{");
    postData.append(" \"role\": \"reader\", ");
    postData.append(" \"type\": \"anyone\", ");
    postData.append(" \"value\": \"me\", ");
    postData.append(" \"withLink\": true ");
    postData.append("}");
    qDebug() << "GCDClient::copyFile postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(req, postData);
}

QNetworkReply * GCDClient::files(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- GCDClient::files -----" << remoteFilePath;

    QApplication::processEvents();

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["key"] = consumerKey;
    sortMap["q"] = QUrl::toPercentEncoding(QString("'%1' in parents and trashed = false").arg((remoteFilePath == "") ? RemoteRoot : remoteFilePath));
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
        Sleeper::msleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

QNetworkReply * GCDClient::property(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- GCDClient::property -----" << remoteFilePath << callback;

    QApplication::processEvents();

    QString uri = propertyURI.arg((remoteFilePath == "") ? RemoteRoot : remoteFilePath);
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
        Sleeper::msleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

QString GCDClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    qDebug() << "----- GCDClient::fileGet -----" << uid << remoteFilePath << localFilePath << synchronous;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>( fileGet(nonce, uid, remoteFilePath, -1, synchronous) );

    if (!synchronous) return "";

    // Construct result.
    QString result = fileGetReplySave(reply);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

void GCDClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName)
{
    qDebug() << "----- GCDClient::filePut -----" << uid << localFilePath << remoteParentPath << remoteFileName;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        filePut(nonce, uid, localSourceFile, fileSize, remoteParentPath, remoteFileName, false);
    } else {
        qDebug() << "GCDClient::filePut file " << localFilePath << " can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
    }
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
                if (m_paramMap.contains("refresh_token") && m_paramMap["refresh_token"] != "") {
                    accessTokenPairMap[uid].secret = m_paramMap["refresh_token"];
                }

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

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), result);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filePutReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::filePutReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteParentPath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString remoteFileName = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

    // Close source file.
    if (m_localFileHash.contains(nonce)) {
        QFile *localTargetFile = m_localFileHash[nonce];
        localTargetFile->close();
        m_localFileHash.remove(nonce);
    }

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(reply->readAll()) + ")");
        QString remoteFilePath = sc.property("id").toString();

        QByteArray metadata;
        metadata.append("{");
        metadata.append(" \"title\": \"" + remoteFileName.toUtf8() + "\", ");
        metadata.append(" \"parents\": [{ \"id\": \"" + remoteParentPath.toUtf8() + "\" }] ");
        metadata.append("}");
        qDebug() << "GCDClient::filePutReplyFinished metadata" << metadata;

        reply = patchFile(nonce, uid, remoteFilePath, metadata);
    }

    // REMARK Use QString::fromUtf8() to support unicode text.
    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filePutMultipartReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::filePutMultipartReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Close source file.
    QFile *localTargetFile = m_localFileHash[nonce];
    localTargetFile->close();
    m_localFileHash.remove(nonce);

    // Remove request buffer.
    if (m_bufferHash.contains(nonce)) {
        m_bufferHash[nonce]->close();
        m_bufferHash.remove(nonce);
    }

    // REMARK Use QString::fromUtf8() to support unicode text.
    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

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

    if (reply->error() == QNetworkReply::NoError) {
        emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(m_propertyReplyHash->value(nonce)));
        m_propertyReplyHash->remove(nonce);
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::fileGetResumeReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::fileGetResumeReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetResumeReplySignal(nonce, reply->error(), reply->errorString(), result);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filePutResumeReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QByteArray replyBody = reply->readAll();
    qDebug() << "GCDClient::filePutResumeReplyFinished" << reply << QString(" Error=%1").arg(reply->error()) << "replyBody" << QString::fromUtf8(replyBody);

    if (reply->error() == QNetworkReply::NoError) {
        QString uploadId = reply->header(QNetworkRequest::LocationHeader).toString();

        // Emit signal with upload_location to start uploading.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString("{ \"upload_id\": \"%1\" }").arg(uploadId) );
    } else {
        // REMARK Use QString::fromUtf8() to support unicode text.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(replyBody));
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filePutResumeUploadReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QByteArray replyBody = reply->readAll();
    qDebug() << "GCDClient::filePutResumeUploadReplyFinished" << reply << QString(" Error=%1").arg(reply->error()) << "replyBody" << QString::fromUtf8(replyBody) << "hasRange" << reply->hasRawHeader("Range");

    if (reply->error() == QNetworkReply::NoError) {
        // TODO Get range and check if resume upload is required.
        if (reply->hasRawHeader("Range")) {
            QStringList ranges = QString::fromAscii(reply->rawHeader("Range")).split("-");
            qint64 offset = ranges.at(1).toUInt() + 1;
            qDebug() << "GCDClient::filePutResumeUploadReplyFinished ranges" << ranges << "offset" << offset;

            // Emit signal with offset to resume upload.
            emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString("{ \"offset\": %1 }").arg(offset) );
        } else {
            // Emit signal with successful upload reply which include uploaded file size.
            emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(replyBody));
        }
    } else {
        // REMARK Use QString::fromUtf8() to support unicode text.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(replyBody));
    }

    // Close source file.
    QFile *localTargetFile = m_localFileHash[nonce];
    localTargetFile->close();
    m_localFileHash.remove(nonce);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filePutResumeStatusReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QByteArray replyBody = reply->readAll();
    qDebug() << "GCDClient::filePutResumeStatusReplyFinished" << reply << QString(" Error=%1").arg(reply->error()) << "replyBody" << QString::fromUtf8(replyBody);

    if (reply->error() == QNetworkReply::NoError) {
        qint64 offset = 0;
        if (reply->hasRawHeader("Range")) {
            QStringList ranges = QString::fromAscii(reply->rawHeader("Range")).split("-");
            offset = ranges.at(1).toUInt() + 1;
            qDebug() << "GCDClient::filePutResumeStatusReplyFinished ranges" << ranges << "offset" << offset;
        }

        // Emit signal with offset to resume upload.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString("{ \"offset\": %1 }").arg(offset) );
    } else {
        // REMARK Use QString::fromUtf8() to support unicode text.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(replyBody));
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}
