#include "webdavclient.h"
#include <QApplication>

const QString WebDavClient::ConsumerKey = "ce970eaaff0f4c1fbc8268bec6e804e4";
const QString WebDavClient::ConsumerSecret = "13bd2edd92344f04b23ee3170521f7fe";
const QString WebDavClient::WebDavRoot = "";
const QString WebDavClient::loginURI = "https://%1/?userinfo"; // GET
const QString WebDavClient::authorizeURI = "https://oauth.yandex.com/authorize"; // ?response_type=<token|code>&client_id=<client_id>[&display=popup][&state=<state>]
const QString WebDavClient::accessTokenURI = "https://oauth.yandex.com/token"; // POST with grant_type=authorization_code&code=<code>&client_id=<client_id>&client_secret=<client_secret> or basic auth with only grant_type=authorization_code&code=<code>
const QString WebDavClient::accountInfoURI = "https://api-fotki.yandex.ru/api/me/";
const QString WebDavClient::quotaURI = "https://%1/"; // PROPFIND
const QString WebDavClient::propertyURI = "https://%1%2"; // PROPFIND with arguments (hostname, path)

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

void WebDavClient::authorize(QString nonce)
{
    qDebug() << "----- WebDavClient::authorize -----";

    QString queryString;
    queryString.append("response_type=code");
    queryString.append("&client_id=" + ConsumerKey);
    queryString.append("&display=popup");
    queryString.append("&state=" + nonce);

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, this->metaObject()->className());
}

void WebDavClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- WebDavClient::accessToken -----";
// grant_type=authorization_code&code=<code>&client_id=<client_id>&client_secret=<client_secret>

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
    QNetworkRequest req = QNetworkRequest(QUrl(accessTokenURI));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager->post(req, postData);
}

void WebDavClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- WebDavClient::accountInfo -----" << uid;

    QString accessToken;
    if (uid == "") {
        // After accessToken, then uses temp access token..
        uid = m_paramMap["authorize_uid"];
        accessToken = m_paramMap["access_token"];
        qDebug() << "WebDavClient::accountInfo accessToken" << accessToken << "from m_paramMap.";
    } else {
        accessToken = accessTokenPairMap[uid].token;
        qDebug() << "WebDavClient::accountInfo accessToken" << accessToken << "from uid" << uid;
    }

//    QString uri = accountInfoURI; // + "?oauth_token=" + accessToken;
    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = loginURI.arg(hostname);
    qDebug() << "WebDavClient::accountInfo uri" << uri;

    QByteArray authHeader;
    authHeader.append(QString("OAuth %1").arg(accessToken));
    qDebug() << "WebDavClient::accountInfo authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
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

    QString requestBody = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><propfind xmlns=\"DAV:\"><prop><quota-available-bytes/><quota-used-bytes/></prop></propfind>";

    QNetworkReply *reply = property(nonce, uid, "/", requestBody, 0);

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::quota nonce" << nonce << "replyBody" << replyBody;

    // Parse XML and convert to JSON.
    QScriptEngine engine;
    QScriptValue jsonObj = engine.newObject();
    QDomDocument doc;
    doc.setContent(replyBody, true);
    QDomElement docElem = doc.documentElement();
    // Populate jsonObj starts from first child.
    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        jsonObj.setProperty(n.localName(), createScriptValue(engine, n, "quota"));
        n = n.nextSibling();
    }
    // Stringify jsonObj.
    replyBody = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << jsonObj).toString();

    qDebug() << "WebDavClient::quota nonce" << nonce << "JSON replyBody" << replyBody;

    emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody);
}

QString WebDavClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    qDebug() << "----- WebDavClient::fileGet -----" << uid << remoteFilePath << localFilePath << synchronous;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);

//    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
//    m_ftp->m_remoteFilePath = remoteFilePath;
//    m_ftp->m_localFilePath = localFilePath;
//    if (!synchronous) {
//        connect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(fileGetReplyFinished(QString,bool)) );
//    }

//    m_ftp->cd(remoteParentPath);

