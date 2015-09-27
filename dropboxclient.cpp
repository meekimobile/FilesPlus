#include "dropboxclient.h"
#include <QtGlobal>
#include <QtNetwork>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QScriptEngine>
#include "sleeper.h"

const QString DropboxClient::DropboxConsumerKey = "7y9bttvg6gu57x0"; // Key from DropBox
const QString DropboxClient::DropboxConsumerSecret = "xw5wq7hz5cnd5zo"; // Secret from Dropbox
const QString DropboxClient::DropboxRoot = "dropbox"; // For full access
const QString DropboxClient::SandboxConsumerKey = "u4f161p2wonac7p"; // Key from DropBox
const QString DropboxClient::SandboxConsumerSecret = "itr3zb95dwequun"; // Secret from Dropbox
const QString DropboxClient::SandboxRoot = "sandbox"; // For app folder access, root will be app folder.

DropboxClient::DropboxClient(QObject *parent) :
    CloudDriveClient(parent)
{
    authorizeURI = "https://www.dropbox.com/1/oauth2/authorize";
    accessTokenURI = "https://api.dropbox.com/1/oauth2/token";
    accountInfoURI = "https://api.dropbox.com/1/account/info";
    fileGetURI = "https://api-content.dropbox.com/1/files/%1%2";
    filePutURI = "https://api-content.dropbox.com/1/files_put/%1%2"; // Parameter if any needs to be appended here. ?param=val
    metadataURI = "https://api.dropbox.com/1/metadata/%1%2";
    createFolderURI = "https://api.dropbox.com/1/fileops/create_folder";
    moveFileURI = "https://api.dropbox.com/1/fileops/move";
    copyFileURI = "https://api.dropbox.com/1/fileops/copy";
    deleteFileURI = "https://api.dropbox.com/1/fileops/delete";
    sharesURI = "https://api.dropbox.com/1/shares/%1%2";
    mediaURI = "https://api.dropbox.com/1/media/%1%2"; // POST
    thumbnailURI = "https://api-content.dropbox.com/1/thumbnails/%1%2"; // GET with format and size.
    deltaURI = "https://api.dropbox.com/1/delta"; // POST
    chunkedUploadURI = "https://api-content.dropbox.com/1/chunked_upload"; // PUT with upload_id and offset.
    commitChunkedUploadURI = "https://api-content.dropbox.com/1/commit_chunked_upload/%1%2"; // POST with upload_id.

    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Set default dropbox mode.
    if (!m_settings.contains("dropbox.fullaccess.enabled")) {
        m_settings.setValue("dropbox.fullaccess.enabled", QVariant(true));
        qDebug() << "DropboxClient::DropboxClient set default dropbox.fullaccess.enabled" << m_settings.value("dropbox.fullaccess.enabled", QVariant(true)).toBool();
    }

    qDebug() << "DropboxClient::DropboxClient dropbox.fullaccess.enabled" << m_settings.value("dropbox.fullaccess.enabled", QVariant(true)).toBool();

    isFullAccess = m_settings.value("dropbox.fullaccess.enabled", QVariant(true)).toBool();
    if (isFullAccess) {
        consumerKey = DropboxConsumerKey;
        consumerSecret = DropboxConsumerSecret;
        dropboxRoot = DropboxRoot;
    } else {
        consumerKey = SandboxConsumerKey;
        consumerSecret = SandboxConsumerSecret;
        dropboxRoot = SandboxRoot;
    }

    m_paramMap["oauth_consumer_key"] = consumerKey;
    m_paramMap["oauth_signature_method"] = signatureMethod;

    // Load accessTokenPair from file
    loadAccessPairMap();
}

DropboxClient::~DropboxClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();
}

QByteArray DropboxClient::createBaseString(QString method, QString uri, QString queryString) {
    QByteArray baseString;
    baseString.append(method);
    baseString.append("&");
    baseString.append(QUrl::toPercentEncoding(uri));
    baseString.append("&");
    baseString.append(QUrl::toPercentEncoding(queryString, "", "()"));

    return baseString;
}

QString DropboxClient::createSignature(QString signatureMethod, QString consumerSecret, QString tokenSecret, QByteArray baseString)
{
    QString signature = "";

    if (signatureMethod == "HMAC-SHA1") {
        // Construct key for HMACSHA1 by using consumer 'Secret' and request token secret.
        signature.append(createSignatureWithHMACSHA1(consumerSecret, tokenSecret, baseString));
    } else if (signatureMethod == "PLAINTEXT") {
        // Construct key for PLAINTEXT by using consumer 'Secret' and request token secret.
        signature.append(createSignatureWithPLAINTEXT(consumerSecret, tokenSecret, baseString));
    }
    qDebug() << "DropboxClient::createSignature signature" << signatureMethod << signature;

    return signature;
}

