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
const QString BoxClient::deleteFileURI = "https://api.box.com/2.0/%1/%2"; // DELETE %1=files/folders %2=ID
const QString BoxClient::copyFileURI = "https://api.box.com/2.0/%1/%2/copy"; // POST %1=files/folders %2=ID with parent destination folder ID in content.
const QString BoxClient::sharesURI = "https://api.box.com/2.0/%1/%2"; // PUT %1=files/folders %2=ID with {"shared_link": {"access": "open"}} (also use for media)
const QString BoxClient::patchURI = "https://api.box.com/2.0/%1/%2"; // PUT %1=files/folders %2=ID (also use for move, rename)
const QString BoxClient::deltaURI = "https://api.box.com/2.0/events"; // GET with stream_position
const QString BoxClient::thumbnailURI = "https://api.box.com/2.0/files/%1/thumbnail.%2";

const qint64 BoxClient::DefaultChunkSize = 4194304; // 4MB
const qint64 BoxClient::DefaultMaxUploadSize = 20971520; // 20MB which support ~20 sec(s) video clip.

BoxClient::BoxClient(QObject *parent) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();

    m_propertyReplyHash = new QHash<QString, QByteArray>;
    m_filesReplyHash = new QHash<QString, QByteArray>;
    m_isDirHash = new QHash<QString, bool>;
}

BoxClient::~BoxClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_propertyReplyHash = 0;
    m_filesReplyHash = 0;
    m_isDirHash = 0;
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
    qDebug() << "----- BoxClient::refreshToken -----" << nonce << uid << accessTokenPairMap[uid].secret;

    refreshTokenUid = uid;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
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
    qDebug() << "----- BoxClient::fileGet -----" << nonce << uid << remoteFilePath << localFilePath << synchronous;

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

    // Check if remoteFilePath is a directory, then proceed getting property.
    bool isDir = false;
    if (m_isDirHash->contains(remoteFilePath)) {
        isDir = m_isDirHash->value(remoteFilePath);
        // Proceed getting property.
        property(nonce, uid, remoteFilePath, isDir, false, "metadata");
    } else {
        // Try getting property as dir.
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, true, "metadata");
        if (propertyReply->error() == QNetworkReply::NoError) {
            // Proceed getting property as dir.
            isDir = true;
            propertyReplyFinished(propertyReply);
        } else {
            // Proceed getting property as file.
            property(nonce, uid, remoteFilePath, false, false, "metadata");
        }
    }

    // Get files if remoteFilePath is dir. Otherwise, proceed with empty JSON object.
    if (isDir) {
        files(nonce, uid, remoteFilePath, 0, false, "metadata");
    } else {
        m_filesReplyHash->insert(nonce, QString("{}").toAscii());
        mergePropertyAndFilesJson(nonce, "metadata", uid);
    }
}

void BoxClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- BoxClient::browse -----" << nonce << uid << remoteFilePath;

    // Default remoteFilePath if it's empty from DrivePage.
    remoteFilePath = (remoteFilePath == "") ? RemoteRoot : remoteFilePath;

    // Check if remoteFilePath is a directory, then proceed getting property.
    bool isDir = false;
    if (m_isDirHash->contains(remoteFilePath)) {
        isDir = m_isDirHash->value(remoteFilePath);
        // Proceed getting property.
        property(nonce, uid, remoteFilePath, isDir, false, "browse");
    } else {
        // Try getting property as dir.
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, true, "browse");
        if (propertyReply->error() == QNetworkReply::NoError) {
            // Proceed getting property as dir.
            isDir = true;
            propertyReplyFinished(propertyReply);
        } else {
            // Proceed getting property as file.
            property(nonce, uid, remoteFilePath, false, false, "browse");
        }
    }

    // Get files if remoteFilePath is dir. Otherwise, proceed with empty JSON object.
    if (isDir) {
        files(nonce, uid, remoteFilePath, 0, false, "browse");
    } else {
        m_filesReplyHash->insert(nonce, QString("{}").toAscii());
        mergePropertyAndFilesJson(nonce, "browse", uid);
    }
}

