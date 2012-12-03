#include "skydriveclient.h"
#include <QtGlobal>
#include <QCryptographicHash>
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

//const QString SkyDriveClient::signatureMethod = "HMAC-SHA1"; // Failed since 1-Sep-12
const QString SkyDriveClient::signatureMethod = "PLAINTEXT";
const QString SkyDriveClient::requestTokenURI = "";
const QString SkyDriveClient::authorizeURI = "https://login.live.com/oauth20_authorize.srf"; // &display=touch
const QString SkyDriveClient::accessTokenURI = "https://login.live.com/oauth20_token.srf";
const QString SkyDriveClient::accountInfoURI = "https://apis.live.net/v5.0/me";
const QString SkyDriveClient::quotaURI = "http://apis.live.net/v5.0/me/skydrive/quota";
const QString SkyDriveClient::logoutURI = "https://login.live.com/oauth20_logout.srf";

const QString SkyDriveClient::fileGetURI = "https://api-content.dropbox.com/1/files/%1%2";
const QString SkyDriveClient::filePutURI = "https://api-content.dropbox.com/1/files_put/%1%2"; // Parameter if any needs to be appended here. ?param=val
const QString SkyDriveClient::metadataURI = "https://api.dropbox.com/1/metadata/%1%2";
const QString SkyDriveClient::createFolderURI = "https://api.dropbox.com/1/fileops/create_folder";
const QString SkyDriveClient::moveFileURI = "https://api.dropbox.com/1/fileops/move";
const QString SkyDriveClient::copyFileURI = "https://api.dropbox.com/1/fileops/copy";
const QString SkyDriveClient::deleteFileURI = "https://api.dropbox.com/1/fileops/delete";
const QString SkyDriveClient::sharesURI = "https://api.dropbox.com/1/shares/%1%2";
const QString SkyDriveClient::mediaURI = "https://api.dropbox.com/1/media/%1%2";

SkyDriveClient::SkyDriveClient(QDeclarativeItem *parent) :
    QObject(parent)
{
    // Load accessTokenPair from file
    loadAccessPairMap();

    m_paramMap["oauth_consumer_key"] = consumerKey;
    m_paramMap["oauth_signature_method"] = signatureMethod;
}

SkyDriveClient::~SkyDriveClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();
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

//QString SkyDriveClient::createNonce() {
//    QString ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
//    QString nonce;

//    for(int i = 0; i <= 16; ++i)
//    {
//        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
//    }

//    return nonce;
//}

QByteArray SkyDriveClient::createBaseString(QString method, QString uri, QString queryString) {
    QByteArray baseString;
    baseString.append(method);
    baseString.append("&");
    baseString.append(QUrl::toPercentEncoding(uri));
    baseString.append("&");
    baseString.append(QUrl::toPercentEncoding(queryString, "", "()"));

    return baseString;
}

QString SkyDriveClient::createSignature(QString signatureMethod, QString consumerSecret, QString tokenSecret, QByteArray baseString)
{
    QString signature = "";

    if (signatureMethod == "HMAC-SHA1") {
        // Construct key for HMACSHA1 by using consumer 'Secret' and request token secret.
        signature = createSignatureWithHMACSHA1(consumerSecret, tokenSecret, baseString);
    } else if (signatureMethod == "PLAINTEXT") {
        // Construct key for PLAINTEXT by using consumer 'Secret' and request token secret.
        signature = createSignatureWithPLAINTEXT(consumerSecret, tokenSecret, baseString);
    }
    qDebug() << "SkyDriveClient::createSignature signature" << signatureMethod << signature;

    return signature;
}

QString SkyDriveClient::createSignatureWithHMACSHA1(QString consumerSecret, QString tokenSecret, QByteArray baseString)
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

QString SkyDriveClient::createSignatureWithPLAINTEXT(QString consumerSecret, QString tokenSecret, QByteArray baseString)
{
    // Join secrets into key.
    QByteArray key;
    key.append(consumerSecret);
    key.append("&");
    key.append(tokenSecret);

    return key;
}

QString SkyDriveClient::createNormalizedQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(QUrl::toPercentEncoding(key)).append("=").append(QUrl::toPercentEncoding(sortMap[key]));
    }

    return queryString;
}

QByteArray SkyDriveClient::createPostData(QString signature, QString queryString) {
    QByteArray postData;
    postData.append("oauth_signature=");
    postData.append(QUrl::toPercentEncoding(signature));
    postData.append("&");
    postData.append(queryString);

    return postData;
}