QByteArray DropboxClient::createSignatureWithHMACSHA1(QString consumerSecret, QString tokenSecret, QByteArray baseString)
{
    // Join secrets into key.
    QByteArray key;
    key.append(QUrl::toPercentEncoding(consumerSecret));
    key.append("&");
    key.append(QUrl::toPercentEncoding(tokenSecret));

    int blockSize = 64; // HMAC-SHA-1 block size, defined in SHA-1 standard
    if (key.length() > blockSize) { // if key is longer than block size (64), reduce key length with SHA-1 compression
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    }

    QByteArray innerPadding(blockSize, char(0x36)); // initialize inner padding with char "6"
    QByteArray outerPadding(blockSize, char(0x5c)); // initialize outer padding with char "\"
    // ascii characters 0x36 ("6") and 0x5c ("\") are selected because they have large
    // Hamming distance (http://en.wikipedia.org/wiki/Hamming_distance)

    for (int i = 0; i < key.length(); i++) {
        innerPadding[i] = innerPadding[i] ^ key.at(i); // XOR operation between every byte in key and innerpadding, of key length
        outerPadding[i] = outerPadding[i] ^ key.at(i); // XOR operation between every byte in key and outerpadding, of key length
    }

    // result = hash ( outerPadding CONCAT hash ( innerPadding CONCAT baseString ) ).toBase64
    QByteArray total = outerPadding;
    QByteArray part = innerPadding;
    part.append(baseString);
    total.append(QCryptographicHash::hash(part, QCryptographicHash::Sha1));
    QByteArray hashed = QCryptographicHash::hash(total, QCryptographicHash::Sha1);

    return hashed.toBase64();
}

QByteArray DropboxClient::createSignatureWithPLAINTEXT(QString consumerSecret, QString tokenSecret, QByteArray baseString)
{
    // Join secrets into key.
    QByteArray key;
    key.append(consumerSecret);
    key.append("&");
    key.append(tokenSecret);

    return key;
}

QByteArray DropboxClient::createPostData(QString signature, QString queryString) {
    QByteArray postData;
    postData.append("oauth_signature=");
    postData.append(QUrl::toPercentEncoding(signature));
    postData.append("&");
    postData.append(queryString);

    return postData;
}

QByteArray DropboxClient::createOAuthHeaderString(QMap<QString, QString> sortMap) {
    QByteArray authHeader;
    foreach (QString key, sortMap.keys()) {
        if (authHeader != "") authHeader.append(",");
        authHeader.append(QUrl::toPercentEncoding(key));
        authHeader.append("=\"");
        authHeader.append(QUrl::toPercentEncoding(sortMap[key]));
        authHeader.append("\"");
    }
    authHeader.prepend("OAuth ");

    return authHeader;
}

void DropboxClient::authorize(QString nonce, QString hostname)
{
    qDebug() << "----- DropboxClient::authorize -----";

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["client_id"] = consumerKey;
    sortMap["response_type"] = "code";
    sortMap["redirect_uri"] = "https://sites.google.com/site/meekimobile/support/oauth_callback";
    sortMap["state"] = nonce;
    QString queryString = createQueryString(sortMap);

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, objectName());
}

void DropboxClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- DropboxClient::accessToken -----" << nonce << pin;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["redirect_uri"] = "https://sites.google.com/site/meekimobile/support/oauth_callback";
    sortMap["code"] = pin;
    sortMap["grant_type"] = "authorization_code";
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::accessToken" << nonce << "queryString" << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "DropboxClient::accessToken" << nonce << "postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager->post(req, postData);
}

QByteArray DropboxClient::createOAuthHeaderForUid(QString nonce, QString uid, QString method, QString uri, QMap<QString, QString> addParamMap) {
    if (uid.isEmpty()) {
        qDebug() << "DropboxClient::createOAuthHeaderForUid uid is empty.";
        return QByteArray();
    }

    if (!accessTokenPairMap.contains(uid)) {
        qDebug() << "DropboxClient::createOAuthHeaderForUid uid" << uid << "is not authorized.";
        return QByteArray();
    }

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["oauth_consumer_key"] = consumerKey;
    sortMap["oauth_token"] = accessTokenPairMap[uid].token;
    sortMap["oauth_signature_method"] = signatureMethod;
    sortMap["oauth_timestamp"] = createTimestamp();
    sortMap["oauth_nonce"] = nonce;
    sortMap.unite(addParamMap);
    QString queryString = createNormalizedQueryString(sortMap);
//    qDebug() << "queryString " << queryString;

    // Construct baseString for creating signature.
    QString encodedURI = encodeURI(uri);
//    qDebug() << "encodedURI " << encodedURI;
    QByteArray baseString = createBaseString(method, encodedURI, queryString);
//    qDebug() << "baseString " << baseString;

    QString signature = createSignature(signatureMethod, consumerSecret, accessTokenPairMap[uid].secret, baseString);
    qDebug() << "DropboxClient::createOAuthHeaderForUid signature" << signatureMethod << signature;

    // Set Authorization header with added signature.
    sortMap["oauth_signature"] = signature;
    QByteArray authHeader = createOAuthHeaderString(sortMap);
    qDebug() << "DropboxClient::createOAuthHeaderForUid authHeader" << authHeader;

    return authHeader;
}

void DropboxClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- DropboxClient::accountInfo ----- uid" << uid;

    QString uri = accountInfoURI;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    manager->get(req);
}

void DropboxClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- DropboxClient::quota ----- uid" << uid;

    if (uid == "") {
        emit quotaReplySignal(nonce, -1, "uid is empty", "", 0, 0, -1);
        return;
    }

    QString uri = accountInfoURI;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    manager->get(req);
}

QIODevice *DropboxClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- DropboxClient::fileGet -----" << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    QString uri = fileGetURI.arg(dropboxRoot, remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::fileGet" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "DropboxClient::fileGet" << nonce << "rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    reply = getRedirectedReply(reply);

    if (!synchronous) {
        // Wait until readyRead().
        QEventLoop loop;
        connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));
        loop.exec();

        fileGetReplyFinished(reply, synchronous);
    } else {
        // Wait until finished().
        while (!reply->isFinished()) {
            QApplication::processEvents(QEventLoop::AllEvents, 100);
            Sleeper::msleep(100);
        }

        // Remove finished reply from hash.
        m_replyHash->remove(nonce);
    }

    return reply;
}

QString DropboxClient::getDefaultLocalFilePath(const QString &remoteFilePath)
{
    QRegExp rx("^(/*)([C-F])(.+)$");
    rx.indexIn(remoteFilePath);
    if (rx.captureCount() == 3) {
        return rx.cap(2).append(":").append(rx.cap(3));
    } else if (rx.captureCount() == 2) {
        return rx.cap(1).append(":").append(rx.cap(2));
    }
    return "";
}

QNetworkReply *DropboxClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- DropboxClient::filePut -----" << nonce << uid << remoteParentPath << remoteFileName << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString uri = filePutURI.arg(dropboxRoot, remoteParentPath + "/" + remoteFileName);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::filePut uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
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

QString DropboxClient::thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format, QString size)
{
    if (uid.isEmpty()) {
        qDebug() << "DropboxClient::thumbnail" << nonce << "uid is empty.";
        return "";
    }

    if (!accessTokenPairMap.contains(uid)) {
        qDebug() << "DropboxClient::thumbnail" << nonce << "uid" << uid << "is not authorized.";
        return "";
    }

    QString uri = thumbnailURI.arg(dropboxRoot, remoteFilePath);

    QMap<QString, QString> sortMap;
    sortMap["access_token"] = accessTokenPairMap[uid].token;
    sortMap["format"] = format;
    sortMap["size"] = size;

    QString queryString = createNormalizedQueryString(sortMap);
    QString encodedURI = encodeURI(uri);
    QString url = encodedURI + "?" + queryString;

    qDebug() << "DropboxClient::thumbnail" << nonce << "url" << url;
    return url;
}

QString DropboxClient::media(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- DropboxClient::media -----" << nonce << uid << remoteFilePath;

    if (remoteFilePath.isEmpty()) {
        qDebug() << "DropboxClient::media" << nonce << uid << "remoteFilePath is empty. Operation is aborted.";
        return "";
    }

    QString uri = mediaURI.arg(dropboxRoot, remoteFilePath);
//    uri = encodeURI(uri);
    qDebug() << "DropboxClient::media" << nonce << "uri " << uri;

    QByteArray postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return mediaReplyFinished(reply);
}

QString DropboxClient::getDefaultRemoteFilePath(const QString &localFilePath)
{
    QRegExp rx("^([C-F])(:)(/.+)$");
    rx.indexIn(localFilePath);
    if (rx.captureCount() == 3) {
        return "/" + rx.cap(1).append(rx.cap(3));
    }
    return "";
}

bool DropboxClient::isRemoteAbsolutePath()
{
    return true;
}

bool DropboxClient::isRemotePathCaseInsensitive()
{
    return true;
}

bool DropboxClient::isFilePutResumable(qint64 fileSize)
{
    return (fileSize == -1 || fileSize >= getChunkSize());
}

bool DropboxClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize == -1 || fileSize >= getChunkSize());
}

bool DropboxClient::isDeltaSupported()
{
    return true;
}

