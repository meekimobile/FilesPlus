#include "skydriveclient.h"
#include <QtGlobal>
#include <QApplication>
#include <QCoreApplication>
#include <QScriptEngine>

const QString SkyDriveClient::consumerKey = "00000000480E4F62";
const QString SkyDriveClient::consumerSecret = "iulfMFYbOZqdgcSHOMeoR0mehcl0yFeO";
const QString SkyDriveClient::RemoteRoot = "me/skydrive";

const QString SkyDriveClient::authorizeURI = "https://login.live.com/oauth20_authorize.srf"; // &display=touch
const QString SkyDriveClient::accessTokenURI = "https://login.live.com/oauth20_token.srf";
const QString SkyDriveClient::accountInfoURI = "https://apis.live.net/v5.0/me";
const QString SkyDriveClient::quotaURI = "https://apis.live.net/v5.0/me/skydrive/quota";
const QString SkyDriveClient::logoutURI = "https://login.live.com/oauth20_logout.srf";

const QString SkyDriveClient::fileGetURI = "https://apis.live.net/v5.0/%1/content"; // ?download=true ?suppress_redirects=true
const QString SkyDriveClient::filePutURI = "https://apis.live.net/v5.0/%1/files/%2"; // PUT
const QString SkyDriveClient::filesURI = "https://apis.live.net/v5.0/%1/files";
const QString SkyDriveClient::propertyURI = "https://apis.live.net/v5.0/%1"; // GET or PUT to update.
const QString SkyDriveClient::createFolderURI = "https://apis.live.net/v5.0/%1"; // POST with json with name = new folder name.
const QString SkyDriveClient::moveFileURI = "https://apis.live.net/v5.0/%1"; // MOVE to destination folder ID in content.
const QString SkyDriveClient::copyFileURI = "https://apis.live.net/v5.0/%1"; // COPY to destination folder ID in content.
const QString SkyDriveClient::deleteFileURI = "https://apis.live.net/v5.0/%1"; // DELETE
const QString SkyDriveClient::renameFileURI = "https://apis.live.net/v5.0/%1"; // PUT with json with name = new folder name.
const QString SkyDriveClient::sharesURI = "https://apis.live.net/v5.0/%1/shared_read_link";

const qint64 SkyDriveClient::ChunkSize = 4194304; // 4MB

SkyDriveClient::SkyDriveClient(QObject *parent) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();

    m_propertyReplyHash = new QHash<QString, QByteArray>;
    m_filesReplyHash = new QHash<QString, QByteArray>;
}

SkyDriveClient::~SkyDriveClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_propertyReplyHash = 0;
    m_filesReplyHash = 0;
}

QString SkyDriveClient::createTimestamp() {
    qint64 seconds = QDateTime::currentMSecsSinceEpoch() / 1000;

    return QString("%1").arg(seconds);
}

QString SkyDriveClient::createNormalizedQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(QUrl::toPercentEncoding(key)).append("=").append(QUrl::toPercentEncoding(sortMap[key]));
    }

    return queryString;
}

QString SkyDriveClient::encodeURI(const QString uri) {
    // Example: https://api.dropbox.com/1/metadata/sandbox/C/B/NES/Solomon's Key (E) [!].nes
    // All non-alphanumeric except : and / must be encoded.
    return QUrl::toPercentEncoding(uri, ":/");
}

void SkyDriveClient::authorize(QString nonce)
{
    qDebug() << "----- SkyDriveClient::authorize -----";

    QString queryString;
    queryString.append("client_id=" + consumerKey);
    queryString.append("&response_type=code");
    queryString.append("&scope=" + QUrl::toPercentEncoding("wl.signin wl.basic wl.offline_access wl.emails wl.skydrive wl.skydrive_update"));
    queryString.append("&redirect_uri=http://www.meeki.mobi/products/filesplus/skd_oauth_callback");
    queryString.append("&display=touch");

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, objectName());
}

void SkyDriveClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- SkyDriveClient::accessToken -----" << "pin" << pin;

    refreshTokenUid = "";

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["redirect_uri"] = "http://www.meeki.mobi/products/filesplus/skd_oauth_callback";
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

void SkyDriveClient::refreshToken(QString nonce, QString uid)
{
    qDebug() << "----- SkyDriveClient::refreshToken -----" << "uid" << uid;

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

void SkyDriveClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- SkyDriveClient::accountInfo ----- uid" << uid;

    QString accessToken;
    if (uid == "") {
        // After accessToken, then uses temp access token..
        accessToken = m_paramMap["access_token"];
    } else {
        accessToken = accessTokenPairMap[uid].token;
    }

    QString uri = accountInfoURI;
    qDebug() << "SkyDriveClient::accountInfo uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessToken).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

void SkyDriveClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- SkyDriveClient::quota ----- uid" << uid;

    QString uri = quotaURI;
    qDebug() << "SkyDriveClient::quota uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

QString SkyDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::fileGet -----" << uid << remoteFilePath << localFilePath << synchronous;

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

void SkyDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName) {
    qDebug() << "----- SkyDriveClient::filePut -----" << localFilePath << "to" << remoteParentPath << remoteFileName;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        filePut(nonce, uid, localSourceFile, fileSize, remoteParentPath, remoteFileName, false);
    } else {
        qDebug() << "SkyDriveClient::filePut file " << localFilePath << " can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
    }
}