//    m_localFileHash[nonce] = new QFile(localFilePath);
//    QFile *localTargetFile = m_localFileHash[nonce];
//    if (localTargetFile->open(QIODevice::WriteOnly)) {
//        m_ftp->get(getRemoteFileName(remoteFilePath), localTargetFile);
//    }

//    if (!synchronous) return "";

//    // Construct result.
//    QString result;
//    if (m_ftp->error() == QFtp::NoError) {
//        m_ftp->list(remoteFilePath);
//        m_ftp->waitForDone();

//        if (m_ftp->getItemList().isEmpty()) {
//            // remoteFilePath is not found.
//            qDebug() << "WebDavClient::fileGet" << uid << remoteFilePath << "is not found.";
//            result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(-1).arg(tr("%1 is not found.").arg(remoteFilePath));
//        } else {
//            qDebug() << "WebDavClient::fileGet" << m_ftp->m_uid << m_ftp->m_remoteFilePath << "is a file.";
//            result = getPropertyJson(getParentRemotePath(remoteFilePath), m_ftp->getItemList().first());
//        }
//    } else {
//        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(m_ftp->error()).arg(m_ftp->errorString());
//    }

//    // Close and remove localFile pointer.
//    if (m_localFileHash.contains(nonce)) {
//        m_localFileHash[nonce]->close();
//        m_localFileHash.remove(nonce);
//    }

//    // Clean up.
//    m_ftp->close();
//    m_ftp->deleteLater();

//    return result;
}

/*
void WebDavClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::filePut -----" << localFilePath << "to" << remoteFilePath;

    QStringList tokens = accessTokenPairMap[uid].token.split("@");

    QUrl uploadurl("ftp://" + tokens[1] + remoteFilePath);
    uploadurl.setUserName(tokens[0]);
    uploadurl.setPassword(accessTokenPairMap[uid].secret);
    qDebug() << "WebDavClient::filePut uploadurl" << uploadurl;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
//        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
        QNetworkRequest req = QNetworkRequest(uploadurl);
        req.setAttribute(QNetworkRequest::User, QVariant(nonce));
        req.setHeader(QNetworkRequest::ContentLengthHeader, fileSize);
        QNetworkReply *reply = manager->put(req, localSourceFile);
        QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);
        connect(w, SIGNAL(uploadProgress(QString,qint64,qint64)), this, SIGNAL(uploadProgress(QString,qint64,qint64)));
        connect(w, SIGNAL(downloadProgress(QString,qint64,qint64)), this, SIGNAL(downloadProgress(QString,qint64,qint64)));

        while (!reply->isFinished()) {
            QApplication::processEvents(QEventLoop::AllEvents, 100);
            Sleeper::msleep(100);
        }

        emit filePutReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());

        // Scheduled to delete later.
        reply->deleteLater();
        reply->manager()->deleteLater();
    } else {
        emit filePutReplySignal(nonce, -1, tr("Can't open %1").arg(localFilePath), "");
    }
}
*/

void WebDavClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName)
{
    qDebug() << "----- WebDavClient::filePut -----" << uid << localFilePath << remoteParentPath << remoteFileName;

    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;

//    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
//    m_ftp->m_localFilePath = localFilePath;
//    m_ftp->m_remoteFilePath = remoteFilePath;
//    connect(m_ftp, SIGNAL(done(QString,bool)), this, SLOT(filePutReplyFinished(QString,bool)) );

//    m_ftp->cd(remoteParentPath);

//    m_localFileHash[nonce] = new QFile(localFilePath);
//    QFile *localSourceFile = m_localFileHash[nonce];
//    if (localSourceFile->open(QIODevice::ReadOnly)) {
//        m_ftp->put(localSourceFile, remoteFileName);
//        m_ftp->waitForDone();
//    }
}