bool DropboxClient::isDeltaEnabled(QString uid)
{
    return m_settings.value(QString("%1.%2.delta.enabled").arg(objectName()).arg(uid), QVariant(false)).toBool();
}

bool DropboxClient::isViewable()
{
    return true;
}

bool DropboxClient::isImageUrlCachable()
{
    return true;
}

QNetworkReply * DropboxClient::property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous, QString callback)
{
    qDebug() << "----- DropboxClient::property -----" << nonce << uid << remoteFilePath << synchronous;

    // root dropbox(Full access) or sandbox(App folder access)
    QString uri = metadataURI.arg(dropboxRoot).arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::property" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        if (callback == "metadata") {
            connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(metadataReplyFinished(QNetworkReply*)));
        } else if (callback == "browse") {
            connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(browseReplyFinished(QNetworkReply*)));
        }
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
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

void DropboxClient::metadata(QString nonce, QString uid, QString remoteFilePath) {
    qDebug() << "----- DropboxClient::metadata -----" << nonce << uid << remoteFilePath;

    // root dropbox(Full access) or sandbox(App folder access)
    QString uri = metadataURI.arg(dropboxRoot).arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::metadata" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(metadataReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    manager->get(req);
}

void DropboxClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- DropboxClient::browse -----" << nonce << uid << remoteFilePath;

    // root dropbox(Full access) or sandbox(App folder access)
    QString uri = metadataURI.arg(dropboxRoot).arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::browse" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(browseReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    manager->get(req);
}

void DropboxClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- DropboxClient::moveFile -----" << nonce << uid << remoteFilePath << newRemoteParentPath << newRemoteFileName;

    QString newRemoteFilePath;
    if (newRemoteParentPath == "") {
        // Rename
        newRemoteFilePath = removeDoubleSlash(getParentRemotePath(remoteFilePath) + "/" + newRemoteFileName);
        qDebug() << "DropboxClient::moveFile" << nonce << "rename remoteFilePath" << remoteFilePath << "newRemoteFilePath" << newRemoteFilePath;
    } else {
        // Move
        newRemoteFilePath = removeDoubleSlash(newRemoteParentPath + "/" + newRemoteFileName);
        qDebug() << "DropboxClient::moveFile" << nonce << "move remoteFilePath" << remoteFilePath << "newRemoteFilePath" << newRemoteFilePath;
    }

    QString uri = moveFileURI;
    qDebug() << "DropboxClient::moveFile" << nonce << "uri" << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
    sortMap["from_path"] = remoteFilePath;
    sortMap["to_path"] = newRemoteFilePath;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::moveFile" << nonce << "queryString" << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "DropboxClient::moveFile" << nonce << "postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager->post(req, postData);
}

void DropboxClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- DropboxClient::copyFile -----" << nonce << uid << remoteFilePath << newRemoteParentPath << newRemoteFileName;

    QString uri = copyFileURI;
    qDebug() << "DropboxClient::copyFile" << nonce << "uri" << uri;

    QString newRemoteFilePath = removeDoubleSlash(newRemoteParentPath + "/" + newRemoteFileName);
    qDebug() << "DropboxClient::copyFile" << nonce << "remoteFilePath" << remoteFilePath << "newRemoteFilePath" << newRemoteFilePath;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
    sortMap["from_path"] = remoteFilePath;
    sortMap["to_path"] = newRemoteFilePath;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::copyFile" << nonce << "queryString" << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "DropboxClient::copyFile" << nonce << "postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(copyFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager->post(req, postData);
}

QString DropboxClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- DropboxClient::deleteFile -----" << nonce << uid << remoteFilePath << synchronous;

    QString uri = deleteFileURI;
    qDebug() << "DropboxClient::deleteFile" << nonce << "uri" << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
    sortMap["path"] = remoteFilePath;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::deleteFile" << nonce <<"queryString" << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "DropboxClient::deleteFile" << nonce << "postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deleteFileReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);

    // Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return deleteFileReplyFinished(reply, synchronous);
}

QString DropboxClient::shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- DropboxClient::shareFile -----" << nonce << uid << remoteFilePath << synchronous;

    if (remoteFilePath.isEmpty()) {
        if (!synchronous) emit shareFileReplySignal(nonce, -1, "remoteFilePath is empty.", "", "", 0);
        return "";
    }

    // root dropbox(Full access) or sandbox(App folder access)
    QString uri = sharesURI.arg(dropboxRoot).arg(remoteFilePath);
    qDebug() << "DropboxClient::shareFile" << nonce << "uri" << uri;

    QByteArray postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);

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

