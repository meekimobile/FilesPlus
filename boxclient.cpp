#include "boxclient.h"
#include <QtGlobal>
#include <QApplication>
#include <QCoreApplication>
#include <QScriptEngine>

const QString BoxClient::consumerKey = "6aer9l7aghmaivzvhgznxrc2x84gml9w";
const QString BoxClient::consumerSecret = "TaDx246tayNNmgK43v8y1scnASJZJaAv";
const QString BoxClient::RemoteRoot = "0";

const QString BoxClient::authorizeURI = "https://www.box.com/api/oauth2/authorize";
const QString BoxClient::accessTokenURI = "https://www.box.com/api/oauth2/token";
const QString BoxClient::accountInfoURI = "https://api.box.com/2.0/users/me";
const QString BoxClient::quotaURI = "https://api.box.com/2.0/users/me";

const QString BoxClient::fileGetURI = "https://api.box.com/2.0/files/%1/content";
const QString BoxClient::filePutURI = "https://upload.box.com/api/2.0/files/content"; // POST multipart
const QString BoxClient::filePutRevURI = "https://upload.box.com/api/2.0/files/%1/content"; // POST multipart
const QString BoxClient::filesURI = "https://api.box.com/2.0/folders/%1/items"; // GET
const QString BoxClient::propertyURI = "https://api.box.com/2.0/%1/%2"; // GET %1=files/folders %2=ID
const QString BoxClient::createFolderURI = "https://api.box.com/2.0/folders"; // POST with json with name = new folder name, parent object with id.
const QString BoxClient::copyFileURI = "https://api.box.com/2.0/%1/%2/copy"; // POST %1=files/folders %2=ID with parent destination folder ID in content.
const QString BoxClient::deleteFileURI = "https://api.box.com/2.0/%1/%2"; // DELETE %1=files/folders %2=ID
const QString BoxClient::sharesURI = "https://api.box.com/2.0/%1/%2"; // PUT %1=files/folders %2=ID with {"shared_link": {"access": "open"}}
const QString BoxClient::patchURI = "https://api.box.com/2.0/%1/%2"; // PUT %1=files/folders %2=ID (also use for move, rename)
const QString BoxClient::thumbnailURI = "https://api.box.com/2.0/files/%1/thumbnail.%2";

const qint64 BoxClient::DefaultChunkSize = 4194304; // 4MB

BoxClient::BoxClient(QObject *parent) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();

    m_propertyReplyHash = new QHash<QString, QByteArray>;
    m_filesReplyHash = new QHash<QString, QByteArray>;
}

BoxClient::~BoxClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_propertyReplyHash = 0;
    m_filesReplyHash = 0;
}

QString BoxClient::createTimestamp() {
    qint64 seconds = QDateTime::currentMSecsSinceEpoch() / 1000;

    return QString("%1").arg(seconds);
}

QString BoxClient::createNormalizedQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(QUrl::toPercentEncoding(key)).append("=").append(QUrl::toPercentEncoding(sortMap[key]));
    }

    return queryString;
}

QString BoxClient::encodeURI(const QString uri) {
    // Example: https://api.dropbox.com/1/metadata/sandbox/C/B/NES/Solomon's Key (E) [!].nes
    // All non-alphanumeric except : and / must be encoded.
    return QUrl::toPercentEncoding(uri, ":/");
}

void BoxClient::authorize(QString nonce, QString hostname)
{
    qDebug() << "----- BoxClient::authorize -----" << nonce << hostname;

    QString queryString;
    queryString.append("client_id=" + consumerKey);
    queryString.append("&response_type=code");
    queryString.append("&state=authenticated");

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, objectName());
}

void BoxClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- BoxClient::accessToken -----" << "pin" << pin;

    refreshTokenUid = "";

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["code"] = pin;
    sortMap["grant_type"] = "authorization_code";

    QString queryString = createNormalizedQueryString(sortMap);
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

