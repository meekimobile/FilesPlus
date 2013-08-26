#include "webdavclient.h"
#include <QApplication>

WebDavClient::WebDavClient(QObject *parent) :
    CloudDriveClient(parent)
{
    consumerKey = "ce970eaaff0f4c1fbc8268bec6e804e4";
    consumerSecret = "13bd2edd92344f04b23ee3170521f7fe";
    authorizeURI = "https://%1/authorize"; // ?response_type=<token|code>&client_id=<client_id>[&display=popup][&state=<state>]
    accessTokenURI = "https://%1/token"; // POST with grant_type=authorization_code&code=<code>&client_id=<client_id>&client_secret=<client_secret> or basic auth with only grant_type=authorization_code&code=<code>
    accountInfoURI = "https://%1"; // GET with ?userinfo
    propertyURI = "https://%1%2"; // PROPFIND with arguments (hostname, path)
    createFolderURI = "https://%1%2"; // MKCOL with arguments (hostname, path)
    moveFileURI = "https://%1%2"; // MOVE with arguments (hostname, path) and destination in header
    copyFileURI = "https://%1%2"; // COPY with arguments (hostname, path) and destination in header
    deleteFileURI = "https://%1%2"; // DELETE with arguments (hostname, path)
    fileGetURI = "https://%1%2"; // GET with arguments (hostname, path)
    filePutURI = "https://%1%2"; // PUT with arguments (hostname, path)
    sharesURI = "https://%1%2"; // POST with arguments (hostname, path) and ?publish
    renameFileURI = "https://%1%2"; // PROPPATCH with arguments (hostname, path)

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
    queryString.append("&client_id=" + consumerKey);
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
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
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
    manager->post(req, postData);
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
    manager->get(req);
}

void WebDavClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- WebDavClient::quota -----" << nonce << uid;

    QString requestBody = "<?xml version=\"1.0\" ?><D:propfind xmlns:D=\"DAV:\"><D:prop><D:quota-available-bytes/><D:quota-used-bytes/></D:prop></D:propfind>";

    property(nonce, uid, "/", requestBody, 0, false, "quota");
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
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

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

    QByteArray destinationHeader = copyFileURI.arg(getHostname(accessTokenPairMap[uid].email, true)).arg(prepareRemotePath(uid, newRemoteFilePath)).toUtf8();
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
    manager->sendCustomRequest(req, "MOVE");
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

    QByteArray destinationHeader = encodeURI(copyFileURI.arg(getHostname(accessTokenPairMap[uid].email, true)).arg(prepareRemotePath(uid, newRemoteFilePath))).toAscii();
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
    manager->sendCustomRequest(req, "COPY");
}

QString WebDavClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
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
    if (!synchronous) {
        connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deleteFileReplyFinished(QNetworkReply*)));
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
    QNetworkReply *reply = manager->deleteResource(req);

    // Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return deleteFileReplyFinished(reply, synchronous);
}

QString WebDavClient::shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- WebDavClient::shareFile -----" << nonce << uid << remoteFilePath << synchronous;

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = sharesURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri) + "?publish";
    qDebug() << "WebDavClient::shareFile uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::shareFile authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    // Configure to ignore errors for self-signed certificate.
    if (synchronous && m_settings.value(objectName() + ".ignoreSSLSelfSignedCertificateErrors", QVariant(false)).toBool()) {
        req.setSslConfiguration(getSelfSignedSslConfiguration(req.sslConfiguration()));
    }
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->post(req, QByteArray());

    // Return if asynchronous.
    if (!synchronous) {
        return "";
    }

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Emit signal and return replyBody.
    return shareFileReplyFinished(reply, synchronous);
}

QString WebDavClient::media(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::media -----" << nonce << uid << remoteFilePath;

    if (remoteFilePath.isEmpty()) {
        qDebug() << "WebDavClient::media" << nonce << uid << "remoteFilePath is empty. Operation is aborted.";
        return "";
    }

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = sharesURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri) + "?publish";
    qDebug() << "WebDavClient::media uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::media authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->post(req, QByteArray());

    while (!reply->isFinished()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    QString replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->header(QNetworkRequest::LocationHeader).isValid()) {
            QString location = reply->header(QNetworkRequest::LocationHeader).toUrl().toString();
            qDebug() << "WebDavClient::media" << nonce << "reply LocationHeader" << location;
            replyBody = location;
        } else {
            // Share link service replies with actual content. Return error then abort connection.
            qDebug() << "WebDavClient::media" << nonce << "operation is not supported.";
            reply->abort();
        }
    } else {
        qDebug() << "WebDavClient::media" << nonce << "reply error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    // Return replyBody.
    return replyBody;
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
    return createFolderReplyFinished(reply, synchronous);
}