QNetworkReply * BoxClient::files(QString nonce, QString uid, QString remoteFilePath, int offset, bool synchronous, QString callback)
{
    qDebug() << "----- BoxClient::files -----" << nonce << uid << remoteFilePath << offset << synchronous << callback;

    int limit = 100;
    if (callback == "metadata") {
        limit = m_settings.value("BoxClient.metadata.files.limit", 500).toInt();
    } else if (callback == "browse") {
        limit = m_settings.value("BoxClient.browse.files.limit", 500).toInt();
    }

    QString uri = filesURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    uri += "?fields=type,id,etag,sha1,name,modified_at,size,shared_link,parent,item_status";
    uri += QString("&offset=%1&limit=%2").arg(offset).arg(limit);
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
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(remoteFilePath));
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

QNetworkReply * BoxClient::property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous, QString callback)
{
    qDebug() << "----- BoxClient::property -----" << nonce << uid << remoteFilePath << isDir << synchronous << callback;

    QString uri = propertyURI.arg(isDir ? "folders" : "files").arg(remoteFilePath);
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
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(remoteFilePath));
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

    QString uri = createFolderURI;
    qDebug() << "BoxClient::createFolder uri" << uri;

    QString postString = QString("{ \"name\":\"%1\", \"parent\":{ \"id\":\"%2\" } }").arg(newRemoteFolderName).arg(remoteParentPath);
    qDebug() << "BoxClient::createFolder postString" << postString;

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
    QNetworkReply *reply = manager->post(req, postString.toUtf8());

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

    if (newRemoteFileName != "" && targetRemoteParentPath == "") {
        // Proceed renaming.
        renameFile(nonce, uid, remoteFilePath, newRemoteFileName);
        return;
    }

    // Check if remoteFilePath is directory.
    bool isDir = isRemoteDir(nonce, uid, remoteFilePath);

    QString uri = patchURI.arg(isDir ? "folders" : "files").arg(remoteFilePath);
    qDebug() << "BoxClient::moveFile uri " << uri;

    QString postString = QString("{ \"name\":\"%1\", \"parent\":{ \"id\":\"%2\" } }").arg(newRemoteFileName).arg(targetRemoteParentPath);
    qDebug() << "BoxClient::moveFile postString" << postString;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->put(req, postString.toUtf8());
}

void BoxClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- BoxClient::copyFile -----" << nonce << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

    // Check if remoteFilePath is directory.
    bool isDir = isRemoteDir(nonce, uid, remoteFilePath);

    QString uri = copyFileURI.arg(isDir ? "folders" : "files").arg(remoteFilePath);
    qDebug() << "BoxClient::copyFile uri " << uri;

    QString postString = QString("{ \"name\":\"%1\", \"parent\":{ \"id\":\"%2\" } }").arg(newRemoteFileName).arg(targetRemoteParentPath);
    qDebug() << "BoxClient::copyFile postString" << postString;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(copyFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->post(req, postString.toUtf8());
}

bool BoxClient::isRemoteDir(QString nonce, QString uid, QString remoteFilePath)
{
    bool isDir = false;
    if (m_isDirHash->contains(remoteFilePath)) {
        isDir = m_isDirHash->value(remoteFilePath);
    } else {
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, true, "isDir");
        if (propertyReply->error() == QNetworkReply::NoError) {
            isDir = true;
        } else {
            qDebug() << "BoxClient::isDir" << nonce << uid << remoteFilePath << "propertyReply->error()" << propertyReply->error() << "remoteFilePath is not a directory.";
        }
        propertyReply->deleteLater();
        propertyReply->manager()->deleteLater();
    }

    return isDir;
}

