#include "webdavclient.h"
#include <QApplication>

const QString WebDavClient::ConsumerKey = "ce970eaaff0f4c1fbc8268bec6e804e4";
const QString WebDavClient::ConsumerSecret = "13bd2edd92344f04b23ee3170521f7fe";

const QString WebDavClient::loginURI = "https://%1"; // GET with ?userinfo
const QString WebDavClient::authorizeURI = "https://oauth.yandex.com/authorize"; // ?response_type=<token|code>&client_id=<client_id>[&display=popup][&state=<state>]
const QString WebDavClient::accessTokenURI = "https://oauth.yandex.com/token"; // POST with grant_type=authorization_code&code=<code>&client_id=<client_id>&client_secret=<client_secret> or basic auth with only grant_type=authorization_code&code=<code>
const QString WebDavClient::accountInfoURI = "https://api-fotki.yandex.ru/api/me/";
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

//    QString uri = accountInfoURI; // + "?oauth_token=" + accessToken;
    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = loginURI.arg(hostname) + "?userinfo";
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

    QString requestBody = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><propfind xmlns=\"DAV:\"><prop><quota-available-bytes/><quota-used-bytes/></prop></propfind>";

    QNetworkReply *reply = property(nonce, uid, "/", requestBody, 0);

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::quota nonce" << nonce << "replyBody" << replyBody;

    // Parse XML and convert to JSON.
    replyBody = createResponseJson(replyBody);

    qDebug() << "WebDavClient::quota nonce" << nonce << "JSON replyBody" << replyBody;

    emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
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

QNetworkReply * WebDavClient::property(QString nonce, QString uid, QString remoteFilePath, QString requestBody, int depth)
{
    qDebug() << "----- WebDavClient::property -----" << uid << remoteFilePath << requestBody << depth;

    // Default to / if remoteFilePath is empty.
    remoteFilePath = (remoteFilePath == "") ? "/" : remoteFilePath;

    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = propertyURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::property uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::property authHeader" << authHeader;

    QBuffer dataBuf;
    dataBuf.open(QIODevice::ReadWrite);
    dataBuf.write(requestBody.toUtf8());

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
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
    qDebug() << "----- WebDavClient::metadata -----" << nonce << uid << remoteFilePath;

    QNetworkReply *reply = property(nonce, uid, remoteFilePath, "", 1);

    QString replyBody = QString::fromUtf8(reply->readAll());
//    qDebug() << "WebDavClient::metadata nonce" << nonce << "replyBody" << replyBody;

    // Parse XML and convert to JSON.
    replyBody = createPropertyJson(replyBody);

//    qDebug() << "WebDavClient::metadata nonce" << nonce << "JSON replyBody" << replyBody;

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
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

QString WebDavClient::createPropertyJson(QString replyBody)
{
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
    QString jsonText = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << jsonObj).toString();

    return jsonText;
}

QString WebDavClient::createResponseJson(QString replyBody)
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
    QString jsonText = engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << jsonObj).toString();

    return jsonText;
}

QString WebDavClient::prepareRemotePath(QString uid, QString remoteFilePath)
{
    QString path = remoteFilePath;

    // Remove prefix remote root path.
    if (uid != "") {
        if (!m_remoteRootHash.contains(uid)) {
            QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
            QString remoteRoot = (hostname.indexOf("/") != -1) ? hostname.mid(hostname.indexOf("/")) : "/";
            path = path.replace(QRegExp("^"+remoteRoot), "/");
            // TODO Stores remoteRoot in m_remoteRootHash[uid].
        } else {
            path = path.replace(QRegExp("^"+m_remoteRootHash[uid]), "/");
        }
    }
    // Replace double slash.
    path = path.replace("//", "/");

    return path;
}

QString WebDavClient::removeDoubleSlash(QString remoteFilePath)
{
    QString path = remoteFilePath;

    // Replace double slash.
    path = path.replace("//", "/");

    return path;
}

void WebDavClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::browse -----" << nonce << uid << remoteFilePath;

    QNetworkReply *reply = property(nonce, uid, remoteFilePath, "", 1);

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::browse nonce" << nonce << "replyBody" << replyBody;

    // Parse XML and convert to JSON.
    replyBody = createPropertyJson(replyBody);

    // TODO Get remote root. It should be gotten while parsing XML in createPropertyJson().
    // TODO Needs to be UID-safety.
    QScriptEngine engine;
    QScriptValue sc = engine.evaluate("(" + replyBody + ")");
    if (remoteFilePath == "") {
        m_remoteRootHash[uid] = sc.property("property").property("href").toString();
        qDebug() << "WebDavClient::browse nonce" << nonce << "uid" << uid << "remote root" << m_remoteRootHash[uid];
    }