QByteArray SkyDriveClient::createOAuthHeaderString(QMap<QString, QString> sortMap) {
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

void SkyDriveClient::authorize(QString nonce)
{
    qDebug() << "----- SkyDriveClient::authorize -----";

    QString queryString;
    queryString.append("client_id=" + consumerKey);
    queryString.append("&response_type=code");
    queryString.append("&scope=" + QUrl::toPercentEncoding("wl.signin wl.basic wl.offline_access wl.emails"));
    queryString.append("&redirect_uri=http://www.meeki.mobi/products/filesplus/skd_oauth_callback");
    queryString.append("&display=touch");

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, "SkyDriveClient");
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
        jsonText.append( QString("\"type\": \"%1\"").arg("SkyDrive") );
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

QString SkyDriveClient::encodeURI(const QString uri) {
    // Example: https://api.dropbox.com/1/metadata/sandbox/C/B/NES/Solomon's Key (E) [!].nes
    // All non-alphanumeric except : and / must be encoded.
    return QUrl::toPercentEncoding(uri, ":/");
}

QByteArray SkyDriveClient::createOAuthHeaderForUid(QString nonce, QString uid, QString method, QString uri, QMap<QString, QString> addParamMap) {
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
    uri.append("?access_token=" + QUrl::toPercentEncoding(accessToken) );
    qDebug() << "SkyDriveClient::accountInfo uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void SkyDriveClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- SkyDriveClient::quota ----- uid" << uid;

    QString uri = quotaURI;
    uri.append("?access_token=" + QUrl::toPercentEncoding(accessTokenPairMap[uid].token) );
    qDebug() << "SkyDriveClient::quota uri " << uri;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SIGNAL(uploadProgress(qint64,qint64)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SIGNAL(downloadProgress(qint64,qint64)));
}

void SkyDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath) {
    qDebug() << "----- SkyDriveClient::fileGet -----";

    QString uri = fileGetURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::fileGet uri " << uri;

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

QString SkyDriveClient::getDefaultLocalFilePath(const QString &remoteFilePath)
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

void SkyDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath) {
    qDebug() << "----- SkyDriveClient::filePut -----";

    QString uri = filePutURI.arg(remoteFilePath);
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
        req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "PUT", uri));
        req.setHeader(QNetworkRequest::ContentLengthHeader, fileSize);
        QNetworkReply *reply = manager->put(req, localSourceFile);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

//        qDebug() << "SkyDriveClient::filePut put file " << localFilePath << " to dropbox " << remoteFilePath;
    } else {
        qDebug() << "SkyDriveClient::filePut file " << localFilePath << " can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
    }
}

QString SkyDriveClient::getDefaultRemoteFilePath(const QString &localFilePath)
{
    QRegExp rx("^([C-F])(:)(/.+)$");
    rx.indexIn(localFilePath);
    if (rx.captureCount() == 3) {
        return "/" + rx.cap(1).append(rx.cap(3));
    }
    return "";
}

void SkyDriveClient::metadata(QString nonce, QString uid, QString remoteFilePath) {
    qDebug() << "----- SkyDriveClient::metadata -----";

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = metadataURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::metadata uri " << uri;

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

void SkyDriveClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- SkyDriveClient::browse -----" << remoteFilePath;

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = metadataURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::browse uri " << uri;

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

void SkyDriveClient::createFolder(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "----- SkyDriveClient::createFolder -----";

    QString uri = createFolderURI;
    qDebug() << "SkyDriveClient::createFolder uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["path"] = remoteFilePath;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
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
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
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
    req.setRawHeader("Authorization", createOAuthHeaderForUid(nonce, uid, "POST", uri, sortMap));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));
}

void SkyDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- SkyDriveClient::deleteFile -----";

    QString uri = deleteFileURI;
    qDebug() << "SkyDriveClient::deleteFile uri " << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
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

void SkyDriveClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- SkyDriveClient::shareFile -----";

    // TODO root dropbox(Full access) or sandbox(App folder access)
    QString uri = sharesURI.arg(remoteFilePath);
//    uri = encodeURI(uri);
    qDebug() << "SkyDriveClient::shareFile uri " << uri;

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

void SkyDriveClient::requestTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "SkyDriveClient::requestTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

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

        qDebug() << "m_paramMap " << m_paramMap;

        // Updates for refreshToken.
        if (refreshTokenUid != "") {
            accessTokenPairMap[refreshTokenUid].token = sc.property("access_token").toString();
            accessTokenPairMap[refreshTokenUid].secret = sc.property("refresh_token").toString();
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
        } else {
            TokenPair accessTokenPair;
            accessTokenPair.token = m_paramMap["access_token"];
            accessTokenPair.secret = m_paramMap["refresh_token"];
            accessTokenPair.email = email;

            if (uid != "" && accessTokenPair.token != "" && accessTokenPair.secret != "") {
                accessTokenPairMap[uid] = accessTokenPair;
            }
        }

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

        qDebug() << "SkyDriveClient::fileGetReplyFinished reply totalBytes=" << totalBytes;

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










