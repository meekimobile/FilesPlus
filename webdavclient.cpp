#include "webdavclient.h"
#include <QApplication>

const QString WebDavClient::ConsumerKey = "ce970eaaff0f4c1fbc8268bec6e804e4";
const QString WebDavClient::ConsumerSecret = "13bd2edd92344f04b23ee3170521f7fe";

const QString WebDavClient::authorizeURI = "https://%1/authorize"; // ?response_type=<token|code>&client_id=<client_id>[&display=popup][&state=<state>]
const QString WebDavClient::accessTokenURI = "https://%1/token"; // POST with grant_type=authorization_code&code=<code>&client_id=<client_id>&client_secret=<client_secret> or basic auth with only grant_type=authorization_code&code=<code>
const QString WebDavClient::accountInfoURI = "https://%1"; // GET with ?userinfo
const QString WebDavClient::propertyURI = "https://%1%2"; // PROPFIND with arguments (hostname, path)
const QString WebDavClient::createFolderURI = "https://%1%2"; // MKCOL with arguments (hostname, path)
const QString WebDavClient::moveFileURI = "https://%1%2"; // MOVE with arguments (hostname, path) and destination in header
const QString WebDavClient::copyFileURI = "https://%1%2"; // COPY with arguments (hostname, path) and destination in header
const QString WebDavClient::deleteFileURI = "https://%1%2"; // DELETE with arguments (hostname, path)
const QString WebDavClient::fileGetURI = "https://%1%2"; // GET with arguments (hostname, path)
const QString WebDavClient::filePutURI = "https://%1%2"; // PUT with arguments (hostname, path)
const QString WebDavClient::sharesURI = "https://%1%2"; // POST with arguments (hostname, path) and ?publish
const QString WebDavClient::renameFileURI = "https://%1%2"; // PROPPATCH with arguments (hostname, path)

const qint64 WebDavClient::ChunkSize = 4194304; // 4194304=4MB, 1048576=1MB  TODO Optimize to have largest chink size which is still valid before token is expired.

const QString WebDavClient::ReplyDateFormat = "ddd, dd MMM yyyy hh:mm:ss";

WebDavClient::WebDavClient(QObject *parent) :
    CloudDriveClient(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();
}

WebDavClient::~WebDavClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();
}

void WebDavClient::authorize(QString nonce, QString hostname)
{
    qDebug() << "----- WebDavClient::authorize -----" << nonce << hostname;

    m_paramMap["authorize_hostname"] = hostname;

    QString queryString;
    queryString.append("response_type=code");
    queryString.append("&client_id=" + ConsumerKey);
    queryString.append("&display=popup");
    queryString.append("&state=" + nonce);

    QString url = authorizeURI.arg(hostname) + "?" + queryString;
    qDebug() << "WebDavClient::authorize url" << url;

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, url, objectName());
}

void WebDavClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- WebDavClient::accessToken -----" << nonce << pin;

    // Uses hostname set by authorize().
    QString hostname = m_paramMap["authorize_hostname"];
    QString uri = accessTokenURI.arg(hostname);
    qDebug() << "WebDavClient::accessToken uri" << uri;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["grant_type"] = "authorization_code";
    sortMap["code"] = pin;
    sortMap["client_id"] = ConsumerKey;
    sortMap["client_secret"] = ConsumerSecret;
    QString queryString = createNormalizedQueryString(sortMap);
    qDebug() << "WebDavClient::accessToken queryString " << queryString;

    QByteArray postData;
    postData.append(queryString);
    qDebug() << "WebDavClient::accessToken postData" << postData;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accessTokenReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
}

void WebDavClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- WebDavClient::accountInfo -----" << uid;

    uid = (uid == "") ? m_paramMap["authorize_uid"] : uid;
    if (uid == "") {
        qDebug() << "WebDavClient::accountInfo uid is empty. Operation is aborted.";
        return;
    }

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = accountInfoURI.arg(hostname) + "?userinfo";
    qDebug() << "WebDavClient::accountInfo uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::accountInfo authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setRawHeader("Authorization", authHeader);
    QNetworkReply *reply = manager->get(req);
}

void WebDavClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- WebDavClient::quota -----" << nonce << uid;

    QString requestBody = "<?xml version=\"1.0\" ?><D:propfind xmlns:D=\"DAV:\"><D:prop><D:quota-available-bytes/><D:quota-used-bytes/></D:prop></D:propfind>";

    property(nonce, uid, "/", requestBody, 0, false, "quota");
}

QString WebDavClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    qDebug() << "----- WebDavClient::fileGet -----" << nonce << uid << remoteFilePath << localFilePath << synchronous;

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

void WebDavClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName)
{
    qDebug() << "----- WebDavClient::filePut -----" << uid << localFilePath << remoteParentPath << remoteFileName;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        filePut(nonce, uid, localSourceFile, fileSize, remoteParentPath, remoteFileName, false);
    } else {
        qDebug() << "WebDavClient::filePut file " << localFilePath << " can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
    }
}