QString BoxClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- BoxClient::deleteFile -----" << nonce << uid << remoteFilePath << synchronous;

    // Check if remoteFilePath is directory.
    bool isDir = isRemoteDir(nonce, uid, remoteFilePath);

    QString uri = deleteFileURI.arg(isDir ? "folders" : "files").arg(remoteFilePath);
    uri += "?recursive=true";
    qDebug() << "BoxClient::deleteFile" << nonce << "uri" << uri;

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

    // Check if remoteFilePath is directory.
    bool isDir = isRemoteDir(nonce, uid, remoteFilePath);

    QString uri = patchURI.arg(isDir ? "folders" : "files").arg(remoteFilePath);
    qDebug() << "BoxClient::renameFile uri " << uri;

    QString postString = QString("{ \"name\":\"%1\" }").arg(newName);
    qDebug() << "BoxClient::renameFile postString" << postString;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->put(req, postString.toUtf8());
}

QIODevice *BoxClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- BoxClient::fileGet -----" << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    // Get property for using in fileGetReplyFinished().
    if (!synchronous) {
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, false, true, "fileGet");
        if (propertyReply->error() == QNetworkReply::NoError) {
            // For further using in fileGetReplyFinished.
            m_propertyReplyHash->insert(nonce, propertyReply->readAll());
            propertyReply->deleteLater();
        } else {
            if (!synchronous) {
                emit fileGetReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
                propertyReply->deleteLater();
                return 0;
            } else {
                return propertyReply;
            }
        }
    }

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
    req.setRawHeader("Accept-Encoding", "gzip, deflate");
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
        QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if(!possibleRedirectUrl.toUrl().isEmpty()) {
            qDebug() << "BoxClient::fileGet redirectUrl" << possibleRedirectUrl.toUrl();

            QNetworkRequest redirectedRequest = reply->request();
            redirectedRequest.setUrl(possibleRedirectUrl.toUrl());

            // Delete original reply (which is in m_replyHash).
            m_replyHash->remove(nonce);
            reply->deleteLater();

            reply = reply->manager()->get(redirectedRequest);
            QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
            connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

            // Store reply for further usage.
            m_replyHash->insert(nonce, reply);

            while (!reply->isFinished()) {
                QApplication::processEvents(QEventLoop::AllEvents, 100);
                Sleeper::msleep(100);
            }
        }
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

QNetworkReply *BoxClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- BoxClient::filePut -----" << nonce << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    qint64 maxFileSize = m_settings.value(QString("%1.maxUploadFileSize").arg(objectName()), DefaultMaxUploadSize).toULongLong();
    if (bytesTotal > maxFileSize) {
        emit filePutReplySignal(nonce, QNetworkReply::UnknownContentError,
                                tr("File size is too large (> %n MB). Please upload by using BOX web.", "", qRound(maxFileSize / 1048576)), "");

        // Close source file.
        if (m_localFileHash.contains(nonce)) {
            QFile *localTargetFile = m_localFileHash[nonce];
            localTargetFile->close();
            m_localFileHash.remove(nonce);
        }

        return 0;
    }

    QString uri = filePutURI;
    uri = encodeURI(uri);
    qDebug() << "BoxClient::filePut uri" << uri;

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
    m_bufferHash[nonce]->write(QString("Content-Disposition: form-data; name=\"filename\"; filename=\"%1\"" + CRLF).arg(remoteFileName).toUtf8());
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
    QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, false, true, "fileGetResume");
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
    req.setRawHeader("Accept-Encoding", "gzip, deflate");
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

bool BoxClient::isDeltaSupported()
{
    return true;
}

bool BoxClient::isDeltaEnabled(QString uid)
{
    return m_settings.value(QString("%1.%2.delta.enabled").arg(objectName()).arg(uid), QVariant(false)).toBool();
}

bool BoxClient::isViewable()
{
    return true;
}

bool BoxClient::isMediaEnabled(QString uid)
{
    return m_settings.value(QString("%1.%2.media.enabled").arg(objectName()).arg(uid), QVariant(false)).toBool();
}

bool BoxClient::isImageUrlCachable()
{
    return true;
}