void SkyDriveClient::metadata(QString nonce, QString uid, QString remoteFilePath) {
    qDebug() << "----- SkyDriveClient::metadata -----" << "uid" << uid << "remoteFilePath" << remoteFilePath;

    if (remoteFilePath.isEmpty()) {
        emit metadataReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    property(nonce, uid, remoteFilePath, false, "metadata");
    files(nonce, uid, remoteFilePath, false, "metadata");

/*
    // Merge files and property into single JSON string.
    QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath);
    if (propertyReply->error() != QNetworkReply::NoError) {
        propertyReply->deleteLater();
        propertyReply->manager()->deleteLater();
        emit metadataReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), propertyReply->readAll());
        return;
    }

    QScriptEngine engine;
    QScriptValue sc;
    QScriptValue scProperty;
    QScriptValue scJsonStringify;

    QString propertyJsonText = propertyReply->readAll();
    scProperty = engine.evaluate("(" + propertyJsonText + ")");
    QString typeName = scProperty.property("type").toString();
//    qDebug() << "SkyDriveClient::metadata propertyJsonText" << propertyJsonText << "typeName" << typeName;

    QNetworkReply *filesReply;
    if (typeName == "folder" || typeName == "album") {
        filesReply = files(nonce, uid, remoteFilePath);
        if (filesReply->error() != QNetworkReply::NoError) {
            filesReply->deleteLater();
            filesReply->manager()->deleteLater();
            emit metadataReplySignal(nonce, filesReply->error(), filesReply->errorString(), filesReply->readAll());
            return;
        }

        sc = engine.evaluate("(" + filesReply->readAll() + ")");
    } else {
        sc = engine.evaluate("({})");
    }

    sc.setProperty("property", scProperty);
    scJsonStringify = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << sc);
//    qDebug() << "SkyDriveClient::metadata scJsonStringify.toString()" << scJsonStringify.toString();

    // TODO scheduled to delete later.
    propertyReply->deleteLater();
    propertyReply->manager()->deleteLater();
    filesReply->deleteLater();
    filesReply->manager()->deleteLater();

    emit metadataReplySignal(nonce, QNetworkReply::NoError, "", scJsonStringify.toString());
*/
}

void SkyDriveClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- SkyDriveClient::browse -----" << remoteFilePath;
    if (remoteFilePath.isEmpty()) {
        emit browseReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    property(nonce, uid, remoteFilePath, false, "browse");
    files(nonce, uid, remoteFilePath, false, "browse");

/*
    // Merge files and property into single JSON string.
    QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath);
    if (propertyReply->error() != QNetworkReply::NoError) {
        propertyReply->deleteLater();
        propertyReply->manager()->deleteLater();
        emit browseReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), propertyReply->readAll());
        return;
    }
    QNetworkReply *filesReply = files(nonce, uid, remoteFilePath);
    if (filesReply->error() != QNetworkReply::NoError) {
        filesReply->deleteLater();
        filesReply->manager()->deleteLater();
        emit browseReplySignal(nonce, filesReply->error(), filesReply->errorString(), filesReply->readAll());
        return;
    }

    QScriptEngine engine;
    QScriptValue sc;
    QScriptValue scProperty;
    QScriptValue scJsonStringify;
    sc = engine.evaluate("(" + filesReply->readAll() + ")");
    scProperty = engine.evaluate("(" + propertyReply->readAll() + ")");
    sc.setProperty("property", scProperty);

    scJsonStringify = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << sc);
//    qDebug() << "SkyDriveClient::browse scJsonStringify.toString()" << scJsonStringify.toString();

    // TODO scheduled to delete later.
    propertyReply->deleteLater();
    propertyReply->manager()->deleteLater();
    filesReply->deleteLater();
    filesReply->manager()->deleteLater();

    emit browseReplySignal(nonce, QNetworkReply::NoError, "", scJsonStringify.toString());
*/
}

QNetworkReply * SkyDriveClient::files(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- SkyDriveClient::files -----" << remoteFilePath;

    QApplication::processEvents();

    QString uri = filesURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::files uri " << uri;

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

QNetworkReply * SkyDriveClient::property(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- SkyDriveClient::property -----" << remoteFilePath << callback;

    QApplication::processEvents();

    QString uri = propertyURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::property uri " << uri;

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

void SkyDriveClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteParentPath, newRemoteFolderName, false);
}

QString SkyDriveClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::createFolder -----" << newRemoteFolderName << remoteParentPath;

    if (remoteParentPath.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
    }

    QString uri = createFolderURI.arg(remoteParentPath);
    qDebug() << "SkyDriveClient::createFolder uri " << uri;

    QByteArray postData;
    postData.append("{ \"name\": \"");
    postData.append(newRemoteFolderName.toUtf8());
    postData.append("\" }");
    qDebug() << "SkyDriveClient::createFolder postData" << postData;

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

void SkyDriveClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- SkyDriveClient::moveFile -----" << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

//    QRegExp rx("(file\.|folder\.)(" + uid + ")(.*)");
//    if (rx.exactMatch(targetRemoteParentPath)) {
//        // Match as remote ID.
//    }

    if (newRemoteFileName != "" && targetRemoteParentPath == "") {
        // Proceed renaming.
        renameFile(nonce, uid, remoteFilePath, newRemoteFileName);
        return;
    }

    QString uri = moveFileURI.arg(remoteFilePath);
    qDebug() << "SkyDriveClient::moveFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"destination\": \"");
    postData.append(targetRemoteParentPath.toUtf8());
    postData.append("\" }");
    qDebug() << "SkyDriveClient::moveFile postData" << postData;

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

void SkyDriveClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- SkyDriveClient::copyFile -----" << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

    QString uri = copyFileURI.arg(remoteFilePath);
    qDebug() << "SkyDriveClient::copyFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"destination\": \"");
    postData.append(targetRemoteParentPath.toUtf8());
    postData.append("\" }");
    qDebug() << "SkyDriveClient::copyFile postData" << postData;

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

void SkyDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    deleteFile(nonce, uid, remoteFilePath, false);
}

QString SkyDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::deleteFile -----";

    QString uri = deleteFileURI.arg(remoteFilePath);
//    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::deleteFile uri " << uri;

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