QString DropboxClient::delta(QString nonce, QString uid, bool synchronous)
{
    qDebug() << "----- DropboxClient::delta -----" << nonce << uid << synchronous;

    QString uri = deltaURI;
    qDebug() << "DropboxClient::delta" << nonce << "uri" << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["cursor"] = m_settings.value(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid)).toString();
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::delta" << nonce << "queryString" << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "DropboxClient::delta" << nonce << "postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deltaReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);

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

QIODevice *DropboxClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- DropboxClient::fileGetResume -----" << nonce << uid << remoteFilePath << localFilePath << offset;

    QString uri = fileGetURI.arg(dropboxRoot, remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::fileGetResume" << nonce << "uri" << uri;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetResumeReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "DropboxClient::fileGetResume" << nonce << "rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    return reply;
}

QNetworkReply * DropboxClient::filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset)
{
    qDebug() << "----- DropboxClient::filePutResume -----" << nonce << uid << localFilePath << remoteParentPath << remoteFileName << uploadId << offset;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        localSourceFile->seek(offset);

        // Send request.
        filePutResumeUpload(nonce, uid, localSourceFile, remoteFileName, fileSize, uploadId, offset, false);
        return 0;
    } else {
        qDebug() << "DropboxClient::filePutResume" << nonce << "file" << localFilePath << "can't be opened.";
        emit filePutResumeReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");

        return 0;
    }
}

QString DropboxClient::filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous)
{
    return "";
}

QString DropboxClient::filePutResumeUpload(QString nonce, QString uid, QIODevice *source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    /*
     *NOTE
     *source must be seeked to required offset before invoking this method.
     *bytesTotal is total source file size.
     *offset is uploading offset.
     */
    qDebug() << "----- DropboxClient::filePutResumeUpload -----" << nonce << uid << uploadId << offset << "synchronous" << synchronous;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    if (uploadId != "") sortMap["upload_id"] = uploadId;
    if (offset >= 0) sortMap["offset"] = QString("%1").arg(offset);
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::filePutResumeUpload" << nonce << "queryString" << queryString;

    QString uri = chunkedUploadURI;
    QUrl url = QUrl(uri + "?" + queryString);
    qDebug() << "DropboxClient::filePutResumeUpload" << nonce << "url" << url;

    qint64 chunkSize = qMin(bytesTotal-offset, getChunkSize());
    qDebug() << "DropboxClient::filePutResumeUpload" << nonce << "source->size()" << source->size() << "bytesTotal" << bytesTotal << "offset" << offset << "chunkSize" << chunkSize;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(url);
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, chunkSize);
    QNetworkReply *reply = manager->put(req, source);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    // Return immediately if it's not synchronous.
    if (!synchronous) {
        return "";
    }

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Construct result.
    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        result = QString::fromUtf8(reply->readAll());
    } else if (reply->error() == 400) { // Check if it's incorrect offset, then re-queue job.
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + result + ")");
        if (sc.property("offset").isValid()) {
            result = QString::fromUtf8(reply->readAll());
        } else {
            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
        }
    } else {
        qDebug() << "DropboxClient::filePutResumeUpload nonce" << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString DropboxClient::filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    /*
     *NOTE
     *source must be seeked to required offset before invoking this method.
     *bytesTotal is total source file size.
     *offset is uploading offset.
     */
    qDebug() << "----- DropboxClient::filePutResumeStatus -----" << nonce << uid << uploadId << offset << "synchronous" << synchronous;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    if (uploadId != "") sortMap["upload_id"] = uploadId;
    if (offset >= 0) sortMap["offset"] = QString("%1").arg(offset);
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::filePutResumeStatus" << nonce << "queryString" << queryString;

    QString uri = chunkedUploadURI;
    QUrl url = QUrl(uri + "?" + queryString);
    qDebug() << "DropboxClient::filePutResumeStatus" << nonce << "url" << url;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(url);
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, 0);
    QNetworkReply *reply = manager->put(req, QByteArray());
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Return immediately if it's not synchronous.
    if (!synchronous) return "";

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Construct result.
    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        result = QString::fromUtf8(reply->readAll());
    } else {
        result = QString::fromUtf8(reply->readAll());
        qDebug() << "DropboxClient::filePutResumeStatus" << nonce << reply->error() << reply->errorString() << result;
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + result + ")");
        if (sc.property("offset").isValid()) {
            qint64 offset = sc.property("offset").toInteger();
            result = QString("{ \"offset\": %1 }").arg(offset);
        } else {
            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
        }
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString DropboxClient::filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous)
{
    qDebug() << "----- DropboxClient::filePutCommit -----" << nonce << uid << remoteFilePath << uploadId << "synchronous" << synchronous;

    QString uri = commitChunkedUploadURI.arg(dropboxRoot, remoteFilePath);
//    uri = encodeURI(uri); // Post doesn't need to encode URI before request.
    qDebug() << "DropboxClient::filePutCommit" << nonce << "uri" << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["upload_id"] = uploadId;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::filePutCommit" << nonce << "queryString" << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
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
        result = QString::fromUtf8(reply->readAll());
    } else {
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString DropboxClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- DropboxClient::createFolder -----" << uid << remoteParentPath << newRemoteFolderName << synchronous;

    if (remoteParentPath.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
    }

    QString uri = createFolderURI;
    qDebug() << "DropboxClient::createFolder" << nonce << "uri" << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
    sortMap["path"] = remoteParentPath + "/" + newRemoteFolderName;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "DropboxClient::createFolder" << nonce << "queryString" << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "DropboxClient::createFolder" << nonce << "postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(sortMap["path"]));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);

    // Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return createFolderReplyFinished(reply, synchronous);
}

void DropboxClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::accessTokenReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Load response parameters into map.
    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        m_paramMap["access_token"] = sc.property("access_token").toString();
        m_paramMap["uid"] = sc.property("uid").toString();
        qDebug() << "DropboxClient::accessTokenReplyFinished m_paramMap " << m_paramMap;

        if (m_paramMap["uid"] != "" && m_paramMap["access_token"] != "") {
            TokenPair accessTokenPair;
            accessTokenPair.token = m_paramMap["access_token"];

            accessTokenPairMap[m_paramMap["uid"]] = accessTokenPair;
        }

        // Save tokens.
        saveAccessPairMap();

        // Get email from accountInfo will be requested by CloudDriveModel.accessTokenReplyFilter().
    }

    emit accessTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::accountInfoReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        QString uid = sc.property("uid").toString();
        QString email = sc.property("email").toString();

        if (uid != "" && accessTokenPairMap.contains(uid)) {
            accessTokenPairMap[uid].email = email;
            qDebug() << "DropboxClient::accountInfoReplyFinished nonce" << nonce << "uid" << uid << "is updated with email" << email;

            // Save account after got id and email.
            saveAccessPairMap();
        } else {
            qDebug() << "DropboxClient::accountInfoReplyFinished nonce" << nonce << "uid" << uid << "is not found. Account is not updated.";
        }
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::quotaReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::quotaReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
        qint64 sharedValue = jsonObj.property("quota_info").property("shared").toInteger();
        qint64 normalValue = jsonObj.property("quota_info").property("normal").toInteger();
        qint64 quotaValue = jsonObj.property("quota_info").property("quota").toInteger();

        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, normalValue, sharedValue, quotaValue);
    } else {
        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, 0, 0, -1);
    }

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString DropboxClient::fileGetReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        QString metadata;
        metadata.append(reply->rawHeader("x-dropbox-metadata"));
        qDebug() << "DropboxClient::fileGetReplyResult" << nonce << "x-dropbox-metadata" << metadata;

        // Return common json.
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + metadata + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        result = stringifyScriptValue(engine, parsedObj);
    } else {
        qDebug() << "DropboxClient::fileGetReplyResult" << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\", \"http_status_code\": %3 }")
                .arg(reply->error())
                .arg(reply->errorString())
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    }

    return result;
}

QString DropboxClient::filePutReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "DropboxClient::filePutReplyResult" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        // Return common json.
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    return replyBody;
}