//    qDebug() << "WebDavClient::browse nonce" << nonce << "JSON replyBody" << replyBody;

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    createFolder(nonce, uid, remoteParentPath, newRemoteFolderName, false);
}

void WebDavClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    qDebug() << "----- WebDavClient::moveFile -----" << nonce << uid << remoteFilePath << newRemoteFilePath << newRemoteFileName;

    if (newRemoteFileName != "" && newRemoteFilePath == "") {
        if (remoteFilePath.endsWith("/")) {
            // Directory
            newRemoteFilePath = getParentRemotePath(remoteFilePath.mid(0, remoteFilePath.length()-1)) + "/" + newRemoteFileName + "/";
        } else {
            // File
            newRemoteFilePath = getParentRemotePath(remoteFilePath) + "/" + newRemoteFileName;
        }
        qDebug() << "WebDavClient::moveFile rename" << uid << remoteFilePath << newRemoteFilePath;
    } else {
        if (remoteFilePath.endsWith("/")) {
            // Directory
            newRemoteFilePath = newRemoteFilePath + "/";
        }
        qDebug() << "WebDavClient::moveFile move" << uid << remoteFilePath << newRemoteFilePath;
    }

    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
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
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    req.setRawHeader("Destination", destinationHeader);
    QNetworkReply *reply = manager->sendCustomRequest(req, "MOVE");

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::moveFile replyBody" << replyBody;
    emit moveFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    qDebug() << "----- WebDavClient::copyFile -----" << uid << remoteFilePath << newRemoteFilePath << newRemoteFileName;

    if (remoteFilePath.endsWith("/")) {
        // Directory
        newRemoteFilePath = newRemoteFilePath + "/";
        qDebug() << "WebDavClient::copyFile" << uid << remoteFilePath << "prepared newRemoteFilePath" << newRemoteFilePath;
    }

    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
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
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    req.setRawHeader("Destination", destinationHeader);
    req.setRawHeader("Overwrite", "F");
    QNetworkReply *reply = manager->sendCustomRequest(req, "COPY");

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::copyFile replyBody" << replyBody;
    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::deleteFile -----" << uid << remoteFilePath;

    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = deleteFileURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::deleteFile uri " << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::deleteFile authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->deleteResource(req);

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "WebDavClient::deleteFile replyBody" << replyBody;
    emit deleteFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- WebDavClient::shareFile -----" << nonce << uid << remoteFilePath;

    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = sharesURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri) + "?publish";
    qDebug() << "WebDavClient::shareFile uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::shareFile authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->post(req, QByteArray());

    // TODO Loop until readyRead.
    // ISSUE readyRead is never fired.
//    QEventLoop loop;
//    connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));
//    loop.exec();

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    qDebug() << "WebDavClient::shareFile reply ContentTypeHeader" << reply->header(QNetworkRequest::ContentTypeHeader) << "ContentLengthHeader" << reply->header(QNetworkRequest::ContentLengthHeader);
    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->header(QNetworkRequest::LocationHeader).isValid()) {
            QString location = reply->header(QNetworkRequest::LocationHeader).toUrl().toString();
            qDebug() << "WebDavClient::shareFile reply LocationHeader" << location;
            result = QString("{ \"url\": \"%1\" }").arg(location);
            emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), result);
        } else {
            // Share link service replies with actual content. Return the URI then abort connection.
            result = QString("{ \"url\": \"%1\" }").arg(uri);
            emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), result);
            reply->abort();
        }
    } else {
        result = QString::fromUtf8(reply->readAll());
        emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), result);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString WebDavClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- WebDavClient::createFolder -----" << nonce << uid << remoteParentPath << newRemoteFolderName;

    QString remoteFilePath = remoteParentPath + newRemoteFolderName;
    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = createFolderURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::createFolder uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::createFolder authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", authHeader);
    req.setRawHeader("Accept", QByteArray("*/*"));
    QNetworkReply *reply = manager->sendCustomRequest(req, "MKCOL");

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        reply = property(nonce, uid, remoteFilePath);
        if (reply->error() == QNetworkReply::NoError) {
            result = createPropertyJson(QString::fromUtf8(reply->readAll()));
        } else {
            result = createResponseJson(QString::fromUtf8(reply->readAll()));
        }
    } else {
        result = createResponseJson(QString::fromUtf8(reply->readAll()));
    }

    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), result);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QIODevice *WebDavClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- WebDavClient::fileGet -----" << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = fileGetURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::fileGet uri" << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::fileGet authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
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

        // Get property.
        QString result;
        if (reply->error() == QNetworkReply::NoError) {
            reply = property(nonce, uid, remoteFilePath);
            if (reply->error() == QNetworkReply::NoError) {
                result = createPropertyJson(QString::fromUtf8(reply->readAll()));
            }
        }

        return result;
    } else {
        qDebug() << "WebDavClient::fileGetReplySave nonce" << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        return QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Remove once used.
    m_localFileHash.remove(nonce);
}

