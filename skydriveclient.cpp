#include "skydriveclient.h"
#include <QtGlobal>
#include <QCoreApplication>
#include <QScriptEngine>

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString SkyDriveClient::KeyStoreFilePath = "/home/user/.filesplus/SkyDriveClient.dat";
#else
const QString SkyDriveClient::KeyStoreFilePath = "C:/SkyDriveClient.dat";
#endif
const QString SkyDriveClient::consumerKey = "00000000480E4F62";
const QString SkyDriveClient::consumerSecret = "iulfMFYbOZqdgcSHOMeoR0mehcl0yFeO";
const QString SkyDriveClient::clientTypeName = "SkyDriveClient";
const QString SkyDriveClient::SkyDriveRoot = "me/skydrive";

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
const QString SkyDriveClient::sharesURI = "https://apis.live.net/v5.0/%1/shared_read_link";

SkyDriveClient::SkyDriveClient(QObject *parent) :
    QObject(parent)
{
    // Load accessTokenPair from file
    loadAccessPairMap();

    m_propertyReplyHash = new QHash<QString, QString>;
    m_filesReplyHash = new QHash<QString, QString>;
}

SkyDriveClient::~SkyDriveClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_propertyReplyHash = 0;
    m_filesReplyHash = 0;
}

void SkyDriveClient::loadAccessPairMap() {
    QFile file(KeyStoreFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> accessTokenPairMap;

        qDebug() << QTime::currentTime() << "SkyDriveClient::loadAccessPairMap " << accessTokenPairMap;
    }
}

void SkyDriveClient::saveAccessPairMap() {
    // TODO workaround fix to remove tokenPair with key="".
    accessTokenPairMap.remove("");

    // TODO To prevent invalid code to save damage data for testing only.
//    if (accessTokenPairMap.isEmpty()) return;

    QFile file(KeyStoreFilePath);
    QFileInfo info(file);
    if (!info.absoluteDir().exists()) {
        qDebug() << "SkyDriveClient::saveAccessPairMap dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(info.absolutePath());
        if (!res) {
            qDebug() << "SkyDriveClient::saveAccessPairMap can't make dir" << info.absolutePath();
        } else {
            qDebug() << "SkyDriveClient::saveAccessPairMap make dir" << info.absolutePath();
        }
    }    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << accessTokenPairMap;

        qDebug() << "SkyDriveClient::saveAccessPairMap " << accessTokenPairMap;
    }
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
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, clientTypeName);
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
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
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
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

bool SkyDriveClient::isAuthorized()
{
    return (!accessTokenPairMap.isEmpty());
}

QStringList SkyDriveClient::getStoredUidList()
{
    QStringList list;
    foreach (QString s, accessTokenPairMap.keys()) {
        TokenPair t = accessTokenPairMap[s];

        QString jsonText = "{ ";
        jsonText.append( QString("\"uid\": \"%1\", ").arg(s) );
        jsonText.append( QString("\"email\": \"%1\", ").arg(t.email) );
        jsonText.append( QString("\"type\": \"%1\"").arg(clientTypeName) );
        jsonText.append(" }");

        list.append(jsonText);
    }
    return list;
}

int SkyDriveClient::removeUid(QString uid)
{
    qDebug() << "SkyDriveClient::removeUid uid" << uid;
    int n = accessTokenPairMap.remove(uid);
    qDebug() << "SkyDriveClient::removeUid accessTokenPairMap" << accessTokenPairMap;

    return n;
}

void SkyDriveClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- SkyDriveClient::accountInfo ----- uid" << uid;

    QString uri = accountInfoURI;
    QString accessToken;
    if (uid == "") {
        // After accessToken, then uses temp access token..
        accessToken = m_paramMap["access_token"];
    } else {
        accessToken = accessTokenPairMap[uid].token;
    }
    qDebug() << "SkyDriveClient::accountInfo uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessToken).toAscii() );
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
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
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void SkyDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath) {
    qDebug() << "----- SkyDriveClient::fileGet -----" << remoteFilePath << "to" << localFilePath;

    QString uri = fileGetURI.arg(remoteFilePath); // + "?access_token=" + accessTokenPairMap[uid].token;
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::fileGet uri " << uri;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void SkyDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath) {
    qDebug() << "----- SkyDriveClient::filePut -----" << localFilePath << "to" << remoteParentPath;

    QFileInfo localFileInfo(localFilePath);
    QString uri = filePutURI.arg(remoteParentPath).arg(localFileInfo.fileName());
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::filePut uri " << uri;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
        req.setHeader(QNetworkRequest::ContentLengthHeader, fileSize);
        QNetworkReply *reply = manager->put(req, localSourceFile);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

//        qDebug() << "SkyDriveClient::filePut put file" << localFilePath << "to" << remoteParentPath;
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
        emit metadataReplySignal(nonce, -1, "remoteFilePath is empty.", "");
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
        Sleeper().sleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

QNetworkReply * SkyDriveClient::property(QString nonce, QString uid, QString remoteFilePath, bool synchronous, QString callback)
{
    qDebug() << "----- SkyDriveClient::property -----" << remoteFilePath;

    QApplication::processEvents();

    // TODO root dropbox(Full access) or sandbox(App folder access)
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
        Sleeper().sleep(100);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

QNetworkReply * SkyDriveClient::createFolder(QString nonce, QString uid, QString newRemoteFolderName, QString remoteParentPath, bool synchronous)
{
    qDebug() << "----- SkyDriveClient::createFolder -----" << newRemoteFolderName << remoteParentPath;

    if (remoteParentPath.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return 0;
    }

    if (newRemoteFolderName.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return 0;
    }

    QString uri = createFolderURI.arg(remoteParentPath);
    qDebug() << "SkyDriveClient::createFolder uri " << uri;

    QByteArray postData;
    postData.append(QString("{ \"name\": \"%1\" }").arg(newRemoteFolderName));
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

void SkyDriveClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath)
{
    qDebug() << "----- SkyDriveClient::moveFile -----";

    QString uri = moveFileURI;
    qDebug() << "SkyDriveClient::moveFile uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["from_path"] = remoteFilePath;
    sortMap["to_path"] = newRemoteFilePath;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void SkyDriveClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath)
{
    qDebug() << "----- SkyDriveClient::copyFile -----";

    QString uri = copyFileURI;
    qDebug() << "SkyDriveClient::copyFile uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["from_path"] = remoteFilePath;
    sortMap["to_path"] = newRemoteFilePath;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(copyFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

QNetworkReply * SkyDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
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

void SkyDriveClient::fileGetReplyFinished(QNetworkReply *reply) {
    qDebug() << "SkyDriveClient::fileGetReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    // Detect redirection.
    QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
    if(!possibleRedirectUrl.toUrl().isEmpty()) {
        qDebug() << "SkyDriveClient::fileGetReplyFinished redirectUrl" << possibleRedirectUrl.toUrl();

        QNetworkRequest redirectedRequest = reply->request();
        redirectedRequest.setUrl(possibleRedirectUrl.toUrl());
        reply = reply->manager()->get(redirectedRequest);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
        return;
    }

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "SkyDriveClient::fileGetReplyFinished reply bytesAvailable" << reply->bytesAvailable();

        // Stream replyBody to a file on localPath.
        qint64 totalBytes = 0;
        char buf[1024];
        QFile *localTargetFile = m_localFileHash[nonce];
        if (localTargetFile->open(QIODevice::WriteOnly)) {
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

        qDebug() << "SkyDriveClient::fileGetReplyFinished reply totalBytes=" << totalBytes;

        // Close target file.
        localTargetFile->close();
        m_localFileHash.remove(nonce);

        emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), "Request property");
    } else {
        emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::filePutReplyFinished(QNetworkReply *reply) {
    qDebug() << "SkyDriveClient::filePutReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Close source file.
    QFile *localTargetFile = m_localFileHash[nonce];
    localTargetFile->close();
    m_localFileHash.remove(nonce);

    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

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
        m_propertyReplyHash->insert(nonce, QString(reply->readAll()));
    } else {
//        m_propertyReplyHash->insert(nonce, QString(reply->readAll()));

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
        sc = engine.evaluate("(" + m_filesReplyHash->value(nonce) + ")");
        scProperty = engine.evaluate("(" + m_propertyReplyHash->value(nonce) + ")");
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

    if (reply->error() == QNetworkReply::NoError) {
        m_filesReplyHash->insert(nonce, QString(reply->readAll()));
    } else {
        m_filesReplyHash->insert(nonce, QString(reply->readAll()));
    }

    if (m_propertyReplyHash->contains(nonce) && m_filesReplyHash->contains(nonce)) {
        QScriptEngine engine;
        QScriptValue sc;
        QScriptValue scProperty;
        QScriptValue scJsonStringify;
        sc = engine.evaluate("(" + m_filesReplyHash->value(nonce) + ")");
        scProperty = engine.evaluate("(" + m_propertyReplyHash->value(nonce) + ")");
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

    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::moveFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::moveFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit moveFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::copyFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::copyFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::deleteFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void SkyDriveClient::shareFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::shareFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}










