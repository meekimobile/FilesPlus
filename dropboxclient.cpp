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

//const QString DropboxClient::signatureMethod = "HMAC-SHA1"; // Failed since 1-Sep-12
const QString DropboxClient::signatureMethod = "PLAINTEXT";
const QString DropboxClient::requestTokenURI = "https://api.dropbox.com/1/oauth/request_token";
const QString DropboxClient::authorizeURI = "https://www.dropbox.com/1/oauth/authorize";
const QString DropboxClient::accessTokenURI = "https://api.dropbox.com/1/oauth/access_token";
const QString DropboxClient::accountInfoURI = "https://api.dropbox.com/1/account/info";
const QString DropboxClient::fileGetURI = "https://api-content.dropbox.com/1/files/%1%2";
const QString DropboxClient::filePutURI = "https://api-content.dropbox.com/1/files_put/%1%2"; // Parameter if any needs to be appended here. ?param=val
const QString DropboxClient::metadataURI = "https://api.dropbox.com/1/metadata/%1%2";
const QString DropboxClient::createFolderURI = "https://api.dropbox.com/1/fileops/create_folder";
const QString DropboxClient::moveFileURI = "https://api.dropbox.com/1/fileops/move";
const QString DropboxClient::copyFileURI = "https://api.dropbox.com/1/fileops/copy";
const QString DropboxClient::deleteFileURI = "https://api.dropbox.com/1/fileops/delete";
const QString DropboxClient::sharesURI = "https://api.dropbox.com/1/shares/%1%2";
const QString DropboxClient::mediaURI = "https://api.dropbox.com/1/media/%1%2";
const QString DropboxClient::thumbnailURI = "https://api-content.dropbox.com/1/thumbnails/%1%2"; // GET with format and size.

DropboxClient::DropboxClient(QObject *parent, bool fullAccess) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    isFullAccess = fullAccess;
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

QString DropboxClient::createTimestamp() {
    qint64 seconds = QDateTime::currentMSecsSinceEpoch() / 1000;

    return QString("%1").arg(seconds);
}

QString DropboxClient::createNonce() {
    QString ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString nonce;

    for(int i = 0; i <= 16; ++i)
    {
        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
    }

    return nonce;
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
        signature = createSignatureWithHMACSHA1(consumerSecret, tokenSecret, baseString);
    } else if (signatureMethod == "PLAINTEXT") {
        // Construct key for PLAINTEXT by using consumer 'Secret' and request token secret.
        signature = createSignatureWithPLAINTEXT(consumerSecret, tokenSecret, baseString);
    }
    qDebug() << "DropboxClient::createSignature signature" << signatureMethod << signature;

    return signature;
}

QString DropboxClient::createSignatureWithHMACSHA1(QString consumerSecret, QString tokenSecret, QByteArray baseString)
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

QString DropboxClient::createSignatureWithPLAINTEXT(QString consumerSecret, QString tokenSecret, QByteArray baseString)
{
    // Join secrets into key.
    QByteArray key;
    key.append(consumerSecret);
    key.append("&");
    key.append(tokenSecret);

    return key;
}

QString DropboxClient::createNormalizedQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(QUrl::toPercentEncoding(key)).append("=").append(QUrl::toPercentEncoding(sortMap[key]));
    }

    return queryString;
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

void DropboxClient::requestToken(QString nonce)
{
    qDebug() << "----- DropboxClient::requestToken -----";

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["oauth_consumer_key"] = m_paramMap["oauth_consumer_key"];
    sortMap["oauth_signature_method"] = m_paramMap["oauth_signature_method"];
    sortMap["oauth_timestamp"] = createTimestamp();
    sortMap["oauth_nonce"] = nonce;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    // Construct baseString for creating signature.
    QByteArray baseString = createBaseString("POST", requestTokenURI, queryString);
    qDebug() << "baseString " << baseString;

    // Construct key for HMACSHA1 or PLAINTEXT by using consumer 'Secret' and no request token secret.
    QString signature = createSignature(signatureMethod, consumerSecret, "", baseString);
    qDebug() << "signature " << signature;

    QByteArray postData = createPostData(signature, queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(requestTokenURI));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
}

void DropboxClient::authorize(QString nonce)
{
    qDebug() << "----- DropboxClient::authorize -----";

    QString queryString;
    queryString.append("oauth_token=" + m_paramMap["oauth_token"]);
    queryString.append("&oauth_callback=" + m_paramMap["oauth_callback"]);
    queryString.append("&display=mobile");
    // TODO Force with locale=en.
//    queryString.append("&locale=en");

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, this->metaObject()->className());
}

void DropboxClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- DropboxClient::accessToken -----";

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["oauth_consumer_key"] = consumerKey;
    sortMap["oauth_token"] = requestTokenPair.token;
    sortMap["oauth_signature_method"] = signatureMethod;
    sortMap["oauth_timestamp"] = createTimestamp();
    sortMap["oauth_nonce"] = nonce;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    // Construct baseString for creating signature.
    QByteArray baseString = createBaseString("POST", accessTokenURI, queryString);
    qDebug() << "baseString " << baseString;

    // Construct key for HMACSHA1 or PLAINTEXT by using consumer 'Secret' and request token secret.
    QString signature = createSignature(signatureMethod, consumerSecret, requestTokenPair.secret, baseString);
    qDebug() << "signature " << signature;

    QByteArray postData = createPostData(signature, queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
}

QString DropboxClient::encodeURI(const QString uri) {
    // Example: https://api.dropbox.com/1/metadata/sandbox/C/B/NES/Solomon's Key (E) [!].nes
    // All non-alphanumeric except : and / must be encoded.
    return QUrl::toPercentEncoding(uri, ":/");
}

QByteArray DropboxClient::createOAuthHeaderForUid(QString nonce, QString uid, QString method, QString uri, QMap<QString, QString> addParamMap) {
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
    qDebug() << "signature" << signatureMethod << signature;

    // Set Authorization header with added signature.
    sortMap["oauth_signature"] = signature;
    QByteArray authHeader = createOAuthHeaderString(sortMap);
    qDebug() << "authHeader " << authHeader;

    return authHeader;
}

void DropboxClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- DropboxClient::accountInfo ----- uid" << uid;

    QString uri = accountInfoURI;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "GET", uri));
    QNetworkReply *reply = manager->get(req);
}

void DropboxClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- DropboxClient::quota ----- uid" << uid;

    QString uri = accountInfoURI;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "GET", uri));
    QNetworkReply *reply = manager->get(req);
}

void DropboxClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath) {
    qDebug() << "----- DropboxClient::fileGet -----" << uid << remoteFilePath << localFilePath;

    QString uri = fileGetURI.arg(dropboxRoot, remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::fileGet uri " << uri;

    // Create localTargetFile for file getting.
//    QFile localTargetFile(localFilePath);
//    m_localFileHash[nonce] = &localTargetFile;
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "GET", uri));
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

QIODevice *DropboxClient::fileGet(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- DropboxClient::fileGet -----" << uid << remoteFilePath;

    QString uri = fileGetURI.arg(dropboxRoot, remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::fileGet uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "GET", uri));
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
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

void DropboxClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath) {
    qDebug() << "----- DropboxClient::filePut -----" << uid << localFilePath << remoteFilePath;

    QString uri = filePutURI.arg(dropboxRoot, remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::filePut uri " << uri;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
        QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "PUT", uri));
        req.setHeader(QNetworkRequest::ContentLengthHeader, fileSize);
        QNetworkReply *reply = manager->put(req, localSourceFile);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

//        qDebug() << "DropboxClient::filePut put file " << localFilePath << " to dropbox " << remoteFilePath;
    } else {
        qDebug() << "DropboxClient::filePut file " << localFilePath << " can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
    }
}

QNetworkReply *DropboxClient::filePut(QString nonce, QString uid, QIODevice *source, QString remoteParentPath, QString remoteFileName)
{
    qDebug() << "----- DropboxClient::filePut -----" << uid << remoteParentPath << remoteFileName << "source->bytesAvailable()" << source->bytesAvailable();

    QString uri = filePutURI.arg(dropboxRoot, remoteParentPath + "/" + remoteFileName);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::filePut uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "PUT", uri));
    req.setHeader(QNetworkRequest::ContentLengthHeader, source->bytesAvailable());
    QNetworkReply *reply = manager->put(req, source);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    return reply;
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

void DropboxClient::metadata(QString nonce, QString uid, QString remoteFilePath) {
    qDebug() << "----- DropboxClient::metadata -----";

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = metadataURI.arg(dropboxRoot).arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::metadata uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(metadataReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "GET", uri));
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void DropboxClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- DropboxClient::browse -----" << remoteFilePath;

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = metadataURI.arg(dropboxRoot).arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "DropboxClient::browse uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(browseReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "GET", uri));
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void DropboxClient::createFolder(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteFilePath, newRemoteFolderName, false);
//    qDebug() << "----- DropboxClient::createFolder -----" << uid << remoteFilePath << newRemoteFolderName;

//    QString uri = createFolderURI;
//    qDebug() << "DropboxClient::createFolder uri " << uri;

//    // Construct normalized query string.
//    QMap<QString, QString> sortMap;
//    sortMap["root"] = dropboxRoot;
//    sortMap["path"] = remoteFilePath + "/" + newRemoteFolderName;
//    QString queryString = createNormalizedQueryString(sortMap);
//    qDebug() << "queryString " << queryString;