QNetworkReply * WebDavClient::property(QString nonce, QString uid, QString remoteFilePath, QString requestBody, int depth, bool synchronous, QString callback)
{
    qDebug() << "----- WebDavClient::property -----" << nonce << uid << remoteFilePath << requestBody << depth << synchronous << callback;

    // Default to / if remoteFilePath is empty.
    remoteFilePath = (remoteFilePath == "") ? "/" : remoteFilePath;

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = propertyURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::property nonce" << nonce << "uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::property nonce" << nonce << "authHeader" << authHeader;

    // Insert buffer to hash.
    m_bufferHash.insert(nonce, new QBuffer());
    m_bufferHash[nonce]->open(QIODevice::ReadWrite);
    m_bufferHash[nonce]->write(requestBody.toUtf8());
    m_bufferHash[nonce]->reset(); // Reset pos for reading while request.

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(propertyReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    // Configure to ignore errors for self-signed certificate.
    if (synchronous && m_settings.value(objectName() + ".ignoreSSLSelfSignedCertificateErrors", QVariant(false)).toBool()) {
        req.setSslConfiguration(getSelfSignedSslConfiguration(req.sslConfiguration()));
    }
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(callback));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(remoteFilePath));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/xml");
    req.setHeader(QNetworkRequest::ContentLengthHeader, m_bufferHash[nonce]->size());
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    req.setRawHeader("Depth", QString("%1").arg(depth).toAscii());
    QNetworkReply *reply = manager->sendCustomRequest(req, "PROPFIND", m_bufferHash[nonce]);

    // Return if asynchronous.
    if (!synchronous) {
        return reply;
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Remove request buffer.
    m_bufferHash[nonce]->close();
    m_bufferHash.remove(nonce);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return reply;
}

void WebDavClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::metadata -----" << nonce << uid << remoteFilePath;

    property(nonce, uid, remoteFilePath, "", 1, false, "metadata");
}

void WebDavClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::browse -----" << nonce << uid << remoteFilePath;

    property(nonce, uid, remoteFilePath, "", 1, false, "browse");
}

void WebDavClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteParentPath, newRemoteFolderName, false);
}

void WebDavClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- WebDavClient::moveFile -----" << nonce << uid << remoteFilePath << newRemoteParentPath << newRemoteFileName;

    QString newRemoteFilePath;
    if (newRemoteFileName != "" && newRemoteParentPath == "") {
        // Rename
        if (remoteFilePath.endsWith("/")) {
            // Directory
            newRemoteFilePath = getParentRemotePath(remoteFilePath.mid(0, remoteFilePath.length()-1)) + "/" + newRemoteFileName + "/";
        } else {
            // File
            newRemoteFilePath = getParentRemotePath(remoteFilePath) + "/" + newRemoteFileName;
        }
        qDebug() << "WebDavClient::moveFile rename" << uid << remoteFilePath << "to" << newRemoteFilePath;
    } else {
        // Move
        if (remoteFilePath.endsWith("/")) {
            // Directory
            newRemoteFilePath = newRemoteParentPath + "/" + newRemoteFileName + "/";
        } else {
            // File
            newRemoteFilePath = newRemoteParentPath + "/" + newRemoteFileName;
        }
        qDebug() << "WebDavClient::moveFile move" << uid << remoteFilePath << "to" << newRemoteFilePath;
    }

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = moveFileURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::moveFile uri " << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::moveFile authHeader" << authHeader;

    QByteArray destinationHeader = copyFileURI.arg(hostname).arg(prepareRemotePath(uid, newRemoteFilePath)).toUtf8();
    qDebug() << "WebDavClient::moveFile destinationHeader" << destinationHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(moveFileReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(newRemoteFilePath));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    req.setRawHeader("Destination", destinationHeader);
    QNetworkReply *reply = manager->sendCustomRequest(req, "MOVE");
}

void WebDavClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- WebDavClient::copyFile -----" << uid << remoteFilePath << newRemoteParentPath << newRemoteFileName;

    QString newRemoteFilePath;
    if (remoteFilePath.endsWith("/")) {
        // Directory
        newRemoteFilePath = newRemoteParentPath + "/" + newRemoteFileName + "/";
    } else {
        // File
        newRemoteFilePath = newRemoteParentPath + "/" + newRemoteFileName;
    }
    qDebug() << "WebDavClient::copyFile" << uid << remoteFilePath << "to" << newRemoteFilePath;

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = copyFileURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::copyFile uri " << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::copyFile authHeader" << authHeader;

    QByteArray destinationHeader = copyFileURI.arg(hostname).arg(prepareRemotePath(uid, newRemoteFilePath)).toUtf8();
    qDebug() << "WebDavClient::copyFile destinationHeader" << destinationHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(copyFileReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(newRemoteFilePath));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    req.setRawHeader("Destination", destinationHeader);
    req.setRawHeader("Overwrite", "F");
    QNetworkReply *reply = manager->sendCustomRequest(req, "COPY");
}

void WebDavClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::deleteFile -----" << uid << remoteFilePath;

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = deleteFileURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::deleteFile uri " << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::deleteFile authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deleteFileReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->deleteResource(req);
}

void WebDavClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::shareFile -----" << nonce << uid << remoteFilePath;

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = sharesURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri) + "?publish";
    qDebug() << "WebDavClient::shareFile uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::shareFile authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->post(req, QByteArray());
}

QString WebDavClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- WebDavClient::createFolder -----" << nonce << uid << remoteParentPath << newRemoteFolderName;

    QString remoteFilePath = remoteParentPath + "/" + newRemoteFolderName + "/"; // remote folder path needs to have trailing slash.
    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = createFolderURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::createFolder uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::createFolder authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    // Configure to ignore errors for self-signed certificate.
    if (synchronous && m_settings.value(objectName() + ".ignoreSSLSelfSignedCertificateErrors", QVariant(false)).toBool()) {
        req.setSslConfiguration(getSelfSignedSslConfiguration(req.sslConfiguration()));
    }
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->sendCustomRequest(req, "MKCOL");

    // Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return createFolderReplyFinished(reply);
}

QIODevice *WebDavClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- WebDavClient::fileGet -----" << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = fileGetURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::fileGet uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::fileGet authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    // Configure to ignore errors for self-signed certificate.
    if (synchronous && m_settings.value(objectName() + ".ignoreSSLSelfSignedCertificateErrors", QVariant(false)).toBool()) {
        req.setSslConfiguration(getSelfSignedSslConfiguration(req.sslConfiguration()));
    }
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+ChunkSize-1);
        qDebug() << "WebDavClient::fileGet rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    // TODO TE: chunked, Accept-Encoding: gzip
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    return reply;
}

QString WebDavClient::fileGetReplySave(QNetworkReply *reply)
{
    qDebug() << "WebDavClient::fileGetReplySave" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "WebDavClient::fileGetReplySave reply bytesAvailable" << reply->bytesAvailable();

        // Stream replyBody to a file on localPath.
        qint64 totalBytes = 0;
        char buf[1024];
        QFile *localTargetFile = m_localFileHash[nonce];
        qDebug() << "WebDavClient::fileGetReplySave" << nonce << "localTargetFile" << localTargetFile;
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

        qDebug() << "WebDavClient::fileGetReplySave reply totalBytes=" << totalBytes;

        // Close target file.
        localTargetFile->close();

        // Return common json.
        reply = property(nonce, uid, remoteFilePath);
        QString propertyReplyBody = QString::fromUtf8(reply->readAll());
        qDebug() << "WebDavClient::fileGetReplySave propertyReplyBody" << propertyReplyBody;
        if (reply->error() == QNetworkReply::NoError) {
            result = createPropertyJson(propertyReplyBody, "fileGetReplySave");
        } else {
            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
        }
    } else {
        qDebug() << "WebDavClient::fileGetReplySave nonce" << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Remove once used.
    m_localFileHash.remove(nonce);

    return result;
}

QNetworkReply *WebDavClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- WebDavClient::filePut -----" << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;
    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = filePutURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::filePut uri " << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::filePut authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    // Configure to ignore errors for self-signed certificate.
    if (synchronous && m_settings.value(objectName() + ".ignoreSSLSelfSignedCertificateErrors", QVariant(false)).toBool()) {
        req.setSslConfiguration(getSelfSignedSslConfiguration(req.sslConfiguration()));
    }
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteParentPath));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(remoteFileName));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/binary"));
    req.setHeader(QNetworkRequest::ContentLengthHeader, bytesTotal);
    // TODO Content-Encoding: gzip, Transfer-Encoding: chunked
    QNetworkReply *reply = manager->put(req, source);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (synchronous) {
        // Close source file.
        source->close();
    }

    return reply;
}

QIODevice *WebDavClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- WebDavClient::fileGetResume -----" << remoteFilePath << "to" << localFilePath << "offset" << offset;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = fileGetURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::fileGet uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::fileGet authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetResumeReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+ChunkSize-1);
        qDebug() << "WebDavClient::fileGetResume rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    // TODO TE: chunked, Accept-Encoding: gzip
    QNetworkReply *reply = manager->get(req);
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
    connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

    return reply;
}

QByteArray WebDavClient::createAuthHeader(QString uid)
{
    QByteArray authHeader;
    if (uid == "") {
        // After accessToken, then uses temp access token..
        QString uid = m_paramMap["authorize_uid"];
        QString accessToken = m_paramMap["access_token"];
        qDebug() << "WebDavClient::createAuthHeader get uid" << uid << "accessToken" << accessToken << "from m_paramMap.";
        authHeader.append("OAuth " + accessToken);
    } else if (accessTokenPairMap.contains(uid)) {
        if (accessTokenPairMap[uid].token.toLower() == "basic") {
            QByteArray token;
            token.append(QString("%1:%2").arg(accessTokenPairMap[uid].email.split("@").at(0)).arg(accessTokenPairMap[uid].secret));
            authHeader.append("Basic ");
            authHeader.append(token.toBase64());
        } else {
            authHeader.append("OAuth " + accessTokenPairMap[uid].token);
        }
    }

    return authHeader;
}