QString BoxClient::shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- BoxClient::shareFile -----" << nonce << uid << remoteFilePath << synchronous;

    // Check if remoteFilePath is directory.
    bool isDir = isRemoteDir(nonce, uid, remoteFilePath);

    QString uri = sharesURI.arg(isDir ? "folders" : "files").arg(remoteFilePath);
    qDebug() << "BoxClient::shareFile uri" << uri;

    QString postString = "{\"shared_link\":{\"access\":\"open\"}}";
    qDebug() << "BoxClient::shareFile postString" << postString;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->put(req, postString.toUtf8());

    // Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return shareFileReplyFinished(reply, synchronous);
}

QString BoxClient::delta(QString nonce, QString uid, bool synchronous)
{
    qDebug() << "----- BoxClient::delta -----" << nonce << uid << synchronous;

    QString uri = deltaURI;
    // Skip to last if it's in reset state and doesn't opt to process.
    bool syncOnReset = m_settings.value(QString("%1.%2.syncOnReset").arg(objectName()).arg(uid), QVariant(false)).toBool();
    bool isReset = !m_settings.contains(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid)) || m_settings.value(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(false)).toBool();
    if (isReset && !syncOnReset) {
        // Reset without sync.
        uri += QString("?stream_position=%1").arg("now");
    } else if (m_settings.contains(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid))) {
        QString nextDeltaCursor = m_settings.value(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid)).toString();
        uri += QString("?stream_position=%1").arg(nextDeltaCursor);
    }
    qDebug() << "BoxClient::delta" << nonce << "uri" << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deltaReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);

    // Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return deltaReplyFinished(reply);
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

            // Save tokens.
            saveAccessPairMap();
        }

        // Get email from accountInfo will be requested by CloudDriveModel.accessTokenReplyFilter().
    }

    emit accessTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // Scheduled to delete later.
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

    // Scheduled to delete later.
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

    // Scheduled to delete later.
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

        // Delete existing reply..
        m_replyHash->remove(nonce);
        reply->deleteLater();

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