void BoxClient::refreshToken(QString nonce, QString uid)
{
    qDebug() << "----- BoxClient::refreshToken -----" << "uid" << uid;

    refreshTokenUid = uid;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["redirect_uri"] = "https://login.live.com/oauth20_desktop.srf";
    sortMap["refresh_token"] = accessTokenPairMap[uid].secret;
    sortMap["grant_type"] = "refresh_token";

    QString queryString = createNormalizedQueryString(sortMap);
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

void BoxClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- BoxClient::accountInfo ----- uid" << uid;

    QString accessToken;
    if (uid == "") {
        // After accessToken, then uses temp access token..
        accessToken = m_paramMap["access_token"];
    } else {
        accessToken = accessTokenPairMap[uid].token;
    }

    QString uri = accountInfoURI;
    qDebug() << "BoxClient::accountInfo uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessToken).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

void BoxClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- BoxClient::quota ----- uid" << uid;

    QString uri = quotaURI;
    qDebug() << "BoxClient::quota uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

QString BoxClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    qDebug() << "----- BoxClient::fileGet -----" << uid << remoteFilePath << localFilePath << synchronous;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>( fileGet(nonce, uid, remoteFilePath, -1, synchronous) );

    if (!synchronous) {
        return "";
    }

    // Construct result.
    QString result = fileGetReplySave(reply);

    // scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

void BoxClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName) {
    qDebug() << "----- BoxClient::filePut -----" << localFilePath << "to" << remoteParentPath << remoteFileName;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        filePut(nonce, uid, localSourceFile, fileSize, remoteParentPath, remoteFileName, false);
    } else {
        qDebug() << "BoxClient::filePut file " << localFilePath << " can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
    }
}

void BoxClient::metadata(QString nonce, QString uid, QString remoteFilePath) {
    qDebug() << "----- BoxClient::metadata -----" << nonce << uid << remoteFilePath;

    if (remoteFilePath.isEmpty()) {
        emit metadataReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    property(nonce, uid, remoteFilePath, false, "metadata");
    files(nonce, uid, remoteFilePath, false, "metadata");
}

void BoxClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- BoxClient::browse -----" << nonce << uid << remoteFilePath;

    // Default remoteFilePath if it's empty from DrivePage.
    remoteFilePath = (remoteFilePath == "") ? RemoteRoot : remoteFilePath;

    property(nonce, uid, remoteFilePath, false, "browse");
    files(nonce, uid, remoteFilePath, false, "browse");
}

QNetworkReply * BoxClient::files(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- BoxClient::files -----" << nonce << uid << remoteFilePath << synchronous << callback;

    QString uri = filesURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    uri += "?fields=type,id,etag,sha1,name,modified_at,size,shared_link,parent,item_status";
    qDebug() << "BoxClient::files" << nonce << "uri" << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filesReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(callback));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(uid));
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

QNetworkReply * BoxClient::property(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- BoxClient::property -----" << nonce << uid << remoteFilePath << synchronous << callback;

    QString uri = propertyURI.arg("folders").arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "BoxClient::property" << nonce << "uri" << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(propertyReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(callback));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(uid));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);

    // Return if asynchronous.
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

void BoxClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteParentPath, newRemoteFolderName, false);
}

QString BoxClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- BoxClient::createFolder -----" << nonce << uid << remoteParentPath << newRemoteFolderName << synchronous;

    if (remoteParentPath.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
    }

    QString uri = createFolderURI.arg(remoteParentPath);
    qDebug() << "BoxClient::createFolder uri " << uri;

    QByteArray postData;
    postData.append("{ \"name\": \"");
    postData.append(newRemoteFolderName.toUtf8());
    postData.append("\" }");
    qDebug() << "BoxClient::createFolder postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteParentPath));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(newRemoteFolderName));
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

    // Emit signal and return replyBody.
    return createFolderReplyFinished(reply, synchronous);
}

void BoxClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- BoxClient::moveFile -----" << nonce << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

//    QRegExp rx("(file\.|folder\.)(" + uid + ")(.*)");
//    if (rx.exactMatch(targetRemoteParentPath)) {
//        // Match as remote ID.
//    }

    if (newRemoteFileName != "" && targetRemoteParentPath == "") {
        // Proceed renaming.
        renameFile(nonce, uid, remoteFilePath, newRemoteFileName);
        return;
    }

    QString uri = patchURI.arg(remoteFilePath);
    qDebug() << "BoxClient::moveFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"destination\": \"");
    postData.append(targetRemoteParentPath.toUtf8());
    postData.append("\" }");
    qDebug() << "BoxClient::moveFile postData" << postData;

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
        QNetworkReply *reply = manager->sendCustomRequest(req, "MOVE", m_bufferHash[nonce]);
    }
}

void BoxClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- BoxClient::copyFile -----" << nonce << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

    QString uri = copyFileURI.arg(remoteFilePath);
    qDebug() << "BoxClient::copyFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"destination\": \"");
    postData.append(targetRemoteParentPath.toUtf8());
    postData.append("\" }");
    qDebug() << "BoxClient::copyFile postData" << postData;

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
        QNetworkReply *reply = manager->sendCustomRequest(req, "COPY", m_bufferHash[nonce]);
    }
}

void BoxClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    deleteFile(nonce, uid, remoteFilePath, false);
}

QString BoxClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- BoxClient::deleteFile -----" << nonce << uid << remoteFilePath << synchronous;

    QString uri = deleteFileURI.arg(remoteFilePath);
//    uri = encodeURI(uri);
    qDebug() << "BoxClient::deleteFile uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deleteFileReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
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

void BoxClient::renameFile(QString nonce, QString uid, QString remoteFilePath, QString newName)
{
    qDebug() << "----- BoxClient::renameFile -----" << nonce << uid << remoteFilePath << newName;

    QString uri = patchURI.arg(remoteFilePath);
    qDebug() << "BoxClient::renameFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"name\": \"");
    postData.append(newName.toUtf8());
    postData.append("\" }");
    qDebug() << "BoxClient::renameFile postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->put(req, postData);
}

QIODevice *BoxClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- BoxClient::fileGet -----" << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    QString uri = fileGetURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "BoxClient::fileGet" << nonce << "uri" << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "BoxClient::fileGet rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    // Redirect to download URL.
    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (synchronous) {
    }

    return reply;
}

QString BoxClient::fileGetReplySave(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "BoxClient::fileGetReplySave reply bytesAvailable" << reply->bytesAvailable();

        // Find offset.
        qint64 offset = getOffsetFromRange(QString::fromAscii(reply->request().rawHeader("Range")));
        qDebug() << "BoxClient::fileGetReplySave reply request offset" << offset;

        // Stream replyBody to a file on localPath.
        qint64 totalBytes = reply->bytesAvailable();
        qint64 writtenBytes = 0;
        char buf[FileWriteBufferSize];
        QFile *localTargetFile = m_localFileHash[nonce];
        if (localTargetFile->open(QIODevice::ReadWrite)) {
            // Issue: Writing to file with QDataStream << QByteArray will automatically prepend with 4-bytes prefix(size).
            // Solution: Use QIODevice to write directly.

            // Move to offset.
            localTargetFile->seek(offset);

            // Read first buffer.
            qint64 c = reply->read(buf, sizeof(buf));
            while (c > 0) {
                writtenBytes += localTargetFile->write(buf, c);

                // Tell event loop to process event before it will process time consuming task.
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

                // Read next buffer.
                c = reply->read(buf, sizeof(buf));
            }
        }

        qDebug() << "BoxClient::fileGetReplySave reply writtenBytes" << writtenBytes << "totalBytes" << totalBytes << "localTargetFile size" << localTargetFile->size();

        // Close target file.
        localTargetFile->flush();
        localTargetFile->close();

        QString propertyReplyBody = QString::fromUtf8(m_propertyReplyHash->value(nonce));
        qDebug() << "BoxClient::fileGetReplySave propertyReplyBody" << propertyReplyBody;

        // Return common json.
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + propertyReplyBody + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        result = stringifyScriptValue(engine, parsedObj);
    } else {
        qDebug() << "BoxClient::fileGetReplySave nonce" << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Remove once used.
    m_localFileHash.remove(nonce);
    m_propertyReplyHash->remove(nonce);

    return result;
}