QIODevice *WebDavClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- WebDavClient::fileGet -----" << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = fileGetURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::fileGet" << nonce << "uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::fileGet" << nonce << "authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
//        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetReplyFinished(QNetworkReply*)));
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
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "WebDavClient::fileGet" << nonce << "rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    // TODO TE: chunked, Accept-Encoding: gzip
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

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

QNetworkReply *WebDavClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- WebDavClient::filePut -----" << nonce << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;
    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = filePutURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::filePut" << nonce << "uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::filePut" << nonce << "authHeader" << authHeader;

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
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    if (synchronous) {
        // Close source file.
        source->close();

        // Remove finished reply from hash.
        m_replyHash->remove(nonce);
    }

    return reply;
}

QIODevice *WebDavClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- WebDavClient::fileGetResume -----" << nonce << remoteFilePath << "to" << localFilePath << "offset" << offset;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    QString hostname = getHostname(accessTokenPairMap[uid].email);
    QString uri = fileGetURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::fileGet" << nonce << "uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::fileGet" << nonce << "authHeader" << authHeader;

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
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "WebDavClient::fileGetResume" << nonce << "rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    // TODO TE: chunked, Accept-Encoding: gzip
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

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
            QString username = accessTokenPairMap[uid].email.mid(0, accessTokenPairMap[uid].email.lastIndexOf("@"));
            token.append(QString("%1:%2").arg(username).arg(accessTokenPairMap[uid].secret));
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
        QString v = n.toText().nodeValue();
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
        QApplication::processEvents();

        QScriptValue childObj = parseCommonPropertyScriptValue(engine, createScriptValue(engine, n, caller));
        if (!m_settings.value("WebDavClient.createPropertyJson.showHiddenSystem.enabled", false).toBool()
                && childObj.property("isHidden").toBool()) {
            // Skip hidden child if showing is disabled.
        } else {
            // Add child to array.
            childrenArrayObj.setProperty(childrenArrayIndex++, childObj);
        }
        n = n.nextSibling();
    }
    jsonObj.setProperty("children", childrenArrayObj);
    // Stringify jsonObj.
    qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "WebDavClient::createPropertyJson stringifyScriptValue started.";
    QString jsonText = stringifyScriptValue(engine, jsonObj);
    qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "WebDavClient::createPropertyJson stringifyScriptValue done." << jsonText.size();

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
        QApplication::processEvents();

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
            // TODO If path is URL, use https://<hostname with path> as remoteRoot. Otherwise use only hostname's path as remoteRoot.
            if (path.startsWith("https")) {
                QString hostname = getHostname(accessTokenPairMap[uid].email);
                QString remoteRoot = "https://" + hostname;
                path = path.replace(QRegExp("^"+remoteRoot), "/");
                qDebug() << "WebDavClient::prepareRemotePath uid" << uid << "remoteRoot" << remoteRoot << "removed. path" << path;
                // Stores remoteRoot in m_remoteRootHash[uid].
                m_remoteRootHash[uid] = remoteRoot;
            } else {
                QString hostname = getHostname(accessTokenPairMap[uid].email);
                QString remoteRoot = (hostname.indexOf("/") != -1) ? hostname.mid(hostname.indexOf("/")) : "/";
                path = path.replace(QRegExp("^"+remoteRoot), "/");
                qDebug() << "WebDavClient::prepareRemotePath uid" << uid << "remoteRoot" << remoteRoot << "removed. path" << path;
                // Stores remoteRoot in m_remoteRootHash[uid].
                m_remoteRootHash[uid] = remoteRoot;
            }
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

QString WebDavClient::getHostname(QString email, bool hostOnly)
{
    QString hostname = email.mid(email.lastIndexOf("@")+1);
    if (hostOnly && hostname.lastIndexOf(":") != -1) {
        hostname = hostname.mid(0, hostname.lastIndexOf(":"));
    }
    return hostname;
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
        if (remotePath.endsWith("/")) {
            QString trimmedRemotePath = remotePath.mid(0, remotePath.length()-1);
            name = trimmedRemotePath.mid(trimmedRemotePath.lastIndexOf("/") + 1);
        } else {
            name = remotePath.mid(remotePath.lastIndexOf("/") + 1);
        }
    }

    qDebug() << "WebDavClient::getRemoteFileName" << remotePath << "->" << name;
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
        emit logRequestSignal("",
                              "error",
                              "WebDAV " + tr("Test connection"),
                              reply->errorString(),
                              5000);
        return false;
    }
}