void BoxClient::mergePropertyAndFilesJson(QString nonce, QString callback, QString uid)
{
    if (m_propertyReplyHash->contains(nonce) && m_filesReplyHash->contains(nonce)) {
        QScriptEngine engine;
        engine.globalObject().setProperty("nonce", QScriptValue(nonce));
        engine.globalObject().setProperty("uid", QScriptValue(uid));
        QScriptValue mergedObj;
        QScriptValue propertyObj;
        QScriptValue filesObj;
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "BoxClient::mergePropertyAndFilesJson" << nonce << "started.";
        if (m_settings.value("Logging.enabled", false).toBool()) {
            qDebug() << "BoxClient::mergePropertyAndFilesJson propertyJson" << QString::fromUtf8(m_propertyReplyHash->value(nonce));
            qDebug() << "BoxClient::mergePropertyAndFilesJson filesJson" << QString::fromUtf8(m_filesReplyHash->value(nonce));
        }
        propertyObj = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
        filesObj = engine.evaluate("(" + QString::fromUtf8(m_filesReplyHash->value(nonce)) + ")");

        mergedObj = parseCommonPropertyScriptValue(engine, propertyObj);
        int totalCount = filesObj.property("total_count").toInteger();
        int offset = filesObj.property("offset").toInteger();
        int limit = filesObj.property("limit").toInteger();
        mergedObj.setProperty("totalCount", QScriptValue(totalCount));
        mergedObj.setProperty("offset", QScriptValue(offset));
        mergedObj.setProperty("limit", QScriptValue(limit));
        mergedObj.setProperty("children", engine.newArray());
        int contentsCount = filesObj.property("entries").property("length").toInteger();
        for (int i = 0; i < contentsCount; i++) {
            QApplication::processEvents();

            mergedObj.property("children").setProperty(i, parseCommonPropertyScriptValue(engine, filesObj.property("entries").property(i)));
        }

        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "BoxClient::mergePropertyAndFilesJson" << nonce << "stringifyScriptValue started.";
        QString replyBody = stringifyScriptValue(engine, mergedObj);
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "BoxClient::mergePropertyAndFilesJson" << nonce << "stringifyScriptValue done." << replyBody.size();

        if (callback == "browse") {
            emit browseReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else {
            qDebug() << "BoxClient::mergePropertyAndFilesJson invalid callback" << callback;
        }

        // Check if it requires next chunk, then proceed with next offset.
        int nextOffset = offset + limit;
        if (nextOffset < totalCount) {
            // Remove only files reply.
            m_filesReplyHash->remove(nonce);

            // Proceed getting next chunk.
            files(nonce, uid, mergedObj.property("absolutePath").toString(), nextOffset, false, callback);
        } else {
            // Remove once used.
            m_propertyReplyHash->remove(nonce);
            m_filesReplyHash->remove(nonce);
        }
    }
}

void BoxClient::propertyReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::propertyReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

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
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

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
            int nextOffset = 0;
            int totalCount = 1;
            bool isFound = false;
            while (!isFound && nextOffset < totalCount) {
                // Delete existing reply.
                reply->deleteLater();
                reply->manager()->deleteLater();

                // Find existing folder ID.
                reply = files(nonce, uid, remoteParentPath, nextOffset, true, "createFolderReplyFinished");
                replyBody = QString::fromUtf8(reply->readAll());
                qDebug() << "BoxClient::createFolderReplyFinished" << nonce << "files replyBody" << replyBody;
                if (reply->error() == QNetworkReply::NoError) {
                    QScriptEngine engine;
                    QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
                    totalCount = jsonObj.property("total_count").toInteger();
                    int offset = jsonObj.property("offset").toInteger();
                    int limit = jsonObj.property("limit").toInteger();
                    nextOffset = offset + limit;

                    int contentsCount = jsonObj.property("entries").property("length").toInteger();
                    for (int i = 0; i < contentsCount; i++) {
                        QApplication::processEvents();

                        QScriptValue item = jsonObj.property("entries").property(i);
                        if (item.property("name").toString() == newRemoteFileName) {
                            QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, item);
                            replyBody = stringifyScriptValue(engine, parsedObj);
                            isFound = true;
                            break;
                        }
                    } // for
                }
            } // while
        }
    }

    // Emit signal only for asynchronous request. To avoid invoking CloudDriveModel.createFolderReplyFilter() as it's not required. And also avoid invoking jobDone() which causes issue #FP20130232.
    if (!synchronous) emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
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

    // Scheduled to delete later.
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

    // Scheduled to delete later.
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

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString BoxClient::shareFileReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "BoxClient::shareFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());
    QScriptEngine engine;
    QScriptValue sc;
    QString url = "";
    int expires = 0;

    if (reply->error() == QNetworkReply::NoError) {
        sc = engine.evaluate("(" + replyBody + ")");
        url = sc.property("shared_link").property("url").toString();
        expires = -1;
    }

    if (!synchronous) emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, url, expires);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return url;
}