QNetworkReply * WebDavClient::property(QString nonce, QString uid, QString remoteFilePath, QString requestBody, int depth)
{
    qDebug() << "----- WebDavClient::property -----" << uid << remoteFilePath << requestBody << depth;

    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = propertyURI.arg(hostname).arg(remoteFilePath);
    qDebug() << "WebDavClient::property uri" << uri;

    QByteArray authHeader;
    authHeader.append("OAuth " + accessTokenPairMap[uid].token);
    qDebug() << "WebDavClient::property authHeader" << authHeader;

    QBuffer dataBuf;
    dataBuf.open(QIODevice::ReadWrite);
    dataBuf.write(requestBody.toUtf8());

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(createNonce()));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setHeader(QNetworkRequest::ContentLengthHeader, requestBody.length());
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    req.setRawHeader("Depth", QString("%1").arg(depth).toAscii());
    QNetworkReply *reply = manager->sendCustomRequest(req, "PROPFIND", &dataBuf);

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    return reply;
}

void WebDavClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::metadata -----" << uid << remoteFilePath;

//    QString propertyJson = property(nonce, uid, remoteFilePath);
//    if (propertyJson.isEmpty()) {
//        qDebug() << "WebDavClient::metadata" << uid << remoteFilePath << "is not found.";
//        emit metadataReplySignal(nonce, QNetworkReply::ContentNotFoundError, tr("%1 is not found.").arg(remoteFilePath), "");
//        return;
//    }

//    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

//    // Check if remoteFilePath is dir or file by change directory.
//    m_ftp->cd(remoteFilePath);
//    m_ftp->waitForDone();

//    if (m_ftp->error() == QFtp::NoError) {
//        // remoteFilePath is dir.
//        qDebug() << "WebDavClient::metadata" << uid << remoteFilePath << " is dir.";
//        m_ftp->pwd();
//        m_ftp->list();
//        m_ftp->waitForDone();

//        emit metadataReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"data\": %1, \"property\": %2 }").arg(getItemListJson(m_ftp->getCurrentPath(), m_ftp->getItemList())).arg(propertyJson) );
//    } else {
//        // remoteFilePath is file or not found.
//        qDebug() << "WebDavClient::metadata" << uid << remoteFilePath << "is file or not found. error" << m_ftp->error() << m_ftp->errorString();
//        m_ftp->list(remoteFilePath);
//        m_ftp->waitForDone();

//        if (m_ftp->getItemList().isEmpty()) {
//            // remoteFilePath is not found.
//            qDebug() << "WebDavClient::metadata" << uid << remoteFilePath << "is not found.";
//            emit metadataReplySignal(m_ftp->getNonce(), QNetworkReply::ContentNotFoundError, tr("%1 is not found.").arg(remoteFilePath), "");
//        } else {
//            // remoteFilePath is file.
//            QString remoteParentPath = getParentRemotePath(remoteFilePath);
//            qDebug() << "WebDavClient::metadata" << uid << remoteFilePath << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
//            emit metadataReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), QString("{ \"data\": [], \"property\": %1 }").arg(getPropertyJson(remoteParentPath, m_ftp->getItemList().first())) );
//        }
//    }

//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());
}

QScriptValue WebDavClient::createScriptValue(QScriptEngine &engine, QDomNode &n, QString caller)
{
    qDebug() << "WebDavClient::createScriptValue caller" << caller << "nodeName" << n.nodeName() << "localName" << n.localName() << "nodeType" << n.nodeType();

    if (n.isText()) {
        return QScriptValue(n.toText().nodeValue());
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

void WebDavClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::browse -----" << nonce << uid << remoteFilePath;

    if (remoteFilePath == "") {
        remoteFilePath = "/";
    }

    QNetworkReply *reply = property(nonce, uid, remoteFilePath, "", 1);

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::browse nonce" << nonce << "replyBody" << replyBody;

    // Parse XML and convert to JSON.
    QScriptEngine engine;
    QScriptValue jsonObj = engine.newObject();
    QDomDocument doc;
    doc.setContent(replyBody, true);
    QDomElement docElem = doc.documentElement();
    QDomNode n = docElem.firstChild();
    // Populate property from first child.
    if (!n.isNull()) {
        QScriptValue propertyObj = createScriptValue(engine, n, "browse");
        jsonObj.setProperty("property", propertyObj);
    }
    // Populate data from remain children.
    QScriptValue dataArrayObj = engine.newArray();
    int dataArrayIndex = 0;
    n = n.nextSibling();
    while(!n.isNull()) {
        dataArrayObj.setProperty(dataArrayIndex++, createScriptValue(engine, n, "browse"));
        n = n.nextSibling();
    }
    jsonObj.setProperty("data", dataArrayObj);
    // Stringify jsonObj.
    replyBody = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << jsonObj).toString();

    qDebug() << "WebDavClient::browse nonce" << nonce << "JSON replyBody" << replyBody;

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), replyBody);
}

void WebDavClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteParentPath, newRemoteFolderName, false);
}

void WebDavClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit moveFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, tr("FTP doesn't support move command."), "");
}

void WebDavClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit copyFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, tr("FTP doesn't support copy command."), "");
}

void WebDavClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::deleteFile -----" << uid << remoteFilePath;

//    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

//    deleteRecursive(m_ftp, remoteFilePath);

//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());
}

void WebDavClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::shareFile -----" << uid << remoteFilePath;

    emit shareFileReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, "FTP doesn't support resource link sharing.", "");
}

QString WebDavClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- WebDavClient::createFolder -----" << uid << remoteParentPath << newRemoteFolderName;

//    if (remoteParentPath.isEmpty()) {
//        emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
//        return "";
//    }

//    if (newRemoteFolderName.isEmpty()) {
//        emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
//        return "";
//    }

//    QString newRemoteFolderPath = remoteParentPath + "/" + newRemoteFolderName;

//    QFtpWrapper *m_ftp = connectToHost(nonce, uid);

//    m_ftp->mkdir(newRemoteFolderPath);
//    m_ftp->waitForDone();

//    QString result;
//    if (m_ftp->error() != QFtp::NoError) {
//        // NOTE json string doesn't support newline character.
//        QString escapedErrorString =  m_ftp->errorString().replace("\n", " ");
//        emit createFolderReplySignal(nonce, QNetworkReply::ContentOperationNotPermittedError, m_ftp->errorString(), QString("{ \"error\": \"%1\", \"path\": \"%2\" }").arg(escapedErrorString).arg(newRemoteFolderPath) );
//    } else {
//        // Get property.
//        QString propertyJson = property(nonce, uid, newRemoteFolderPath);
//        if (propertyJson.isEmpty()) {
//            qDebug() << "WebDavClient::createFolder" << uid << newRemoteFolderPath << "is not found.";
//            emit createFolderReplySignal(nonce, QNetworkReply::ContentNotFoundError, "Created path is not found.", QString("{ \"error\": \"Created path is not found.\", \"path\": \"%1\" }").arg(newRemoteFolderPath) );
//        } else {
//            result = propertyJson;
//            emit createFolderReplySignal(nonce, QNetworkReply::NoError, "", result);
//        }
//    }

//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());

//    return result;
}

QIODevice *WebDavClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- WebDavClient::fileGet -----" << uid << remoteFilePath << "synchronous" << synchronous;

    QString remoteParentPath = getParentRemotePath(remoteFilePath);

//    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
//    m_ftp->m_remoteFilePath = remoteFilePath;
//    m_ftp->cd(remoteParentPath);

//    // Creat buffer as QIODevice.
//    QBuffer *buf = new QBuffer();
//    if (buf->open(QIODevice::WriteOnly)) {
//        m_ftp->get(getRemoteFileName(remoteFilePath), buf);
//        m_ftp->waitForDone();
//        buf->close();
//    }

//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());

//    return buf;
}

QNetworkReply *WebDavClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- WebDavClient::filePut -----" << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString remoteFilePath = remoteParentPath + "/" + remoteFileName;

//    QFtpWrapper *m_ftp = connectToHost(nonce, uid);
//    m_ftp->m_remoteFilePath = remoteFilePath;
//    m_ftp->cd(remoteParentPath);

//    m_ftp->put(source, remoteFileName);
//    m_ftp->waitForDone();
//    source->close();

//    // TODO Get uploaded file property.
//    m_ftp->list(remoteFilePath);
//    m_ftp->waitForDone();