QNetworkReply *BoxClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous, QString sourceFilePath, QDateTime sourceFileTimestamp)
{
    qDebug() << "----- BoxClient::filePut -----" << nonce << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal << "sourceFilePath" << sourceFilePath << "sourceFileTimestamp" << sourceFileTimestamp;

    QString uri = filePutURI;
    uri = encodeURI(uri);
    qDebug() << "BoxClient::filePut uri " << uri;

    QNetworkReply *reply = 0;
    qint64 fileSize = source->size();
    QString contentType = getContentType(remoteFileName);
    qDebug() << "BoxClient::filePut remoteFileName" << remoteFileName << "contentType" << contentType << "fileSize" << fileSize;

    // Requires to submit job with multipart.
    QString boundary = "----------" + nonce;
    QString CRLF = "\r\n";

    // Insert buffer to hash.
    m_bufferHash.insert(nonce, new QBuffer());
    m_bufferHash[nonce]->open(QIODevice::WriteOnly);
    m_bufferHash[nonce]->write(QString("--" + boundary + CRLF).toAscii());
    m_bufferHash[nonce]->write(QString("Content-Disposition: form-data; name=\"filename\"; filename=\"%1\"" + CRLF).arg(encodeURI(remoteFileName)).toAscii());
    m_bufferHash[nonce]->write(QString("Content-Type: " + contentType + CRLF).toAscii());
    m_bufferHash[nonce]->write(QString(CRLF).toAscii());
    m_bufferHash[nonce]->write(source->readAll());
    m_bufferHash[nonce]->write(QString(CRLF).toAscii());
    m_bufferHash[nonce]->write(QString("--" + boundary + CRLF).toAscii());
    m_bufferHash[nonce]->write(QString("Content-Disposition: form-data; name=\"parent_id\"" + CRLF).toAscii());
    m_bufferHash[nonce]->write(QString(CRLF).toAscii());
    m_bufferHash[nonce]->write(QString(remoteParentPath).toAscii());
    m_bufferHash[nonce]->write(QString(CRLF).toAscii());
    m_bufferHash[nonce]->write(QString("--" + boundary + "--" + CRLF).toAscii());
    m_bufferHash[nonce]->close();

    if (m_bufferHash[nonce]->open(QIODevice::ReadOnly)) {
        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        if (!synchronous) {
            connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
        }
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
        req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
        req.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=\"" + boundary + "\"");
        req.setHeader(QNetworkRequest::ContentLengthHeader, m_bufferHash[nonce]->size());
        reply = manager->post(req, m_bufferHash[nonce]->readAll());
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));

        // Store reply for further usage.
        m_replyHash->insert(nonce, reply);

        while (synchronous && !reply->isFinished()) {
            QApplication::processEvents(QEventLoop::AllEvents, 100);
            Sleeper::msleep(100);
        }
    }

    if (synchronous) {
        // Remove request buffer.
        if (m_bufferHash.contains(nonce)) {
            m_bufferHash[nonce]->deleteLater();
            m_bufferHash.remove(nonce);
        }
        // Remove reply from hash.
        // NOTE reply and its manager will be deleted later by caller (CloudDriveModel.migrateFile_Block()).
        m_replyHash->remove(nonce);
    }

    return reply;
}

QIODevice *BoxClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- BoxClient::fileGetResume -----" << nonce << uid << remoteFilePath << "to" << localFilePath << "offset" << offset;

    // remoteFilePath is not a URL. Procees getting property to get downloadUrl.
    QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "fileGetResume");
    if (propertyReply->error() == QNetworkReply::NoError) {
        // For further using in fileGetReplyFinished.
        m_propertyReplyHash->insert(nonce, propertyReply->readAll());
        propertyReply->deleteLater();
    } else {
        emit fileGetReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
        propertyReply->deleteLater();
        return 0;
    }

    QString uri = fileGetURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "BoxClient::fileGetResume uri " << uri;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetResumeReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "BoxClient::fileGetResume rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    return reply;
}

QString BoxClient::getRemoteRoot(QString uid)
{
    return RemoteRoot;
}

bool BoxClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize == -1 || fileSize >= getChunkSize());
}

bool BoxClient::isViewable()
{
    return true;
}

void BoxClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- BoxClient::shareFile -----" << nonce << uid << remoteFilePath;

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = sharesURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "BoxClient::shareFile uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

void BoxClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::accessTokenReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Load response parameters into map.
    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        m_paramMap["access_token"] = sc.property("access_token").toString();
        m_paramMap["refresh_token"] = sc.property("refresh_token").toString();
        qDebug() << "BoxClient::accessTokenReplyFinished m_paramMap " << m_paramMap;

        if (refreshTokenUid != "") {
            // Updates for refreshToken.
            accessTokenPairMap[refreshTokenUid].token = sc.property("access_token").toString();
            accessTokenPairMap[refreshTokenUid].secret = sc.property("refresh_token").toString();
            qDebug() << "BoxClient::accessTokenReplyFinished accessTokenPairMap" << accessTokenPairMap;

            // Reset refreshTokenUid.
            refreshTokenUid = "";
        }

        // Get email from accountInfo will be requested by CloudDriveModel.accessTokenReplyFilter().
    }

    emit accessTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::accountInfoReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        QString uid = sc.property("id").toString();
        QString name = sc.property("name").toString();
        QString email = sc.property("login").toString();

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