QString BoxClient::deltaReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::deltaReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "BoxClient::deltaReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sourceObj = engine.evaluate("(" + replyBody + ")");

        // Check reset state.
        bool syncOnReset = m_settings.value(QString("%1.%2.syncOnReset").arg(objectName()).arg(uid), QVariant(false)).toBool();
        bool isReset = !m_settings.contains(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid)) || m_settings.value(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(false)).toBool();
        if (isReset) {
            qDebug() << "BoxClient::deltaReplyFinished" << nonce << "isReset" << isReset << "set to update state only.";
            m_settings.setValue(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(true));
        }

        // Get entries count.
        int entriesCount = sourceObj.property("entries").property("length").toInteger();
        qDebug() << "BoxClient::deltaReplyFinished entriesCount" << entriesCount;

        // Process sourceObj.
        QScriptValue parsedObj = engine.newObject();
        QScriptValue childrenObj = engine.newArray();
        for (int i = 0; (!isReset || syncOnReset) && i < entriesCount; i++) {
            QScriptValue sourceChildObj = sourceObj.property("entries").property(i);
            QString absolutePath = sourceChildObj.property("source").property("id").toString();
            QString parentPath = sourceChildObj.property("source").property("parent").property("id").toString();
            bool isDeleted = sourceChildObj.property("event_type").toString() == "DELETE"
                              || sourceChildObj.property("event_type").toString() == "ITEM_TRASH";

            QScriptValue parsedChildObj = engine.newObject();
            parsedChildObj.setProperty("changeId", sourceChildObj.property("event_id"));
            parsedChildObj.setProperty("isDeleted", QScriptValue(isDeleted));
            parsedChildObj.setProperty("absolutePath", QScriptValue(absolutePath));
            parsedChildObj.setProperty("parentPath", QScriptValue(parentPath));
            childrenObj.setProperty(i, parsedChildObj);
        }
        parsedObj.setProperty("children", childrenObj);
        parsedObj.setProperty("nextDeltaCursor", sourceObj.property("next_stream_position"));
        parsedObj.setProperty("reset", QScriptValue(isReset));

        // Check hasMore and reset state.
        bool hasMore = (sourceObj.property("next_stream_position").toString() != "" && entriesCount > 0);
        parsedObj.setProperty("hasMore", QScriptValue(hasMore));
        if (isReset) {
            if (hasMore) {
                qDebug() << "BoxClient::deltaReplyFinished hasMore" << hasMore << "proceed update state only.";
            } else {
                qDebug() << "BoxClient::deltaReplyFinished hasMore" << hasMore << "reset to normal state.";
                m_settings.setValue(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(false));
            }
        }

        // Save nextDeltaCursor
        m_settings.setValue(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid), QVariant(parsedObj.property("nextDeltaCursor").toString()));

        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit deltaReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

void BoxClient::fileGetResumeReplyFinished(QNetworkReply *reply)
{
    qDebug() << "BoxClient::fileGetResumeReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Detect redirection.
    QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!possibleRedirectUrl.toUrl().isEmpty()) {
        qDebug() << "BoxClient::fileGetResumeReplyFinished redirectUrl" << possibleRedirectUrl.toUrl();

        // Delete existing reply..
        m_replyHash->remove(nonce);
        reply->deleteLater();

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

    QString nonce = engine.globalObject().property("nonce").toString();
    QString uid = engine.globalObject().property("uid").toString();

    bool isDir = jsonObj.property("type").toString().indexOf(QRegExp("folder|album")) == 0;
    // Save item isDir to hash.
    m_isDirHash->insert(jsonObj.property("id").toString(), isDir);

    QString jsonDateString = formatJSONDateString(parseReplyDateString(jsonObj.property("modified_at").toString()));
    QString hash = isDir ? jsonObj.property("modified_at").toString() : jsonObj.property("sha1").toString();
    QString thumbnailUrl = (!isDir) ? thumbnail(nonce, uid, jsonObj.property("id").toString(), "png", "64x64") : "";
    QString thumbnail128Url = (!isDir) ? thumbnail(nonce, uid, jsonObj.property("id").toString(), "png", "128x128") : "";
    QString previewUrl = (!isDir) ? thumbnail(nonce, uid, jsonObj.property("id").toString(), "png", "256x256") : "";
    QString alternative = jsonObj.property("shared_link").property("url").toString();

    parsedObj.setProperty("name", jsonObj.property("name"));
    parsedObj.setProperty("absolutePath", jsonObj.property("id"));
    parsedObj.setProperty("parentPath", !jsonObj.property("parent").isNull() ? jsonObj.property("parent").property("id") : QScriptValue(""));
    parsedObj.setProperty("size", jsonObj.property("size"));
    parsedObj.setProperty("isDeleted", QScriptValue(jsonObj.property("item_status").toString() == "trashed"));
    parsedObj.setProperty("isDir", QScriptValue(isDir));
    parsedObj.setProperty("lastModified", QScriptValue(jsonDateString));
    parsedObj.setProperty("hash", QScriptValue(hash));
    parsedObj.setProperty("source", QScriptValue());
    parsedObj.setProperty("alternative", QScriptValue(alternative));
    parsedObj.setProperty("thumbnail", QScriptValue(thumbnailUrl));
    parsedObj.setProperty("thumbnail128", QScriptValue(thumbnail128Url));
    parsedObj.setProperty("preview", QScriptValue(previewUrl));
    parsedObj.setProperty("fileType", QScriptValue(getFileType(jsonObj.property("name").toString())));

    return parsedObj;
}