void SkyDriveClient::renameFile(QString nonce, QString uid, QString remoteFilePath, QString newName)
{
    qDebug() << "----- SkyDriveClient::renameFile -----" << remoteFilePath << newName;

    QString uri = renameFileURI.arg(remoteFilePath);
    qDebug() << "SkyDriveClient::renameFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"name\": \"");
    postData.append(newName.toUtf8());
    postData.append("\" }");
    qDebug() << "SkyDriveClient::renameFile postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = manager->put(req, postData);
}

QIODevice *SkyDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::fileGet -----" << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    // remoteFilePath is not a URL. Procees getting property to get downloadUrl.
    if (!synchronous) {
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "fileGet");
        if (propertyReply->error() == QNetworkReply::NoError) {
            // For further using in fileGetReplyFinished.
            m_propertyReplyHash->insert(nonce, propertyReply->readAll());
            propertyReply->deleteLater();
        } else {
            emit fileGetReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
            propertyReply->deleteLater();
            return 0;
        }
    }

    QString uri = fileGetURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::fileGet uri " << uri;

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
        qDebug() << "SkyDriveClient::fileGet rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    // Redirect to download URL.
    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (synchronous) {
        QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if(!possibleRedirectUrl.toUrl().isEmpty()) {
            qDebug() << "SkyDriveClient::fileGetReplyFinished redirectUrl" << possibleRedirectUrl.toUrl();

            QNetworkRequest redirectedRequest = reply->request();
            redirectedRequest.setUrl(possibleRedirectUrl.toUrl());
            reply = reply->manager()->get(redirectedRequest);
            QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
            connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

            while (!reply->isFinished()) {
                QApplication::processEvents(QEventLoop::AllEvents, 100);
                Sleeper::msleep(100);
            }
        }
    }

    return reply;
}

QString SkyDriveClient::fileGetReplySave(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "SkyDriveClient::fileGetReplySave reply bytesAvailable" << reply->bytesAvailable();

        // Stream replyBody to a file on localPath.
        qint64 totalBytes = 0;
        char buf[1024];
        QFile *localTargetFile = m_localFileHash[nonce];
        if (localTargetFile->open(QIODevice::Append)) {
            // Issue: Writing to file with QDataStream << QByteArray will automatically prepend with 4-bytes prefix(size).
            // Solution: Use QIODevice to write directly.

            // TODO Move to offset.
            qDebug() << "SkyDriveClient::fileGetReplySave localTargetFile->pos()" << localTargetFile->pos();

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

        qDebug() << "SkyDriveClient::fileGetReplySave reply totalBytes=" << totalBytes;

        // Close target file.
        localTargetFile->close();

        return QString::fromUtf8(m_propertyReplyHash->value(nonce));
    } else {
        qDebug() << "SkyDriveClient::fileGetReplySave nonce" << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        return QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Remove once used.
    m_localFileHash.remove(nonce);
    m_propertyReplyHash->remove(nonce);
}

QNetworkReply *SkyDriveClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::filePut -----" << nonce << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString uri = filePutURI.arg(remoteParentPath).arg(remoteFileName);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::filePut uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, bytesTotal);
    QNetworkReply *reply = manager->put(req, source);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    return reply;
}

QIODevice *SkyDriveClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- SkyDriveClient::fileGetResume -----" << remoteFilePath << "to" << localFilePath << "offset" << offset;

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
    qDebug() << "SkyDriveClient::fileGetResume uri " << uri;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetResumeReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+ChunkSize-1);
        qDebug() << "SkyDriveClient::fileGetResume rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    return reply;
}

QString SkyDriveClient::getRemoteRoot(QString uid)
{
    return RemoteRoot;
}

bool SkyDriveClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize >= ChunkSize);
}

void SkyDriveClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- SkyDriveClient::shareFile -----" << remoteFilePath;

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = sharesURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::shareFile uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);
}

void SkyDriveClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::accessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Load response parameters into map.
    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        m_paramMap["access_token"] = sc.property("access_token").toString();
        m_paramMap["refresh_token"] = sc.property("refresh_token").toString();
        qDebug() << "SkyDriveClient::accessTokenReplyFinished m_paramMap " << m_paramMap;

        if (refreshTokenUid != "") {
            // Updates for refreshToken.
            accessTokenPairMap[refreshTokenUid].token = sc.property("access_token").toString();
            accessTokenPairMap[refreshTokenUid].secret = sc.property("refresh_token").toString();
            qDebug() << "SkyDriveClient::accessTokenReplyFinished accessTokenPairMap" << accessTokenPairMap;

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

void SkyDriveClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::accountInfoReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        QString uid = sc.property("id").toString();
        QString name = sc.property("name").toString();
        QString email = sc.property("emails").property("account").toString();

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

//        qDebug() << "SkyDriveClient::accountInfoReplyFinished accessTokenPairMap" << accessTokenPairMap;

        // Save account after got id and email.
        saveAccessPairMap();
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::quotaReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::quotaReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit quotaReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::fileGetReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::fileGetReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    // Detect redirection.
    QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!possibleRedirectUrl.toUrl().isEmpty()) {
        qDebug() << "SkyDriveClient::fileGetReplyFinished redirectUrl" << possibleRedirectUrl.toUrl();

        QNetworkRequest redirectedRequest = reply->request();
        redirectedRequest.setUrl(possibleRedirectUrl.toUrl());
        reply = reply->manager()->get(redirectedRequest);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
        return;
    }

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), result);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::filePutReplyFinished(QNetworkReply *reply) {
    qDebug() << "SkyDriveClient::filePutReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    // Close source file.
    QFile *localTargetFile = m_localFileHash[nonce];
    localTargetFile->close();
    m_localFileHash.remove(nonce);

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(reply->readAll()) + ")");
        QString remoteFilePath = sc.property("id").toString();
        // Get property synchronously.
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "filePutReplyFinished");
        if (propertyReply->error() == QNetworkReply::NoError) {
            sc = engine.evaluate("(" + QString::fromUtf8(propertyReply->readAll()) + ")");
        }
        propertyReply->deleteLater();

        QScriptValue scJsonStringify = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << sc);
        emit filePutReplySignal(nonce, reply->error(), reply->errorString(), scJsonStringify.toString());
    } else {
        // REMARK Use QString::fromUtf8() to support unicode text.
        emit filePutReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::metadataReplyFinished(QNetworkReply *reply) {
    qDebug() << "SkyDriveClient::metadataReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::browseReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::browseReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::propertyReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::propertyReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

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
            qDebug() << "SkyDriveClient::propertyReplyFinished invalid callback" << callback;
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
            qDebug() << "SkyDriveClient::propertyReplyFinished invalid callback" << callback;
        }
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::filesReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::filesReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

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
            qDebug() << "SkyDriveClient::filesReplyFinished invalid callback" << callback;
        }
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::createFolderReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::createFolderReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::moveFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::moveFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

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

void SkyDriveClient::copyFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::copyFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // Remove request buffer.
    m_bufferHash[nonce]->close();
    m_bufferHash.remove(nonce);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::deleteFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::shareFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::shareFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(reply->readAll()));

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::fileGetResumeReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::fileGetResumeReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    // Detect redirection.
    QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!possibleRedirectUrl.toUrl().isEmpty()) {
        qDebug() << "SkyDriveClient::fileGetResumeReplyFinished redirectUrl" << possibleRedirectUrl.toUrl();

        QNetworkRequest redirectedRequest = reply->request();
        redirectedRequest.setUrl(possibleRedirectUrl.toUrl());
        reply = reply->manager()->get(redirectedRequest);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
        return;
    }

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetResumeReplySignal(nonce, reply->error(), reply->errorString(), result);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}