QScriptValue WebDavClient::createScriptValue(QScriptEngine &engine, QDomNode &n, QString caller)
{
//    qDebug() << "WebDavClient::createScriptValue caller" << caller << "nodeName" << n.nodeName() << "localName" << n.localName() << "nodeType" << n.nodeType();

    if (n.isText()) {
        QString v = QUrl::fromPercentEncoding(n.toText().nodeValue().toAscii());
        return QScriptValue(v);
    } else if (n.hasChildNodes()) {
        if (n.firstChild().isText()) {
            // Get text node value if there is only 1 text child node.
            QDomNode textNode = n.firstChild();
            return createScriptValue(engine, textNode, caller);
        } else {
            // Create object from node with children.
            QScriptValue jsonObj = engine.newObject();
            for (int i=0; i < n.childNodes().length(); i++) {
                QDomNode childNode = n.childNodes().at(i);
//                qDebug() << "WebDavClient::createScriptValue child i" << i << childNode.nodeName() << childNode.localName() << childNode.nodeType();
                // Drilldown recursively to create JSON object.
                jsonObj.setProperty(childNode.localName(), createScriptValue(engine, childNode, caller));
            }
            return jsonObj;
        }
    } else {
        return QScriptValue();
    }
}

QString WebDavClient::createPropertyJson(QString replyBody, QString caller)
{
    QScriptEngine engine;
    QScriptValue jsonObj;

    QDomDocument doc;
    doc.setContent(replyBody, true);
    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    // Populate property from first child.
    if (!n.isNull()) {
        jsonObj = parseCommonPropertyScriptValue(engine, createScriptValue(engine, n, caller));
    }
    // Populate children from remain children.
    QScriptValue childrenArrayObj = engine.newArray();
    int childrenArrayIndex = 0;
    n = n.nextSibling();
    while(!n.isNull()) {
        childrenArrayObj.setProperty(childrenArrayIndex++, parseCommonPropertyScriptValue(engine, createScriptValue(engine, n, caller)));
        n = n.nextSibling();
    }
    jsonObj.setProperty("children", childrenArrayObj);
    // Stringify jsonObj.
    QString jsonText = stringifyScriptValue(engine, jsonObj);

    return jsonText;
}

QString WebDavClient::createResponseJson(QString replyBody, QString caller)
{
    QScriptEngine engine;
    QScriptValue jsonObj = engine.newObject();
    QDomDocument doc;
    doc.setContent(replyBody, true);
    QDomElement docElem = doc.documentElement();
    // Populate jsonObj starts from first child.
    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        jsonObj.setProperty(n.localName(), createScriptValue(engine, n, "createResponseJson"));
        n = n.nextSibling();
    }
    // Stringify jsonObj.
    QString jsonText = stringifyScriptValue(engine, jsonObj);

    return jsonText;
}

QString WebDavClient::prepareRemotePath(QString uid, QString remoteFilePath)
{
    QString path = remoteFilePath;
//    qDebug() << "WebDavClient::prepareRemotePath path" << path;

    // Remove prefix remote root path.
    if (uid != "") {
        if (!m_remoteRootHash.contains(uid) || m_remoteRootHash[uid] == "") {
            QString hostname = getHostname(accessTokenPairMap[uid].email);
            QString remoteRoot = (hostname.indexOf("/") != -1) ? hostname.mid(hostname.indexOf("/")) : "/";
            path = path.replace(QRegExp("^"+remoteRoot), "/");
            qDebug() << "WebDavClient::prepareRemotePath uid" << uid << "remoteRoot" << remoteRoot << "removed. path" << path;
            // TODO Stores remoteRoot in m_remoteRootHash[uid].
            m_remoteRootHash[uid] = remoteRoot;
        } else {
            path = path.replace(QRegExp("^"+m_remoteRootHash[uid]), "/");
//            qDebug() << "WebDavClient::prepareRemotePath remoteRoot" << m_remoteRootHash[uid] << "removed. path" << path;
        }
    }
    // Replace double slash.
    path = removeDoubleSlash(path);
//    qDebug() << "WebDavClient::prepareRemotePath double slash removed. path" << path;

    return path;
}

QString WebDavClient::getHostname(QString email)
{
    return email.mid(email.lastIndexOf("@")+1);
}

QString WebDavClient::getParentRemotePath(QString remotePath)
{
    QString remoteParentPath = "";
    if (remotePath != "") {
        remoteParentPath = remotePath.mid(0, remotePath.lastIndexOf("/"));
    }

    return remoteParentPath;
}

QString WebDavClient::getRemoteFileName(QString remotePath)
{
    QString name = "";
    if (remotePath != "") {
        name = remotePath.mid(remotePath.lastIndexOf("/") + 1);
    }

    return name;
}