qint64 BoxClient::getChunkSize()
{
    return m_settings.value(QString("%1.resumable.chunksize").arg(objectName()), DefaultChunkSize).toInt();
}

QString BoxClient::thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format, QString size)
{
    if (uid.isEmpty()) {
        qDebug() << "BoxClient::thumbnail uid is empty.";
        return "";
    }

    if (!accessTokenPairMap.contains(uid)) {
        qDebug() << "BoxClient::thumbnail uid" << uid << "is not authorized.";
        return "";
    }

    QStringList splittedSize = size.split("x");
    QString uri = thumbnailURI.arg(remoteFilePath).arg(format);
    if (splittedSize.count() > 1) {
        uri += QString("?min_height=%1&min_width=%2").arg(splittedSize.at(0)).arg(splittedSize.at(1));
    } else {
        uri += QString("?min_height=%1&min_width=%2").arg(splittedSize.at(0)).arg(splittedSize.at(0));
    }
    uri += "&access_token=" + accessTokenPairMap[uid].token;
//    qDebug() << "BoxClient::thumbnail uri" << uri;

    return uri;
}

QString BoxClient::media(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- BoxClient::media -----" << nonce << uid << remoteFilePath;

    if (remoteFilePath.isEmpty()) {
        qDebug() << "BoxClient::media" << nonce << uid << "remoteFilePath is empty. Operation is aborted.";
        return "";
    }

    if (!isMediaEnabled(uid)) {
        emit logRequestSignal(nonce,
                              "error",
                              "Box " + tr("Direct link"),
                              tr("Direct link to files is available on upgraded users. Once you've upgraded please enable BOX media feature in account settings."),
                              5000);
        return "";
    }

    // Check if remoteFilePath is directory.
    bool isDir = isRemoteDir(nonce, uid, remoteFilePath);

    QString uri = sharesURI.arg(isDir ? "folders" : "files").arg(remoteFilePath);
    qDebug() << "BoxClient::media uri" << uri;

    QString postString = "{\"shared_link\":{\"access\":\"open\"}}";
    qDebug() << "BoxClient::media postString" << postString;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->put(req, postString.toUtf8());

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    QString url = "";
    int expires = 0;
    if (reply->error() == QNetworkReply::NoError) {
        QString replyBody = QString::fromUtf8(reply->readAll());
        qDebug() << "BoxClient::mediaReplyFinished nonce" << nonce << "replyBody" << replyBody;

        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + replyBody + ")");
        url = sc.property("shared_link").property("download_url").toString();
        expires = -1;

        // Append with OAuth token.
        if (url != "") {
            url += "?access_token=" + accessTokenPairMap[uid].token;
        }
    }

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return url;
}

QDateTime BoxClient::parseReplyDateString(QString dateString)
{
    /*
     *Example date string
     * "2013-06-27T03:08:59-07:00"
    */

    QString filteredDateString = dateString;
    QDateTime datetime = QDateTime::fromString(filteredDateString, Qt::ISODate);
    qDebug() << "BoxClient::parseReplyDateString parse filteredDateString" << filteredDateString << "with ISODate to" << datetime << "toUTC" << datetime.toUTC();

    return datetime.toUTC();
}
