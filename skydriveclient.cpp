#include "skydriveclient.h"
#include <QtGlobal>
#include <QApplication>
#include <QCoreApplication>
#include <QScriptEngine>

SkyDriveClient::SkyDriveClient(QObject *parent) :
    CloudDriveClient(parent)
{
    consumerKey = "00000000480E4F62";
    consumerSecret = "iulfMFYbOZqdgcSHOMeoR0mehcl0yFeO";
    RemoteRoot = "me/skydrive";

    authorizeURI = "https://login.live.com/oauth20_authorize.srf"; // &display=touch
    accessTokenURI = "https://login.live.com/oauth20_token.srf";
    accountInfoURI = "https://apis.live.net/v5.0/me";
    quotaURI = "https://apis.live.net/v5.0/me/skydrive/quota";
    logoutURI = "https://login.live.com/oauth20_logout.srf";

    fileGetURI = "https://apis.live.net/v5.0/%1/content"; // ?download=true ?suppress_redirects=true
    filePutURI = "https://apis.live.net/v5.0/%1/files/%2"; // PUT with ?downsize_photo_uploads=false
    filesURI = "https://apis.live.net/v5.0/%1/files";
    propertyURI = "https://apis.live.net/v5.0/%1"; // GET or PUT to update.
    createFolderURI = "https://apis.live.net/v5.0/%1"; // POST with json with name = new folder name.
    moveFileURI = "https://apis.live.net/v5.0/%1"; // MOVE to destination folder ID in content.
    copyFileURI = "https://apis.live.net/v5.0/%1"; // COPY to destination folder ID in content.
    deleteFileURI = "https://apis.live.net/v5.0/%1"; // DELETE
    renameFileURI = "https://apis.live.net/v5.0/%1"; // PUT with json with name = new folder name.
    sharesURI = "https://apis.live.net/v5.0/%1/shared_read_link";

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

void SkyDriveClient::authorize(QString nonce, QString hostname)
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
    qDebug() << "SkyDriveClient::accessToken queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "SkyDriveClient::accessToken postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager->post(req, postData);
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
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager->post(req, postData);
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
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessToken).toAscii() );
    manager->get(req);
}

void SkyDriveClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- SkyDriveClient::quota ----- uid" << uid;

    QString uri = quotaURI;
    qDebug() << "SkyDriveClient::quota uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    manager->get(req);
}

void SkyDriveClient::metadata(QString nonce, QString uid, QString remoteFilePath) {
    qDebug() << "----- SkyDriveClient::metadata -----" << nonce << uid << remoteFilePath;

    if (remoteFilePath.isEmpty()) {
        emit metadataReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    property(nonce, uid, remoteFilePath, false, false, "metadata");
    files(nonce, uid, remoteFilePath, 0, false, "metadata");
}

void SkyDriveClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- SkyDriveClient::browse -----" << nonce << uid << remoteFilePath;

    // Default remoteFilePath if it's empty from DrivePage.
    remoteFilePath = (remoteFilePath == "") ? RemoteRoot : remoteFilePath;

    property(nonce, uid, remoteFilePath, false, false, "browse");
    files(nonce, uid, remoteFilePath, 0, false, "browse");
}

QNetworkReply * SkyDriveClient::files(QString nonce, QString uid, QString remoteFilePath, int offset, bool synchronous, QString callback)
{
    qDebug() << "----- SkyDriveClient::files -----" << nonce << uid << remoteFilePath << synchronous << callback;

    QString uri = filesURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::files uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
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

QNetworkReply * SkyDriveClient::property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous, QString callback)
{
    qDebug() << "----- SkyDriveClient::property -----" << nonce << uid << remoteFilePath << synchronous << callback;

    QString uri = propertyURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::property uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
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

QString SkyDriveClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::createFolder -----" << nonce << uid << remoteParentPath << newRemoteFolderName << synchronous;

    if (remoteParentPath.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
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
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
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

void SkyDriveClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- SkyDriveClient::moveFile -----" << nonce << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

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
        CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        manager->sendCustomRequest(req, "MOVE", m_bufferHash[nonce]);
    }
}

void SkyDriveClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- SkyDriveClient::copyFile -----" << nonce << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

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
        CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(copyFileReplyFinished(QNetworkReply*)) );
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        manager->sendCustomRequest(req, "COPY", m_bufferHash[nonce]);
    }
}

QString SkyDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::deleteFile -----" << nonce << uid << remoteFilePath << synchronous;

    QString uri = deleteFileURI.arg(remoteFilePath);