bool WebDavClient::saveConnection(QString id, QString hostname, QString username, QString password, QString token)
{
    qDebug() << "----- WebDavClient::saveConnection -----" << id << hostname << username << token;

    /* Notes:
     * For basic authentication, stores token as "Basic". App will construct token from (user:password).toBase64() at runtime.
     * For oauth, stores token from oauth process.
     */
    // TODO Encrypt password before store to file.
    if (accessTokenPairMap.contains(id)) {
        if (token.toLower() == "oauth") {
            // Preserves current token which will be updated once authorization is done.
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

    return true;
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
    return (fileSize == -1 || fileSize >= getChunkSize());
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
    QDateTime datetime = QDateTime::fromString(filteredDateString, "ddd, dd MMM yyyy hh:mm:ss");
    qDebug() << "WebDavClient::parseReplyDateString parse filteredDateString" << filteredDateString << "to" << datetime;
    datetime.setTimeSpec(Qt::UTC);
    qDebug() << "WebDavClient::parseReplyDateString parse datetime.setTimeSpec(Qt::UTC)" << datetime;

    return datetime;
}

void WebDavClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = m_paramMap["authorize_uid"];
    qDebug() << "WebDavClient::accessTokenReplyFinished" << nonce << "reply" << reply << QString("Error=%1").arg(reply->error());

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
    qDebug() << "WebDavClient::accountInfoReplyFinished" << nonce << "reply" << reply << QString("Error=%1").arg(reply->error());

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

QString WebDavClient::filePutReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteParentPath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString remoteFileName = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();
    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        // Delete original reply (which is in m_replyHash).
        m_replyHash->remove(nonce);
        reply->deleteLater();
        reply->manager()->deleteLater();

        reply = property(nonce, uid, remoteFilePath);
        if (reply->error() == QNetworkReply::NoError) {
            result = createPropertyJson(QString::fromUtf8(reply->readAll()), "filePutReplyResult");
        } else {
            result = createResponseJson(QString::fromUtf8(reply->readAll()), "filePutReplyResult");
        }
    } else {
        result = createResponseJson(QString::fromUtf8(reply->readAll()), "filePutReplyResult");
    }

    return result;
}

void WebDavClient::propertyReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

    qDebug() << "WebDavClient::propertyReplyFinished" << nonce << callback << uid << remoteFilePath << "reply" << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::propertyReplyFinished nonce" << nonce
             << "contentType" << reply->header(QNetworkRequest::ContentTypeHeader).toString()
             << (m_settings.value("Logging.enabled", false).toBool() ? ("replyBody " + replyBody) : "");

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

QString WebDavClient::createFolderReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qDebug() << "WebDavClient::createFolderReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::createFolderReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        // Delete existing reply.
        reply->deleteLater();
        reply->manager()->deleteLater();

        // Get created folder's property.
        reply = property(nonce, uid, remoteFilePath);
        if (reply->error() == QNetworkReply::NoError) {
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()), "createFolderReplyFinished");
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "createFolderReplyFinished");
        }
    } else if (reply->error() == QNetworkReply::ContentOperationNotPermittedError) {
        // Delete existing reply.
        reply->deleteLater();
        reply->manager()->deleteLater();

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

    // Emit signal only for asynchronous request. To avoid invoking CloudDriveModel.createFolderReplyFilter() as it's not required. And also avoid invoking jobDone() which causes issue #FP20130232.
    if (!synchronous) emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

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

    qDebug() << "WebDavClient::moveFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::moveFileReplyFinished" << nonce << "replyBody" << replyBody << "statusCode" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError) {
        // Delete original reply (which is in m_replyHash).
        m_replyHash->remove(nonce);
        reply->deleteLater();
        reply->manager()->deleteLater();

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

    qDebug() << "WebDavClient::copyFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::copyFileReplyFinished" << nonce << "replyBody" << replyBody << "statusCode" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() == QNetworkReply::NoError) {
        // Delete original reply (which is in m_replyHash).
        m_replyHash->remove(nonce);
        reply->deleteLater();
        reply->manager()->deleteLater();

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

QString WebDavClient::deleteFileReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    qDebug() << "WebDavClient::deleteFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody;
    qDebug() << "WebDavClient::deleteFileReplyFinished" << nonce << "replyBody" << replyBody;
    replyBody = createResponseJson(replyBody, "deleteFile");

    // Emit signal only for asynchronous request.
    if (!synchronous) emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

QString WebDavClient::shareFileReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    qDebug() << "WebDavClient::shareFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody;
    QString url;
    qDebug() << "WebDavClient::shareFileReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->header(QNetworkRequest::LocationHeader).isValid()) {
            url = reply->header(QNetworkRequest::LocationHeader).toUrl().toString();
            qDebug() << "WebDavClient::shareFileReplyFinished" << nonce << "reply LocationHeader" << url;
            replyBody = QString("{ \"url\": \"%1\" }").arg(url);
            if (!synchronous) emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, url, -1);
        } else {
            // Share link service replies with actual content. Return error then abort connection.
            reply->abort();
            if (!synchronous) emit shareFileReplySignal(nonce, -1, "Service is not supported.", "", "", -1);
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()), "shareFile");
        if (!synchronous) emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, "", 0);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return url;
}