void DropboxClient::metadataReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    qDebug() << "DropboxClient::metadataReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    if (m_settings.value("Logging.enabled", false).toBool()) {
        qDebug() << "DropboxClient::metadataReplyFinished replyBody" << replyBody;
    }
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        engine.globalObject().setProperty("nonce", QScriptValue(nonce));
        engine.globalObject().setProperty("uid", QScriptValue(uid));
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        parsedObj.setProperty("children", engine.newArray());
        int contentsCount = jsonObj.property("contents").property("length").toInteger();
        for (int i = 0; i < contentsCount; i++) {
            QApplication::processEvents();

            parsedObj.property("children").setProperty(i, parseCommonPropertyScriptValue(engine, jsonObj.property("contents").property(i)));
        }

        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "DropboxClient::metadataReplyFinished" << nonce << "stringifyScriptValue started.";
        replyBody = stringifyScriptValue(engine, parsedObj);
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "DropboxClient::metadataReplyFinished" << nonce << "stringifyScriptValue done." << replyBody.size();
    }

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::browseReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    qDebug() << "DropboxClient::browseReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    if (m_settings.value("Logging.enabled", false).toBool()) {
        qDebug() << "DropboxClient::browseReplyFinished replyBody" << replyBody;
    }
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        engine.globalObject().setProperty("nonce", QScriptValue(nonce));
        engine.globalObject().setProperty("uid", QScriptValue(uid));
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        parsedObj.setProperty("children", engine.newArray());
        int contentsCount = jsonObj.property("contents").property("length").toInteger();
        for (int i = 0; i < contentsCount; i++) {
            QApplication::processEvents();

            parsedObj.property("children").setProperty(i, parseCommonPropertyScriptValue(engine, jsonObj.property("contents").property(i)));
        }

        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "DropboxClient::browseReplyFinished" << nonce << "stringifyScriptValue started.";
        replyBody = stringifyScriptValue(engine, parsedObj);
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "DropboxClient::browseReplyFinished" << nonce << "stringifyScriptValue done." << replyBody.size();
    }

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString DropboxClient::createFolderReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qDebug() << "DropboxClient::createFolderReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "DropboxClient::createFolderReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    } else if (reply->error() == QNetworkReply::ContentOperationNotPermittedError) {
        // Get property as there is existing folder with the same name.
        reply = property(nonce, uid, remoteFilePath, true);
        replyBody = QString::fromUtf8(reply->readAll());
        qDebug() << "DropboxClient::createFolderReplyFinished" << nonce << "property replyBody" << replyBody;
        if (reply->error() == QNetworkReply::NoError) {
            QScriptEngine engine;
            QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
            QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
            replyBody = stringifyScriptValue(engine, parsedObj);
        }
    }

    // Emit signal only for asynchronous request. To avoid invoking CloudDriveModel.createFolderReplyFilter() as it's not required. And also avoid invoking jobDone() which causes issue #FP20130232.
    if (!synchronous) emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

void DropboxClient::moveFileReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::moveFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "DropboxClient::moveFileReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit moveFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::copyFileReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::copyFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "DropboxClient::copyFileReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString DropboxClient::deleteFileReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::deleteFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "DropboxClient::deleteFileReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    if (!synchronous) emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

QString DropboxClient::shareFileReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::shareFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());
    QScriptEngine engine;
    QScriptValue sc;
    QString url = "";
    int expires = 0;

    if (reply->error() == QNetworkReply::NoError) {
        sc = engine.evaluate("(" + replyBody + ")");
        url = sc.property("url").toString();
        expires = -1;
    }

    if (!synchronous) emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, url, expires);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return url;
}

QString DropboxClient::deltaReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    qDebug() << "DropboxClient::deltaReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "DropboxClient::deltaReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sourceObj = engine.evaluate("(" + replyBody + ")");

        // Check reset state.
        bool syncOnReset = m_settings.value(QString("%1.%2.syncOnReset").arg(objectName()).arg(uid), QVariant(false)).toBool();
        bool isReset = sourceObj.property("reset").toBool() || m_settings.value(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(false)).toBool();
        if (isReset) {
            qDebug() << "DropboxClient::deltaReplyFinished" << nonce << "isReset" << isReset << "set to update state only.";
            m_settings.setValue(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(true));
        }

        // Get entries count.
        int entriesCount = sourceObj.property("entries").property("length").toInteger();
        qDebug() << "DropboxClient::deltaReplyFinished" << nonce << "entriesCount" << entriesCount;

        // Process sourceObj.
        QScriptValue parsedObj = engine.newObject();
        if (entriesCount > 0) {
            QScriptValue childrenObj = engine.newArray();
            for (int i = 0; (!isReset || syncOnReset) && i < entriesCount; i++) {
                QScriptValue sourceChildObj = sourceObj.property("entries").property(i);
                QString parentPath = getParentRemotePath(sourceChildObj.property(0).toString());
                qDebug() << "DropboxClient::deltaReplyFinished" << nonce << "parsedChildObj" <<  sourceChildObj.property(0).toString() << sourceChildObj.property(1).toString();

                QScriptValue parsedChildObj = engine.newObject();
                parsedChildObj.setProperty("isDeleted", sourceChildObj.property(1).isNull());
                parsedChildObj.setProperty("absolutePath", sourceChildObj.property(0));
                parsedChildObj.setProperty("parentPath", QScriptValue(parentPath));
                // Suppress as it's not used in CDM.deltaReplyFilter().
//                if (!sourceChildObj.property(1).isNull()) {
//                    qDebug() << "DropboxClient::deltaReplyFinished sourceChildObj isNull" << sourceChildObj.property(1).isNull() << "update state only.";
//                    parsedChildObj.setProperty("property", parseCommonPropertyScriptValue(engine, sourceChildObj.property(1)));
//                }
                childrenObj.setProperty(i, parsedChildObj);
            }
            parsedObj.setProperty("children", childrenObj);
        }
        parsedObj.setProperty("nextDeltaCursor", sourceObj.property("cursor"));
        parsedObj.setProperty("reset", sourceObj.property("reset"));

        // Check hasMore and reset state.
        bool hasMore = sourceObj.property("has_more").toBool();
        parsedObj.setProperty("hasMore", QScriptValue(hasMore));
        if (isReset) {
            if (hasMore) {
                qDebug() << "DropboxClient::deltaReplyFinished" << nonce << "hasMore" << hasMore << "proceed update state only.";
            } else {
                qDebug() << "DropboxClient::deltaReplyFinished" << nonce << "hasMore" << hasMore << "reset to normal state.";
                m_settings.setValue(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(false));
            }
        }

        // Save nextDeltaCursor
        m_settings.setValue(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid), parsedObj.property("nextDeltaCursor").toVariant());