QNetworkReply *WebDavClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- WebDavClient::filePut -----" << uid << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    QString remoteFilePath = remoteParentPath + remoteFileName;
    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
    QString uri = filePutURI.arg(hostname).arg(prepareRemotePath(uid, remoteFilePath));
    uri = encodeURI(uri);
    qDebug() << "WebDavClient::filePut uri " << uri;

    QByteArray authHeader = createAuthHeader(uid);
    qDebug() << "WebDavClient::filePut authHeader" << authHeader;

    // Send request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
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
    QString hostname = accessTokenPairMap[uid].email.split("@").at(1);
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

bool WebDavClient::testConnection(QString id, QString hostname, QString username, QString password, QString token)
{
    qDebug() << "----- WebDavClient::testConnection -----" << id << hostname << username << token;

    // Redirects to authorize if token is oauth.
    if (token.toLower() == "oauth") {
        // Save id and proceed authorizing.
        saveConnection(id, hostname, username, password, token);
        // Store id while authorizing. It will be used in accountInfo() after authorized access token.
        m_paramMap["authorize_uid"] = id;
        authorize(createNonce());
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

bool WebDavClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize >= ChunkSize);
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

void WebDavClient::fileGetReplyFinished(QNetworkReply *reply)
{
    qDebug() << "WebDavClient::fileGetReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

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
    qDebug() << "WebDavClient::filePutReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteParentPath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();
    QString remoteFileName = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();

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
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()));
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()));
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()));
    }

    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::createFolderReplyFinished(QNetworkReply *reply)
{
    qDebug() << "WebDavClient::createFolderReplyFinished " << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    QString replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        reply = property(nonce, uid, remoteFilePath);
        if (reply->error() == QNetworkReply::NoError) {
            replyBody = createPropertyJson(QString::fromUtf8(reply->readAll()));
        } else {
            replyBody = createResponseJson(QString::fromUtf8(reply->readAll()));
        }
    } else {
        replyBody = createResponseJson(QString::fromUtf8(reply->readAll()));
    }

    emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::fileGetResumeReplyFinished(QNetworkReply *reply)
{
    qDebug() << "WebDavClient::fileGetResumeReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // TODO Content-Type: multipart/byteranges.
    qDebug() << "WebDavClient::fileGetResumeReplyFinished ContentTypeHeader" << reply->header(QNetworkRequest::ContentTypeHeader);

    // Construct result.
    QString result = fileGetReplySave(reply);

    emit fileGetResumeReplySignal(nonce, reply->error(), reply->errorString(), result);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void WebDavClient::sslErrorsReplyFilter(QNetworkReply *reply, QList<QSslError> sslErrors)
{
    qDebug() << "WebDavClient::sslErrorsReplyFilter" << reply << QString(" Error=%1").arg(reply->error()) << "sslErrors" << sslErrors;

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // TODO Configure to ignore errors for self-signed certificate.
    QList<QSslError> expectedSslErrors;
    foreach (QSslError sslError, sslErrors) {
        qDebug() << "WebDavClient::sslErrorsReplyFilter sslError" << sslError << sslError.certificate();
        if (sslError.error() == QSslError::SelfSignedCertificate || sslError.error() == QSslError::HostNameMismatch) {
            expectedSslErrors.append(sslError);
        }
    }

    // TODO Make ignore errors configurable.
    reply->ignoreSslErrors(expectedSslErrors);
}

void WebDavClient::sslErrorsReplyFilter(QList<QSslError> sslErrors)
{
    foreach (QSslError sslError, sslErrors) {
        qDebug() << "WebDavClient::sslErrorsReplyFilter sslError" << sslError << sslError.certificate();
    }
}