void WebDavClient::testSSLConnection(QString hostname)
{
    QSslSocket socket;
//    qDebug() << "WebDavClient::testConnection SSL socket localCertificate" << socket.localCertificate();
//    qDebug() << "WebDavClient::testConnection SSL socket peerCertificate" << socket.peerCertificate();
//    qDebug() << "WebDavClient::testConnection SSL socket peerCertificateChain" << socket.peerCertificateChain();
//    qDebug() << "WebDavClient::testConnection SSL socket defaultCaCertificates" << socket.defaultCaCertificates();
//    qDebug() << "WebDavClient::testConnection SSL socket caCertificates" << socket.caCertificates();
//    qDebug() << "WebDavClient::testConnection SSL socket systemCaCertificates" << socket.systemCaCertificates();
//    qDebug() << "WebDavClient::testConnection SSL socket defaultCiphers" << socket.defaultCiphers();
//    qDebug() << "WebDavClient::testConnection SSL socket ciphers" << socket.ciphers();
    qDebug() << "WebDavClient::testConnection SSL socket peerVerifyDepth" << socket.peerVerifyDepth();
    qDebug() << "WebDavClient::testConnection SSL socket peerVerifyMode" << socket.peerVerifyMode();
    qDebug() << "WebDavClient::testConnection SSL socket privateKey" << socket.privateKey();
//    qDebug() << "WebDavClient::testConnection SSL socket supportedCiphers" << socket.supportedCiphers() ;
//    qDebug() << "WebDavClient::testConnection SSL socket supportsSsl" << socket.supportsSsl();

    // Configure to ignore errors for self-signed certificate.
    QList<QSslCertificate> cert = QSslCertificate::fromPath(QLatin1String("E:/certificates/server.crt"));
    qDebug() << "WebDavClient::testConnection SSL socket loaded certificate" << cert.at(0) << "isValid" << cert.at(0).isValid();
    QList<QSslError> expectedSslErrors;
    expectedSslErrors.append(QSslError(QSslError::SelfSignedCertificate, cert.at(0)));
    expectedSslErrors.append(QSslError(QSslError::HostNameMismatch, cert.at(0)));
    socket.ignoreSslErrors(expectedSslErrors);
    connect(&socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QList<QSslError>)) );

    socket.connectToHostEncrypted(hostname, 443);
    if (!socket.waitForConnected()) {
        qDebug() << "WebDavClient::testConnection SSL socket connection error" << socket.error() << socket.errorString();
    } else {
        qDebug() << "WebDavClient::testConnection SSL socket connection success";
    }
    if (!socket.waitForEncrypted()) {
        qDebug() << "WebDavClient::testConnection SSL socket encryption error" << socket.error() << socket.errorString();
        qDebug() << "WebDavClient::testConnection SSL socket encryption error localCertificate" << socket.localCertificate() << "peerCertificate" << socket.peerCertificate();
    } else {
        qDebug() << "WebDavClient::testConnection SSL socket encryption success";
        qDebug() << "WebDavClient::testConnection SSL socket encryption success localCertificate" << socket.localCertificate() << "peerCertificate" << socket.peerCertificate();

        QByteArray requestBody("GET / HTTP/1.0\r\n\r\n");
        qDebug() << "WebDavClient::testConnection SSL socket request" << requestBody;
        socket.write(requestBody);
        while (socket.waitForReadyRead()) {
            qDebug() << "WebDavClient::testConnection SSL socket response" << socket.readAll().data();
        }
    }
    socket.disconnectFromHost();
    qDebug() << "WebDavClient::testConnection SSL socket connection closed";
}

QSslConfiguration WebDavClient::getSelfSignedSslConfiguration(QSslConfiguration sslConf)
{
//    QFile serverCertFile("E:/certificates/server.crt");
//    QFile serverKeyFile("E:/certificates/server.key");
//    sslConf.setLocalCertificate(QSslCertificate(&serverCertFile, QSsl::Pem));
//    sslConf.setPrivateKey(QSslKey(&serverKeyFile, QSsl::Rsa));
    sslConf.setPeerVerifyMode(QSslSocket::VerifyNone);

    qDebug() << "WebDavClient::getSelfSignedSslConfiguration sslConfiguration is modified. sslConf.peerVerifyMode" << sslConf.peerVerifyMode();
    return sslConf;
}

bool WebDavClient::testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname)
{
    qDebug() << "----- WebDavClient::testConnection -----" << id << hostname << username << token;

    // Redirects to authorize if token is oauth.
    if (token.toLower() == "oauth") {
        // Save id and proceed authorizing.
        saveConnection(id, hostname, username, password, token);
        // Store id while authorizing. It will be used in accountInfo() after authorized access token.
        m_paramMap["authorize_uid"] = id;
        m_paramMap["authorize_hostname"] = authHostname;
        authorize(createNonce(), authHostname);
        return false;
    }

    // Save id and proceed testing.
    saveConnection(id, hostname, username, password, token);

    // Test connection (with empty PROPFIND method) with id's stored access token.
    QNetworkReply *reply = property(createNonce(), id, "/");

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    qDebug() << "WebDavClient::testConnection reply" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
    if (reply->error() == QNetworkReply::NoError) {
        return true;
    } else {
        return false;
    }
}