//    if (m_ftp->getItemList().isEmpty()) {
//        // remoteFilePath is not found.
//        qDebug() << "WebDavClient::filePut" << uid << remoteFilePath << "is not found.";
//        emit migrateFilePutReplySignal(m_ftp->getNonce(), -1, tr("Can't put %1").arg(remoteFilePath), "");
//    } else {
//        qDebug() << "WebDavClient::filePut" << uid << remoteFilePath << "is a file. remoteParentPath" << remoteParentPath << "remoteFileName" << m_ftp->getItemList().first().name();
//        emit migrateFilePutReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(remoteParentPath, m_ftp->getItemList().first()) );
//    }

//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());

//    return 0;
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

bool WebDavClient::testConnection(QString id, QString hostname, QString username, QString password)
{
    qDebug() << "----- WebDavClient::testConnection -----";

    if (!accessTokenPairMap.contains(id) || accessTokenPairMap[id].token == "") {
        // Save id.
        saveConnection(id, hostname, username, password);
        // Store id while authorizing. It will be used in accountInfo() after authorized access token.
        m_paramMap["authorize_uid"] = id;
        // Request authorize to get access token.
        authorize(createNonce());
        // TODO Emit signal to close add connection dialog.
        return false;
    }

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

void WebDavClient::saveConnection(QString id, QString hostname, QString username, QString password)
{
    qDebug() << "----- WebDavClient::saveConnection -----";

    /* Notes:
     * Stores token as (user:password).toBase64();
     */
    // TODO Encrypt password before store to file.
    if (accessTokenPairMap.contains(id)) {
//        accessTokenPairMap[id].token = QByteArray().append(username + ":" + password).toBase64();
        accessTokenPairMap[id].email = QString("%1@%2").arg(username).arg(hostname);
        accessTokenPairMap[id].secret = password;
    } else {
        TokenPair tokenPair;
//       tokenPair.token = QByteArray().append(username + ":" + password).toBase64();
        tokenPair.token = "";
        tokenPair.secret = password;
        tokenPair.email = QString("%1@%2").arg(username).arg(hostname);
        accessTokenPairMap[id] = tokenPair;
    }

    saveAccessPairMap();
}

bool WebDavClient::isRemoteAbsolutePath()
{
    return true;
}

void WebDavClient::deleteFileReplyFinished(QString nonce, int error, QString errorString)
{
    qDebug() << "WebDavClient::deleteFileReplyFinished" << nonce << error;

//    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);

//    emit deleteFileReplySignal(nonce, error, errorString, QString("{ \"path\": \"%1\" }").arg(m_ftp->m_remoteFilePath) );

//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(nonce);
}

void WebDavClient::fileGetReplyFinished(QString nonce, bool error)
{
    // Get stored parameters and clean ftp object.
//    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
//    QString uid = m_ftp->m_uid;
//    QString remoteFilePath = m_ftp->m_remoteFilePath;
//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());

//    if (!error) {
//        // Connect and get uploaded file property.
//        m_ftp = connectToHost(nonce, uid);
//        m_ftp->list(remoteFilePath);
//        m_ftp->waitForDone();

//        if (m_ftp->getItemList().isEmpty()) {
//            // remoteFilePath is not found.
//            qDebug() << "WebDavClient::fileGetReplyFinished" << uid << remoteFilePath << "is not found.";
//            emit fileGetReplySignal(m_ftp->getNonce(), -1, tr("Can't get %1").arg(remoteFilePath), "");
//        } else {
//            qDebug() << "WebDavClient::fileGetReplyFinished" << m_ftp->m_uid << m_ftp->m_remoteFilePath << "is a file.";
//            emit fileGetReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(getParentRemotePath(remoteFilePath), m_ftp->getItemList().first()) );
//        }
//    } else {
//        emit fileGetReplySignal(m_ftp->getNonce(), m_ftp->error(), tr("Can't get %1").arg(remoteFilePath), m_ftp->errorString());
//    }

//    // Close and remove localFile pointer.
//    if (m_localFileHash.contains(nonce)) {
//        m_localFileHash[nonce]->close();
//        m_localFileHash.remove(nonce);
//    }

//    // Clean up.
//    m_ftp->close();
//    m_ftp->deleteLater();
}

void WebDavClient::filePutReplyFinished(QString nonce, bool error)
{
    // Get stored parameters and clean ftp object.
//    QFtpWrapper *m_ftp = m_ftpHash->value(nonce);
//    QString uid = m_ftp->m_uid;
//    QString remoteFilePath = m_ftp->m_remoteFilePath;
//    m_ftp->close();
//    m_ftp->deleteLater();
//    m_ftpHash->remove(m_ftp->getNonce());

//    if (!error) {
//        // Connect and get uploaded file property.
//        m_ftp = connectToHost(nonce, uid);
//        m_ftp->list(remoteFilePath);
//        m_ftp->waitForDone();

//        if (m_ftp->getItemList().isEmpty()) {
//            // remoteFilePath is not found.
//            qDebug() << "WebDavClient::filePutReplyFinished" << uid << remoteFilePath << "is not found.";
//            emit filePutReplySignal(m_ftp->getNonce(), -1, tr("Can't put %1").arg(remoteFilePath), "");
//        } else {
//            qDebug() << "WebDavClient::filePutReplyFinished" << uid << remoteFilePath << "is a file.";
//            emit filePutReplySignal(m_ftp->getNonce(), m_ftp->error(), m_ftp->errorString(), getPropertyJson(getParentRemotePath(remoteFilePath), m_ftp->getItemList().first()) );
//        }
//    } else {
//        emit filePutReplySignal(m_ftp->getNonce(), m_ftp->error(), tr("Can't put %1").arg(remoteFilePath), m_ftp->errorString());
//    }

//    // Close and remove localFile pointer.
//    if (m_localFileHash.contains(nonce)) {
//        m_localFileHash[nonce]->close();
//        m_localFileHash.remove(nonce);
//    }

//    // Clean up.
//    m_ftp->close();
//    m_ftp->deleteLater();
}

void WebDavClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "WebDavClient::accessTokenReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = m_paramMap["authorize_uid"];

    // Load response parameters into map.
    QString replyBody = QString::fromUtf8(reply->readAll());

    qDebug() << "WebDavClient::accessTokenReplyFinished replyBody" << replyBody;

    if (reply->error() == QNetworkReply::NoError) {
        // Parse access token.
        // Sample response: {"access_token": "ea135929105c4f29a0f5117d2960926f", "expires_in": 2592000}
        QScriptEngine engine;
        QScriptValue sc = engine.evaluate("(" + replyBody + ")");

        m_paramMap["access_token"] = sc.property("access_token").toString();
        qDebug() << "m_paramMap " << m_paramMap;

        if (accessTokenPairMap.contains(uid)) {
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
    qDebug() << "WebDavClient::accountInfoReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    QString replyBody = QString::fromUtf8(reply->readAll());

    qDebug() << "WebDavClient::accountInfoReplyFinished replyBody" << replyBody;

    if (reply->error() == QNetworkReply::NoError) {
/*
 * Example response: login:test
 */
        QString username = replyBody.split(":").at(1);
        qDebug() << "WebDavClient::accountInfoReplyFinished m_paramMap" << m_paramMap << "uid" << uid << "username" << username;

        if (accessTokenPairMap.contains(uid)) {
            qDebug() << "WebDavClient::accountInfoReplyFinished found existing accessToken for uid" << uid << accessTokenPairMap[uid];

            if (m_paramMap.contains("access_token")) {
                accessTokenPairMap[uid].token = m_paramMap["access_token"];
//                if (m_paramMap.contains("refresh_token") && m_paramMap["refresh_token"] != "") {
//                    accessTokenPairMap[uid].secret = m_paramMap["refresh_token"];
//                }

                // Reset temp. accessToken and refreshToken.
                m_paramMap.remove("authorize_uid");
                m_paramMap.remove("access_token");
//                m_paramMap.remove("refresh_token");

                // Save account after got id and email.
                saveAccessPairMap();
            }
        } else {
            qDebug() << "WebDavClient::accountInfoReplyFinished not found existing accessToken for uid" << uid;
        }
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}