//        qDebug() << "BoxClient::accountInfoReplyFinished accessTokenPairMap" << accessTokenPairMap;

        // Save account after got id and email.
        saveAccessPairMap();
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::quotaReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::quotaReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString replyBody = QString::fromUtf8(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
        qint64 sharedValue = 0;
        qint64 normalValue = jsonObj.property("space_used").toInteger();
        qint64 quotaValue = jsonObj.property("space_amount").toInteger();

        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, normalValue, sharedValue, quotaValue);
    } else {
        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, 0, 0, -1);
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::fileGetReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::fileGetReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Detect redirection.
    QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!possibleRedirectUrl.toUrl().isEmpty()) {
        qDebug() << "BoxClient::fileGetReplyFinished redirectUrl" << possibleRedirectUrl.toUrl();

        QNetworkRequest redirectedRequest = reply->request();
        redirectedRequest.setUrl(possibleRedirectUrl.toUrl());
        reply = reply->manager()->get(redirectedRequest);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

        // Store reply for further usage.
        m_replyHash->insert(nonce, reply);

        return;
    }

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), result);

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::filePutReplyFinished(QNetworkReply *reply) {
    qDebug() << "BoxClient::filePutReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    // Close source file.
    if (m_localFileHash.contains(nonce)) {
        QFile *localTargetFile = m_localFileHash[nonce];
        localTargetFile->close();
        m_localFileHash.remove(nonce);
    }

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "BoxClient::filePutReplyFinished replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + replyBody + ")");
        if (sc.property("total_count").toInteger() > 0) {
            QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, sc.property("entries").property(0));
            replyBody = stringifyScriptValue(engine, parsedObj);
        }
    }

    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Remove request buffer.
    if (m_bufferHash.contains(nonce)) {
        m_bufferHash[nonce]->deleteLater();
        m_bufferHash.remove(nonce);
    }

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::metadataReplyFinished(QNetworkReply *reply) {
    qDebug() << "BoxClient::metadataReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::browseReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::browseReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::mergePropertyAndFilesJson(QString nonce, QString callback, QString uid)
{
    if (m_propertyReplyHash->contains(nonce) && m_filesReplyHash->contains(nonce)) {
        QScriptEngine engine;
        engine.setProperty("uid", QVariant(uid));
        QScriptValue mergedObj;
        QScriptValue propertyObj;
        QScriptValue filesObj;
        qDebug() << "BoxClient::mergePropertyAndFilesJson propertyJson" << QString::fromUtf8(m_propertyReplyHash->value(nonce));
        qDebug() << "BoxClient::mergePropertyAndFilesJson filesJson" << QString::fromUtf8(m_filesReplyHash->value(nonce));
        propertyObj = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
        filesObj = engine.evaluate("(" + QString::fromUtf8(m_filesReplyHash->value(nonce)) + ")");

        mergedObj = parseCommonPropertyScriptValue(engine, propertyObj);
        mergedObj.setProperty("children", engine.newArray());
        int contentsCount = filesObj.property("total_count").toInteger();
        for (int i = 0; i < contentsCount; i++) {
            mergedObj.property("children").setProperty(i, parseCommonPropertyScriptValue(engine, filesObj.property("entries").property(i)));
        }

        QString replyBody = stringifyScriptValue(engine, mergedObj);

        // Remove once used.
        m_propertyReplyHash->remove(nonce);
        m_filesReplyHash->remove(nonce);

        if (callback == "browse") {
            emit browseReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else {
            qDebug() << "BoxClient::mergePropertyAndFilesJson invalid callback" << callback;
        }
    }
}

void BoxClient::propertyReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::propertyReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    if (reply->error() == QNetworkReply::NoError) {
        m_propertyReplyHash->insert(nonce, reply->readAll());

        // Merge and emit signal if both property and files are available.
        mergePropertyAndFilesJson(nonce, callback, uid);
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
            qDebug() << "BoxClient::propertyReplyFinished invalid callback" << callback;
        }
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::filesReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::filesReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    if (reply->error() == QNetworkReply::NoError) {
        m_filesReplyHash->insert(nonce, reply->readAll());

        // Merge and emit signal if both property and files are available.
        mergePropertyAndFilesJson(nonce, callback, uid);
    } else if (reply->error() == QNetworkReply::UnknownContentError) {
        qDebug() << "BoxClient::filesReplyFinished suppress error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        m_filesReplyHash->insert(nonce, QByteArray("{}"));

        // Merge and emit signal if both property and files are available.
        mergePropertyAndFilesJson(nonce, callback, uid);
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
            qDebug() << "BoxClient::filesReplyFinished invalid callback" << callback;
        }
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString BoxClient::createFolderReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteParentPath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString newRemoteFileName = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

    qDebug() << "BoxClient::createFolderReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "BoxClient::createFolderReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    } else if (reply->error() == QNetworkReply::UnknownContentError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        if (jsonObj.property("error").property("code").toString() == "resource_already_exists") {
            // Get existing folder's property.
            reply = files(nonce, uid, remoteParentPath, true);
            replyBody = QString::fromUtf8(reply->readAll());
            qDebug() << "BoxClient::createFolderReplyFinished" << nonce << "files replyBody" << replyBody;
            if (reply->error() == QNetworkReply::NoError) {
                QScriptEngine engine;
                QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
                int contentsCount = jsonObj.property("data").toVariant().toList().length();
                for (int i = 0; i < contentsCount; i++) {
                    QScriptValue item = jsonObj.property("data").property(i);
                    if (item.property("name").toString() == newRemoteFileName) {
                        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, item);
                        replyBody = stringifyScriptValue(engine, parsedObj);
                        break;
                    }
                }
            }
        }
    }

    // Emit signal only for asynchronous request. To avoid invoking CloudDriveModel.createFolderReplyFilter() as it's not required. And also avoid invoking jobDone() which causes issue #FP20130232.
    if (!synchronous) emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