//    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::deleteFile uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
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
    qDebug() << "----- SkyDriveClient::renameFile -----" << nonce << uid << remoteFilePath << newName;

    QString uri = renameFileURI.arg(remoteFilePath);
    qDebug() << "SkyDriveClient::renameFile uri " << uri;

    QByteArray postData;
    postData.append("{ \"name\": \"");
    postData.append(newName.toUtf8());
    postData.append("\" }");
    qDebug() << "SkyDriveClient::renameFile postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    manager->put(req, postData);
}

QNetworkReply *SkyDriveClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::filePut -----" << nonce << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString uri = filePutURI.arg(remoteParentPath).arg(remoteFileName);
    uri = encodeURI(uri) + "?downsize_photo_uploads=false";
    qDebug() << "SkyDriveClient::filePut" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, bytesTotal);
    QNetworkReply *reply = manager->put(req, source);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (synchronous) {
        // Remove finished reply from hash.
        m_replyHash->remove(nonce);
    }

    return reply;
}

bool SkyDriveClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize == -1 || fileSize >= getChunkSize());
}

bool SkyDriveClient::isFileGetRedirected()
{
    return true;
}

bool SkyDriveClient::isViewable()
{
    return true;
}

QString SkyDriveClient::shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::shareFile -----" << nonce << uid << remoteFilePath << synchronous;

    QString uri = sharesURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::shareFile" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);

    // Return if asynchronous.
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

    return shareFileReplyFinished(reply, synchronous);
}

void SkyDriveClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::accessTokenReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

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

            // Save tokens.
            saveAccessPairMap();
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
    qDebug() << "SkyDriveClient::quotaReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString replyBody = QString::fromUtf8(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
        qint64 sharedValue = 0;
        qint64 normalValue = jsonObj.property("quota").toInteger() - jsonObj.property("available").toInteger();
        qint64 quotaValue = jsonObj.property("quota").toInteger();

        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, normalValue, sharedValue, quotaValue);
    } else {
        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, 0, 0, -1);
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString SkyDriveClient::filePutReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "SkyDriveClient::filePutReplyFinished" << nonce << "replyBody" << replyBody;

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + replyBody + ")");
        QString remoteFilePath = sc.property("id").toString();

        // Delete original reply (which is in m_replyHash).
        m_replyHash->remove(nonce);
        reply->deleteLater();
        reply->manager()->deleteLater();

        // Get property synchronously.
        reply = property(nonce, uid, remoteFilePath, true, "filePutReplyFinished");
        replyBody = QString::fromUtf8(reply->readAll());
        qDebug() << "SkyDriveClient::filePutReplyFinished" << nonce << "property replyBody" << replyBody;
        if (reply->error() == QNetworkReply::NoError) {
            QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
            QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
            replyBody = stringifyScriptValue(engine, parsedObj);
        }
    }

    return replyBody;
}