QScriptValue WebDavClient::parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj)
{
    QScriptValue parsedObj = engine.newObject();

    // NOTE href always be percent-encoded from server. WebDAVClient will decode to readable format. Href will be percent-encoded agqain before actually requesting.

    bool objIsDir = jsonObj.property("href").toString().endsWith("/")
            || jsonObj.property("propstat").property("prop").property("getcontenttype").toString().indexOf(QRegExp("httpd/unix-directory", Qt::CaseInsensitive)) == 0
            || jsonObj.property("propstat").property("prop").property("isFolder").toBool()
            || jsonObj.property("propstat").property("prop").property("resourcetype").property("collection").isValid();
    QString objHref = (jsonObj.property("href").toString().startsWith("http")) ? QUrl::fromPercentEncoding(getPathFromUrl(jsonObj.property("href").toString()).toAscii()) : QUrl::fromPercentEncoding(jsonObj.property("href").toString().toAscii());
    QString objRemotePath = (objHref.endsWith("/")) ? objHref.mid(0, objHref.length()-1) : objHref; // Workaround because it ended with /
    QString objRemoteName = jsonObj.property("propstat").property("prop").property("displayname").isValid() ? jsonObj.property("propstat").property("prop").property("displayname").toString() : getRemoteFileName(objHref);
    uint objRemoteSize = jsonObj.property("propstat").property("prop").property("getcontentlength").isValid() ? jsonObj.property("propstat").property("prop").property("getcontentlength").toInteger() : 0;
    QDateTime objLastModified = jsonObj.property("propstat").property("prop").property("getlastmodified").isValid() ? parseReplyDateString(jsonObj.property("propstat").property("prop").property("getlastmodified").toString()) : QDateTime::currentDateTime();
    QString objRemoteHash = jsonObj.property("propstat").property("prop").property("getlastmodified").isValid() ? formatJSONDateString(objLastModified) : "FFFFFFFF"; // Uses DirtyHash if last modified doesn't exist.
    bool isHidden = objRemoteName.startsWith(".");

    parsedObj.setProperty("name", QScriptValue(objRemoteName));
    parsedObj.setProperty("absolutePath", QScriptValue(objHref));
    parsedObj.setProperty("parentPath", QScriptValue(getParentRemotePath(objRemotePath) + "/"));
    parsedObj.setProperty("size", QScriptValue(objRemoteSize));
    parsedObj.setProperty("isDeleted", QScriptValue(false));
    parsedObj.setProperty("isHidden", QScriptValue(isHidden));
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
    qDebug() << "WebDavClient::sslErrorsReplyFilter nonce" << nonce << "reply" << reply << QString("Error=%1").arg(reply->error()) << "sslErrors" << sslErrors;

    // Configure to ignore errors for self-signed certificate.
    if (m_settings.value(objectName() + ".ignoreSSLSelfSignedCertificateErrors", QVariant(false)).toBool()) {
        QList<QSslError> expectedSslErrors;
        foreach (QSslError sslError, sslErrors) {
            // NOTE Ignore all errors to support WIndows self-signed certificates. sslErrors ["The host name did not match any of the valid hosts for this certificate", "The issuer certificate of a locally looked up certificate could not be found", "No certificates could be verified"]
//            if (sslError.error() == QSslError::SelfSignedCertificate || sslError.error() == QSslError::HostNameMismatch) {
                expectedSslErrors.append(sslError);
//            }
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

QString WebDavClient::fileGetReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, false, true, "fileGetReplyResult");
        if (propertyReply != 0 && propertyReply->error() == QNetworkReply::NoError) {
            result = createPropertyJson(QString::fromUtf8(propertyReply->readAll()), "fileGetReplyResult");
        } else if (propertyReply == 0) {
            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(-1).arg("Service is not implemented.");
        } else {
            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(propertyReply->error()).arg(propertyReply->errorString());
        }
        propertyReply->deleteLater();
        propertyReply->manager()->deleteLater();
    } else {
        qDebug() << "WebDavClient::fileGetReplyResult" << objectName() << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\", \"http_status_code\": %3 }")
                .arg(reply->error())
                .arg(reply->errorString())
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    }

    return result;
}