void BoxClient::moveFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::moveFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "BoxClient::moveFileReplyFinished replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit moveFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Remove request buffer.
    if (m_bufferHash.contains(nonce)) {
        m_bufferHash[nonce]->close();
        m_bufferHash.remove(nonce);
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::copyFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::copyFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "BoxClient::copyFileReplyFinished replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Remove request buffer.
    m_bufferHash[nonce]->close();
    m_bufferHash.remove(nonce);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::deleteFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "BoxClient::deleteFileReplyFinished replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::shareFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::shareFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString replyBody = QString::fromUtf8(reply->readAll());
    QScriptEngine engine;
    QScriptValue sc;
    QString url = "";
    int expires = 0;

    if (reply->error() == QNetworkReply::NoError) {
        sc = engine.evaluate("(" + replyBody + ")");
        url = sc.property("link").toString();
        expires = -1;
    }

    emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, url, expires);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void BoxClient::fileGetResumeReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::fileGetResumeReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Detect redirection.
    QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!possibleRedirectUrl.toUrl().isEmpty()) {
        qDebug() << "BoxClient::fileGetResumeReplyFinished redirectUrl" << possibleRedirectUrl.toUrl();

        QNetworkRequest redirectedRequest = reply->request();
        redirectedRequest.setUrl(possibleRedirectUrl.toUrl());
        reply = reply->manager()->get(redirectedRequest);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

        // Store reply for further usage.
        m_replyHash->insert(nonce, reply);

        return;
    }

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetResumeReplySignal(nonce, reply->error(), reply->errorString(), result);

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QScriptValue BoxClient::parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj)
{
    QScriptValue parsedObj = engine.newObject();

    bool isDir = jsonObj.property("type").toString().indexOf(QRegExp("folder|album")) == 0;
    QString hash = isDir ? jsonObj.property("etag").toString() : jsonObj.property("sha1").toString();

    parsedObj.setProperty("name", jsonObj.property("name"));
    parsedObj.setProperty("absolutePath", jsonObj.property("id"));
    parsedObj.setProperty("parentPath", !jsonObj.property("parent").isNull() ? jsonObj.property("parent").property("id") : QScriptValue(""));
    parsedObj.setProperty("size", jsonObj.property("size"));
    parsedObj.setProperty("isDeleted", QScriptValue(jsonObj.property("item_status").toString() == "trashed"));
    parsedObj.setProperty("isDir", QScriptValue(isDir));
    parsedObj.setProperty("lastModified", jsonObj.property("modified_at"));
    parsedObj.setProperty("hash", QScriptValue(hash));
    parsedObj.setProperty("source", jsonObj.property("source"));
    parsedObj.setProperty("alternative", jsonObj.property("alternative"));
    parsedObj.setProperty("thumbnail", jsonObj.property("thumbnail"));
    parsedObj.setProperty("thumbnail128", jsonObj.property("thumbnail128Url"));
    parsedObj.setProperty("preview", jsonObj.property("preview"));
    parsedObj.setProperty("shareLink", jsonObj.property("shared_link"));
    parsedObj.setProperty("fileType", QScriptValue(getFileType(jsonObj.property("name").toString())));

    return parsedObj;
}

qint64 BoxClient::getChunkSize()
{
    return m_settings.value(QString("%1.resumable.chunksize").arg(objectName()), DefaultChunkSize).toInt();
}