void SkyDriveClient::metadataReplyFinished(QNetworkReply *reply) {
    qDebug() << "SkyDriveClient::metadataReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::browseReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::browseReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::mergePropertyAndFilesJson(QString nonce, QString callback, QString uid)
{
    if (m_propertyReplyHash->contains(nonce) && m_filesReplyHash->contains(nonce)) {
        QScriptEngine engine;
        engine.setProperty("uid", QVariant(uid));
        QScriptValue mergedObj;
        QScriptValue propertyObj;
        QScriptValue filesObj;
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "SkyDriveClient::mergePropertyAndFilesJson" << nonce << "started.";
        if (m_settings.value("Logging.enabled", false).toBool()) {
            qDebug() << "SkyDriveClient::mergePropertyAndFilesJson propertyJson" << QString::fromUtf8(m_propertyReplyHash->value(nonce));
            qDebug() << "SkyDriveClient::mergePropertyAndFilesJson filesJson" << QString::fromUtf8(m_filesReplyHash->value(nonce));
        }
        propertyObj = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
        filesObj = engine.evaluate("(" + QString::fromUtf8(m_filesReplyHash->value(nonce)) + ")");

        mergedObj = parseCommonPropertyScriptValue(engine, propertyObj);
        mergedObj.setProperty("children", engine.newArray());
        int contentsCount = filesObj.property("data").property("length").toInteger();
        for (int i = 0; i < contentsCount; i++) {
            QApplication::processEvents();

            mergedObj.property("children").setProperty(i, parseCommonPropertyScriptValue(engine, filesObj.property("data").property(i)));
        }

        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "SkyDriveClient::mergePropertyAndFilesJson" << nonce << "stringifyScriptValue started.";
        QString replyBody = stringifyScriptValue(engine, mergedObj);
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "SkyDriveClient::mergePropertyAndFilesJson" << nonce << "stringifyScriptValue done." << replyBody.size();

        // Remove once used.
        m_propertyReplyHash->remove(nonce);
        m_filesReplyHash->remove(nonce);

        if (callback == "browse") {
            emit browseReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else {
            qDebug() << "SkyDriveClient::mergePropertyAndFilesJson invalid callback" << callback;
        }
    }
}

void SkyDriveClient::propertyReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::propertyReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

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
            qDebug() << "SkyDriveClient::propertyReplyFinished invalid callback" << callback;
        }
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::filesReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::filesReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    if (reply->error() == QNetworkReply::NoError) {
        m_filesReplyHash->insert(nonce, reply->readAll());

        // Merge and emit signal if both property and files are available.
        mergePropertyAndFilesJson(nonce, callback, uid);
    } else if (reply->error() == QNetworkReply::UnknownContentError) {
        qDebug() << "SkyDriveClient::filesReplyFinished suppress error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
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
            qDebug() << "SkyDriveClient::filesReplyFinished invalid callback" << callback;
        }
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString SkyDriveClient::createFolderReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteParentPath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString newRemoteFileName = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

    qDebug() << "SkyDriveClient::createFolderReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "SkyDriveClient::createFolderReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    } else if (reply->error() == QNetworkReply::UnknownContentError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        if (jsonObj.property("error").property("code").toString() == "resource_already_exists") {
            // Delete original reply (which is in m_replyHash).
            m_replyHash->remove(nonce);
            reply->deleteLater();
            reply->manager()->deleteLater();

            // Get existing folder's property.
            reply = files(nonce, uid, remoteParentPath, 0, true);
            replyBody = QString::fromUtf8(reply->readAll());
            qDebug() << "SkyDriveClient::createFolderReplyFinished" << nonce << "files replyBody" << replyBody;
            if (reply->error() == QNetworkReply::NoError) {
                QScriptEngine engine;
                QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
                int contentsCount = jsonObj.property("data").property("length").toInteger();
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

void SkyDriveClient::moveFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::moveFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "SkyDriveClient::moveFileReplyFinished replyBody" << replyBody;
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

void SkyDriveClient::copyFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::copyFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "SkyDriveClient::copyFileReplyFinished replyBody" << replyBody;
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

void SkyDriveClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::deleteFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "SkyDriveClient::deleteFileReplyFinished replyBody" << replyBody;
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

QString SkyDriveClient::shareFileReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "SkyDriveClient::shareFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

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

    if (!synchronous) emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, url, expires);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return url;
}

QScriptValue SkyDriveClient::parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj)
{
    QScriptValue parsedObj = engine.newObject();

    QString jsonDateString = formatJSONDateString(parseReplyDateString(jsonObj.property("updated_time").toString()));
    QString alternative = jsonObj.property("link").toString();
    QString thumbnail128Url = jsonObj.property("images").property(1).property("source").toString();
    QString previewUrl = jsonObj.property("images").property(0).property("source").toString();

    parsedObj.setProperty("name", jsonObj.property("name"));
    parsedObj.setProperty("absolutePath", jsonObj.property("id"));
    parsedObj.setProperty("parentPath", !jsonObj.property("parent_id").isNull() ? jsonObj.property("parent_id") : QScriptValue(""));
    parsedObj.setProperty("size", jsonObj.property("size"));
    parsedObj.setProperty("isDeleted", QScriptValue(false));
    parsedObj.setProperty("isDir", QScriptValue(jsonObj.property("type").toString().indexOf(QRegExp("folder|album")) == 0));
    parsedObj.setProperty("lastModified", QScriptValue(jsonDateString) );
    parsedObj.setProperty("hash", jsonObj.property("updated_time"));
    parsedObj.setProperty("source", jsonObj.property("source"));
    parsedObj.setProperty("alternative", QScriptValue(alternative));
    parsedObj.setProperty("thumbnail", jsonObj.property("picture"));
    parsedObj.setProperty("thumbnail128", QScriptValue(thumbnail128Url));
    parsedObj.setProperty("preview", QScriptValue(previewUrl));
    parsedObj.setProperty("fileType", QScriptValue(getFileType(jsonObj.property("name").toString())));

    return parsedObj;
}

QDateTime SkyDriveClient::parseReplyDateString(QString dateString)
{
    /*
     *Example date string
     * "2013-06-15T13:58:51+0000"
    */

    QString filteredDateString = dateString;
    QDateTime datetime = QDateTime::fromString(filteredDateString, Qt::ISODate);
    qDebug() << "SkyDriveClient::parseReplyDateString parse filteredDateString" << filteredDateString << "with ISODate to" << datetime;
    datetime.setTimeSpec(Qt::UTC);
    qDebug() << "SkyDriveClient::parseReplyDateString parse datetime.setTimeSpec(Qt::UTC)" << datetime;

    return datetime;
}