//    QByteArray postData;
//    postData.append(queryString);
//    qDebug() << "postData" << postData;

//    // Send request.
//    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
//    QNetworkRequest req = QNetworkRequest(QUrl(uri));
//    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
//    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
//    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//    QNetworkReply *reply = manager->post(req, postData);
//    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
//    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
//    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void DropboxClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    qDebug() << "----- DropboxClient::moveFile -----" << uid << remoteFilePath << newRemoteFilePath << newRemoteFileName;

    if (newRemoteFileName != "") {
        newRemoteFilePath = getParentRemotePath(remoteFilePath) + "/" + newRemoteFileName;
        qDebug() << "DropboxClient::moveFile remoteFilePath" << remoteFilePath << "newRemoteFilePath" << newRemoteFilePath;
    }

    QString uri = moveFileURI;
    qDebug() << "DropboxClient::moveFile uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
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
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void DropboxClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    qDebug() << "----- DropboxClient::copyFile -----" << uid << remoteFilePath << newRemoteFilePath << newRemoteFileName;

    QString uri = copyFileURI;
    qDebug() << "DropboxClient::copyFile uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
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
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void DropboxClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- DropboxClient::deleteFile -----";

    QString uri = deleteFileURI;
    qDebug() << "DropboxClient::deleteFile uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
    sortMap["path"] = remoteFilePath;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deleteFileReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void DropboxClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- DropboxClient::shareFile -----";

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = sharesURI.arg(dropboxRoot).arg(remoteFilePath);
//    uri = encodeURI(uri);
    qDebug() << "DropboxClient::shareFile uri " << uri;

    QByteArray postData;
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

QString DropboxClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- DropboxClient::createFolder -----" << uid << remoteParentPath << newRemoteFolderName << synchronous;

    if (remoteParentPath.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
    }

    QString uri = createFolderURI;
    qDebug() << "DropboxClient::createFolder uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["root"] = dropboxRoot;
    sortMap["path"] = remoteParentPath + "/" + newRemoteFolderName;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // TODO Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
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

void DropboxClient::requestTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::requestTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Load response parameters into map.
    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        foreach (QString s, replyBody.split('&')) {
            QStringList c = s.split('=');
            m_paramMap[c.at(0)] = c.at(1);
        }

        requestTokenPair.token = m_paramMap["oauth_token"];
        requestTokenPair.secret = m_paramMap["oauth_token_secret"];

        qDebug() << "m_paramMap " << m_paramMap;
    }

    emit requestTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::accessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Load response parameters into map.
    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        foreach (QString s, replyBody.split('&')) {
            QStringList c = s.split('=');
            m_paramMap[c.at(0)] = c.at(1);
        }

        TokenPair accessTokenPair;
        accessTokenPair.token = m_paramMap["oauth_token"];
        accessTokenPair.secret = m_paramMap["oauth_token_secret"];
        QString uid = m_paramMap["uid"];

        if (uid != "" && accessTokenPair.token != "" && accessTokenPair.secret != "") {
            accessTokenPairMap[uid] = accessTokenPair;
        }

        // Save tokens.
        saveAccessPairMap();

        qDebug() << "m_paramMap " << m_paramMap;
        qDebug() << "accessTokenPairMap " << accessTokenPairMap;

        // Get email from accountInfo will be requested by CloudDriveModel.accessTokenReplyFilter().
    }

    emit accessTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::accountInfoReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        QString uid = sc.property("uid").toString();
        QString email = sc.property("email").toString();

        accessTokenPairMap[uid].email = email;
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::quotaReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::quotaReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit quotaReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll() );

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::fileGetReplyFinished(QNetworkReply *reply) {
    qDebug() << "DropboxClient::fileGetReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    if (reply->error() == QNetworkReply::NoError) {
        QString metadata;
        metadata.append(reply->rawHeader("x-dropbox-metadata"));
        qDebug() << "x-dropbox-metadata " << metadata;

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

        qDebug() << "DropboxClient::fileGetReplyFinished reply totalBytes=" << totalBytes;

        // Close target file.
        localTargetFile->close();
        m_localFileHash.remove(nonce);

        emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), metadata);
    } else {
        emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::filePutReplyFinished(QNetworkReply *reply) {
    qDebug() << "DropboxClient::filePutReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

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

void DropboxClient::metadataReplyFinished(QNetworkReply *reply) {
    qDebug() << "DropboxClient::metadataReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::browseReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::browseReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::createFolderReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::createFolderReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::moveFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::moveFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit moveFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::copyFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::copyFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::deleteFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void DropboxClient::shareFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "DropboxClient::shareFileReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

    // TODO scheduled to delete later.
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