void WebDavClient::saveConnection(QString id, QString hostname, QString username, QString password, QString token)
{
    qDebug() << "----- WebDavClient::saveConnection -----" << id << hostname << username << token;

    /* Notes:
     * For basic authentication, stores token as "Basic". App will construct token from (user:password).toBase64() at runtime.
     * For oauth, stores token from oauth process.
     */
    // TODO Encrypt password before store to file.
    if (accessTokenPairMap.contains(id)) {
        if (token.toLower() == "oauth") {
            // Preserves current toekn which will be updated once authorization is done.
        } else {
            accessTokenPairMap[id].token = token;
        }
        accessTokenPairMap[id].email = QString("%1@%2").arg(username).arg(hostname);
        accessTokenPairMap[id].secret = password;
    } else {
        TokenPair tokenPair;
        tokenPair.token = token;
        tokenPair.email = QString("%1@%2").arg(username).arg(hostname);
        tokenPair.secret = password;
        accessTokenPairMap[id] = tokenPair;
    }

    saveAccessPairMap();
}

QString WebDavClient::getRemoteRoot(QString uid)
{
    return m_remoteRootHash[uid];
}

bool WebDavClient::isRemoteAbsolutePath()
{
    return true;
}

bool WebDavClient::isConfigurable()
{
    return true;
}

bool WebDavClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize == -1 || fileSize >= ChunkSize);
}

QDateTime WebDavClient::parseReplyDateString(QString dateString)
{
    /*
     *Example date string
     * "Tue, 29 Jan 2013 00:14:31 GMT"  from apache2+mod_dav
     * "Fri, 25 Jan 2013 10:01:27 GMT" from Yandex.ru
    */

    QString filteredDateString = dateString;
    filteredDateString = filteredDateString.replace("GMT","").trimmed();
    qDebug() << "WebDavClient::parseReplyDateString filter dateString" << dateString << "to filteredDateString" << filteredDateString;
    QDateTime datetime = QDateTime::fromString(filteredDateString, ReplyDateFormat);
    qDebug() << "WebDavClient::parseReplyDateString parse filteredDateString" << filteredDateString << "with" << ReplyDateFormat << "to" << datetime;
    datetime.setTimeSpec(Qt::UTC);
    qDebug() << "WebDavClient::parseReplyDateString parse datetime.setTimeSpec(Qt::UTC)" << datetime;

    return datetime;
}

void WebDavClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = m_paramMap["authorize_uid"];
    qDebug() << "WebDavClient::accessTokenReplyFinished" << nonce << "reply" << reply << QString(" Error=%1").arg(reply->error());

    // Load response parameters into map.
    QString replyBody = QString::fromUtf8(reply->readAll());

    qDebug() << "WebDavClient::accessTokenReplyFinished" << nonce << "replyBody" << replyBody;

    if (reply->error() == QNetworkReply::NoError) {
        // Parse access token.
        // Sample response: {"access_token": "ea135929105c4f29a0f5117d2960926f", "expires_in": 2592000}
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + replyBody + ")");

        m_paramMap["access_token"] = sc.property("access_token").toString();
        qDebug() << "m_paramMap " << m_paramMap;

        if (accessTokenPairMap.contains(uid)) {
            // Update access token to UID's account.
            accessTokenPairMap[uid].token = m_paramMap["access_token"];

            // Save account.
            saveAccessPairMap();
        }

        // Get email from accountInfo will be requested by CloudDriveModel.accessTokenReplyFilter().
    }

    emit accessTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    qDebug() << "WebDavClient::accountInfoReplyFinished" << nonce << "reply" << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());

    qDebug() << "WebDavClient::accountInfoReplyFinished" << nonce << "replyBody" << replyBody;

    if (reply->error() == QNetworkReply::NoError) {
/*
 * Example response: login:test
 */
        QString username = replyBody.split(":").at(1);
        qDebug() << "WebDavClient::accountInfoReplyFinished" << nonce << "m_paramMap" << m_paramMap << "uid" << uid << "username" << username;

        if (accessTokenPairMap.contains(uid)) {
            qDebug() << "WebDavClient::accountInfoReplyFinished" << nonce << "found existing accessToken for uid" << uid << accessTokenPairMap[uid];
            // TODO Update username to UID's account?
        } else {
            qDebug() << "WebDavClient::accountInfoReplyFinished" << nonce << "not found existing accessToken for uid" << uid;
        }
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::fileGetReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "WebDavClient::fileGetReplyFinished" << nonce << "reply" << reply << QString(" Error=%1").arg(reply->error());

    // TODO Content-Type: multipart/byteranges.

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), result);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::filePutReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteParentPath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString remoteFileName = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();
    qDebug() << "WebDavClient::filePutReplyFinished" << nonce << "reply" << reply << QString(" Error=%1").arg(reply->error());

    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;

    // Close source file.
    if (m_localFileHash.contains(nonce)) {
        QFile *localTargetFile = m_localFileHash[nonce];
        localTargetFile->close();
        m_localFileHash.remove(nonce);
    }

    QString replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        reply = property(nonce, uid, remoteFilePath);
        if (reply->error() == QNetworkReply::NoError) {
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()), "filePutReplyFinished");
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "filePutReplyFinished");
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "filePutReplyFinished");
    }

    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::propertyReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

    qDebug() << "WebDavClient::propertyReplyFinished" << nonce << callback << uid << remoteFilePath << "reply" << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::propertyReplyFinished nonce" << nonce << "replyBody" << replyBody << "contentType" << reply->header(QNetworkRequest::ContentTypeHeader).toString();

    // Parse XML and convert to JSON.
    if (reply->error() == QNetworkReply::NoError && reply->header(QNetworkRequest::ContentTypeHeader).toString().indexOf("xml") != -1) {
        if (callback == "browse") {
            replyBody = createPropertyJson(replyBody, callback);

            // Get remote root. It should be gotten while parsing XML in createPropertyJson().
            // Needs to be UID-safety.
            QScriptEngine engine;
            QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
            if (remoteFilePath == "" || remoteFilePath == "/") {
                m_remoteRootHash[uid] = jsonObj.property("absolutePath").toString();
                qDebug() << "WebDavClient::propertyReplyFinished browse nonce" << nonce << "uid" << uid << "remote root" << m_remoteRootHash[uid];
            }

            emit browseReplySignal(nonce, reply->error(), reply->errorString(), replyBody);
        } else if (callback == "metadata") {
            replyBody = createPropertyJson(replyBody, callback);

            emit metadataReplySignal(nonce, reply->error(), reply->errorString(), replyBody);
        } else if (callback == "quota") {
            // Parse as generic response json.
            replyBody = createResponseJson(replyBody, callback);

            QScriptEngine engine;
            QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
            qint64 sharedValue = 0;
            qint64 normalValue = jsonObj.property("response").property("propstat").property("prop").property("quota-used-bytes").toInteger();
            qint64 quotaValue = jsonObj.property("response").property("propstat").property("prop").property("quota-available-bytes").toInteger();

            emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, normalValue, sharedValue, quotaValue);
        } else {
            qDebug() << "WebDavClient::propertyReplyFinished nonce" << nonce << "invalid callback" << callback;
        }
    } else {       
        if (callback == "browse") {
            replyBody = createResponseJson(replyBody, callback);
            emit browseReplySignal(nonce, reply->error(), reply->errorString(), replyBody);
        } else if (callback == "metadata") {
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 301) {
                qDebug() << "WebDavClient::propertyReplyFinished nonce" << nonce << "callback" << callback << "got HTTP response code 301, reply as content not found (error 203).";
                // Handle response code 301 to support WebDav on apache2.
                emit metadataReplySignal(nonce, QNetworkReply::ContentNotFoundError, "Content not found", replyBody);
            } else {
                replyBody = createResponseJson(replyBody, callback);
                emit metadataReplySignal(nonce, reply->error(), reply->errorString(), replyBody);
            }
        } else if (callback == "quota") {
            replyBody = createResponseJson(replyBody, callback);
            emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, 0, 0, -1);
        } else {
            qDebug() << "WebDavClient::propertyReplyFinished nonce" << nonce << "invalid callback" << callback;
        }
    }

    // Remove request buffer.
    m_bufferHash[nonce]->close();
    m_bufferHash.remove(nonce);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString WebDavClient::createFolderReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qDebug() << "WebDavClient::createFolderReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::createFolderReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        // Get created folder's property.
        reply = property(nonce, uid, remoteFilePath);
        if (reply->error() == QNetworkReply::NoError) {
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()), "createFolderReplyFinished");
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "createFolderReplyFinished");
        }
    } else if (reply->error() == QNetworkReply::ContentOperationNotPermittedError) {
        // Get existing folder's property.
        reply = property(nonce, uid, remoteFilePath);
        if (reply->error() == QNetworkReply::NoError) {
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()), "createFolderReplyFinished");
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "createFolderReplyFinished");
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "createFolderReplyFinished");
    }

    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