//        qDebug() << "DropboxClient::deltaReplyFinished parsedObj" << stringifyScriptValue(engine, parsedObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit deltaReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

QString DropboxClient::mediaReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::mediaReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());
    QScriptEngine engine;
    QScriptValue sc;
    QString url = "";
    int expires = 0;

    qDebug() << "DropboxClient::mediaReplyFinished nonce" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        sc = engine.evaluate("(" + replyBody + ")");
        url = sc.property("url").toString();
        expires = -1;
    }

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return url;
}

void DropboxClient::filePutResumeReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "DropboxClient::filePutResumeReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    // Close source file.
    if (m_localFileHash.contains(nonce)) {
        QFile *localTargetFile = m_localFileHash[nonce];
        localTargetFile->close();
        m_localFileHash.remove(nonce);
    }

    emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString DropboxClient::getParentRemotePath(QString remotePath)
{
    QString remoteParentPath = "";
    if (remotePath != "" && remotePath != "/") {
        remoteParentPath = remotePath.mid(0, remotePath.lastIndexOf("/"));
        remoteParentPath = (remoteParentPath == "") ? "/" : remoteParentPath;
    }

    return remoteParentPath;
}

QString DropboxClient::getRemoteName(QString remotePath) {
    if (remotePath != "") {
        return remotePath.mid(remotePath.lastIndexOf("/") + 1);
    } else {
        return remotePath;
    }
}

QScriptValue DropboxClient::parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj)
{
    QScriptValue parsedObj = engine.newObject();

    QString nonce = engine.globalObject().property("nonce").toString();
    QString uid = engine.globalObject().property("uid").toString();

    QString thumbnailUrl = jsonObj.property("thumb_exists").toBool() ? thumbnail(nonce, uid, jsonObj.property("path").toString(), "png", "s") : "";
    QString thumbnail128Url = jsonObj.property("thumb_exists").toBool() ? thumbnail(nonce, uid, jsonObj.property("path").toString(), "png", "m") : "";
    QString previewUrl = jsonObj.property("thumb_exists").toBool() ? thumbnail(nonce, uid, jsonObj.property("path").toString(), "png", "l") : "";

    QString jsonDateString = formatJSONDateString(parseReplyDateString(jsonObj.property("modified").toString()));

    parsedObj.setProperty("name", QScriptValue(getRemoteName(jsonObj.property("path").toString())));
    parsedObj.setProperty("absolutePath", jsonObj.property("path"));
    parsedObj.setProperty("parentPath", QScriptValue(getParentRemotePath(jsonObj.property("path").toString())));
    parsedObj.setProperty("size", jsonObj.property("bytes"));
    parsedObj.setProperty("isDeleted", jsonObj.property("is_deleted"));
    parsedObj.setProperty("isDir", jsonObj.property("is_dir"));
    parsedObj.setProperty("lastModified", QScriptValue(jsonDateString));
    parsedObj.setProperty("hash", jsonObj.property("hash").isValid() ? jsonObj.property("hash") : jsonObj.property("rev"));
    parsedObj.setProperty("source", QScriptValue());
    parsedObj.setProperty("thumbnail", QScriptValue(thumbnailUrl));
    parsedObj.setProperty("thumbnail128", QScriptValue(thumbnail128Url));
    parsedObj.setProperty("preview", QScriptValue(previewUrl));
    parsedObj.setProperty("fileType", QScriptValue(getFileType(jsonObj.property("path").toString())));

    return parsedObj;
}

QDateTime DropboxClient::parseReplyDateString(QString dateString)
{
    // NOTE Dropbox uses UTC datestring in its reply.
    QString filteredDateString = dateString;
    QDateTime datetime = QDateTime::fromString(filteredDateString, "ddd, dd MMM yyyy hh:mm:ss +0000");
    qDebug() << "DropboxClient::parseReplyDateString parse filteredDateString" << filteredDateString << "to" << datetime;
    datetime.setTimeSpec(Qt::UTC);
    qDebug() << "DropboxClient::parseReplyDateString parse datetime.setTimeSpec(Qt::UTC)" << datetime;

    return datetime;
}