void WebDavClient::moveFileReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString newRemoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qDebug() << "WebDavClient::moveFileReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::moveFileReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        reply = property(nonce, uid, prepareRemotePath(uid, newRemoteFilePath));
        if (reply->error() == QNetworkReply::NoError) {
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()), "moveFile");
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "moveFile");
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "moveFile");
    }

    emit moveFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::copyFileReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString newRemoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qDebug() << "WebDavClient::copyFileReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::copyFileReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        reply = property(nonce, uid, prepareRemotePath(uid, newRemoteFilePath));
        if (reply->error() == QNetworkReply::NoError) {
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()), "copyFile");
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "copyFile");
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "copyFile");
    }

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    qDebug() << "WebDavClient::deleteFileReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::deleteFileReplyFinished" << nonce << "replyBody" << replyBody;
    replyBody = createResponseJson(replyBody, "deleteFile");

    emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::shareFileReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    qDebug() << "WebDavClient::shareFileReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::shareFileReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->header(QNetworkRequest::LocationHeader).isValid()) {
            QString location = reply->header(QNetworkRequest::LocationHeader).toUrl().toString();
            qDebug() << "WebDavClient::shareFileReplyFinished" << nonce << "reply LocationHeader" << location;
            replyBody = QString("{ \"url\": \"%1\" }").arg(location);
            emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, location, -1);
        } else {
            // Share link service replies with actual content. Return the URI then abort connection.
            reply->abort();
            QString uri = reply->request().url().toString();
            qDebug() << "WebDavClient::shareFileReplyFinished" << nonce << "reply request uri" << uri;
            replyBody = QString("{ \"url\": \"%1\" }").arg(uri);
            emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, uri, -1);
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "shareFile");
        emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, "", 0);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::fileGetResumeReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "WebDavClient::fileGetResumeReplyFinished" << nonce << "reply" << reply << QString(" Error=%1").arg(reply->error());

    // TODO Content-Type: multipart/byteranges.
    qDebug() << "WebDavClient::fileGetResumeReplyFinished" << nonce << "ContentTypeHeader" << reply->header(QNetworkRequest::ContentTypeHeader);

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetResumeReplySignal(nonce, reply->error(), reply->errorString(), result);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QScriptValue WebDavClient::parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj)
{
    QScriptValue parsedObj = engine.newObject();

    bool objIsDir = jsonObj.property("href").toString().endsWith("/");
    QString objRemotePath = objIsDir ? jsonObj.property("href").toString().mid(0, jsonObj.property("href").toString().length()-1) : jsonObj.property("href").toString(); // Workaround because it ended with /
    QString objRemoteName = jsonObj.property("propstat").property("prop").property("displayname").isValid() ? jsonObj.property("propstat").property("prop").property("displayname").toString() : getRemoteFileName(objRemotePath);
    uint objRemoteSize = jsonObj.property("propstat").property("prop").property("getcontentlength").isValid() ? jsonObj.property("propstat").property("prop").property("getcontentlength").toInteger() : 0;
    QDateTime objLastModified = jsonObj.property("propstat").property("prop").property("getlastmodified").isValid() ? parseReplyDateString(jsonObj.property("propstat").property("prop").property("getlastmodified").toString()) : QDateTime::currentDateTime();
    QString objRemoteHash = jsonObj.property("propstat").property("prop").property("getlastmodified").isValid() ? formatJSONDateString(objLastModified) : "FFFFFFFF"; // Uses DirtyHash if last modified doesn't exist.

    parsedObj.setProperty("name", QScriptValue(objRemoteName));
    parsedObj.setProperty("absolutePath", jsonObj.property("href"));
    parsedObj.setProperty("parentPath", QScriptValue(getParentRemotePath(objRemotePath) + "/"));
    parsedObj.setProperty("size", QScriptValue(objRemoteSize));
    parsedObj.setProperty("isDeleted", QScriptValue(false));
    parsedObj.setProperty("isDir", QScriptValue(objIsDir));
    parsedObj.setProperty("lastModified", QScriptValue(formatJSONDateString(objLastModified)));
    parsedObj.setProperty("hash",  QScriptValue(objRemoteHash));
    parsedObj.setProperty("source", QScriptValue());
    parsedObj.setProperty("thumbnail", QScriptValue());
    parsedObj.setProperty("fileType", QScriptValue(getFileType(objRemoteName)));

    return parsedObj;
}

void WebDavClient::sslErrorsReplyFilter(QNetworkReply *reply, QList<QSslError> sslErrors)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "WebDavClient::sslErrorsReplyFilter nonce" << nonce << "reply" << reply << QString(" Error=%1").arg(reply->error()) << "sslErrors" << sslErrors;

    // Configure to ignore errors for self-signed certificate.
    if (m_settings.value(objectName() + ".ignoreSSLSelfSignedCertificateErrors", QVariant(false)).toBool()) {
        QList<QSslError> expectedSslErrors;
        foreach (QSslError sslError, sslErrors) {
            if (sslError.error() == QSslError::SelfSignedCertificate || sslError.error() == QSslError::HostNameMismatch) {
                expectedSslErrors.append(sslError);
            }
        }

        qDebug() << "WebDavClient::sslErrorsReplyFilter nonce" << nonce << "ignore expectedSslErrors" << expectedSslErrors;
        reply->ignoreSslErrors(expectedSslErrors);
    }
}

void WebDavClient::sslErrorsReplyFilter(QList<QSslError> sslErrors)
{
    foreach (QSslError sslError, sslErrors) {
        qDebug() << "WebDavClient::sslErrorsReplyFilter sslError" << sslError << sslError.certificate();
    }
}
