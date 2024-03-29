#include "gcdclient.h"
#include <QtGlobal>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QScriptEngine>
#include <QScriptValueIterator>

GCDClient::GCDClient(QObject *parent) :
    CloudDriveClient(parent)
{
    consumerKey = "196573379494.apps.googleusercontent.com";
    consumerSecret = "il59cyz3dwBW6tsHBkZYGSWj";

    RemoteRoot = "root";

    authorizationScope = "https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile https://www.googleapis.com/auth/drive";

    authorizeURI = "https://accounts.google.com/o/oauth2/auth";
    accessTokenURI = "https://accounts.google.com/o/oauth2/token";
    accountInfoURI = "https://www.googleapis.com/oauth2/v2/userinfo";
    quotaURI = "https://www.googleapis.com/drive/v2/about";

    fileGetURI = "%1"; // Use downloadUrl from property.
    filePutURI = "https://www.googleapis.com/upload/drive/v2/files%1"; // POST with ?uploadType=media or multipart
    filesURI = "https://www.googleapis.com/drive/v2/files"; // GET with q
    propertyURI = "https://www.googleapis.com/drive/v2/files/%1"; // GET
    createFolderURI = "https://www.googleapis.com/drive/v2/files"; // POST with json.
    copyFileURI = "https://www.googleapis.com/drive/v2/files/%1/copy"; // POST with partial json body.
    deleteFileURI = "https://www.googleapis.com/drive/v2/files/%1"; // DELETE
    patchFileURI = "https://www.googleapis.com/drive/v2/files/%1?setModifiedDate=%2"; // PATCH with partial json body.
    sharesURI = "https://www.googleapis.com/drive/v2/files/%1/permissions"; // POST to insert permission.
    trashFileURI = "https://www.googleapis.com/drive/v2/files/%1/trash"; // POST
    startResumableUploadURI = "https://www.googleapis.com/upload/drive/v2/files%1?uploadType=resumable"; // POST with json includes file properties.
    resumeUploadURI = "%1"; // PUT with specified URL.
    deltaURI = "https://www.googleapis.com/drive/v2/changes"; // GET with parameters.

    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    // Load accessTokenPair from file
    loadAccessPairMap();

    m_downloadUrlHash = new QHash<QString, QString>;
}

GCDClient::~GCDClient()
{
    // Save accessTokenPair to file
    saveAccessPairMap();

    m_downloadUrlHash = 0;
}

QMap<QString, QString> GCDClient::createMapFromJson(QString jsonText)
{
    qDebug() << "GCDClient::createMapFromJson " << jsonText;

    QMap<QString, QString> map;
    QScriptEngine engine;
    QScriptValue sc;
    sc = engine.evaluate("(" + jsonText + ")");

    QScriptValueIterator it(sc);
    while (it.hasNext()) {
        it.next();
        map[it.name()] = it.value().toString();
    }

    qDebug() << "GCDClient::createMapFromJson " << map;

    return map;
}

QHash<QString, QString> GCDClient::createHashFromScriptValue(QString parentName, QScriptValue sc) {
    QHash<QString, QString> hash;
    QScriptValueIterator it(sc);
    while (it.hasNext()) {
        it.next();

        // key = parentName.name...
        QString key = ((parentName == "") ? "" : (parentName + ".")) + it.name();

        if(it.value().isArray()) {
//            qDebug() << "key " << key << " isArray";
            QStringList items;
            qScriptValueToSequence(it.value(), items);
            for (int i=0; i<items.length(); i++) {
                QScriptValue v = it.value().property(QString("%1").arg(i));
                hash.unite(createHashFromScriptValue(QString("%1.%2").arg(key).arg(i) , v));
            }
        } else if (it.value().isObject() ) {
//            qDebug() << "key " << key << " isObject";
            hash.unite(createHashFromScriptValue(key, it.value()));
        } else {
            hash[key] = it.value().toString();
        }
    }

//    qDebug() << "GCDClient::createHashFromScriptValue parentName " << parentName << " hash " << hash;

    return hash;
}

QHash<QString, QString> GCDClient::createHashFromJson(QString jsonText)
{
//    qDebug() << "GCDClient::createHashFromJson " << jsonText;

    QHash<QString, QString> hash;
    QScriptEngine engine;
    QScriptValue sc;
    sc = engine.evaluate("(" + jsonText + ")");

    qDebug() << "sc " << sc.toString();

    hash = createHashFromScriptValue("", sc);

//    qDebug() << "GCDClient::createHashFromJson " << hash;

    return hash;
}

bool GCDClient::isCloudOnly(QScriptValue jsonObj)
{
    // Check if file is cloud only which can't be downloaded.
    bool isCloudOnly = jsonObj.property("mimeType").toString().startsWith("application/vnd.google-apps.");

    return isCloudOnly;
}

QScriptValue GCDClient::parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj)
{
    QScriptValue parsedObj = engine.newObject();

    QScriptValue downloadUrlObj = jsonObj.property("downloadUrl");

    // NOTE lastViewedByMeDate is changed on every view on web UI.
    QScriptValue hashObj;
    if (jsonObj.property("lastViewedByMeDate").isValid()) {
        if (!jsonObj.property("lastViewedByMeDate").lessThan(jsonObj.property("modifiedDate")) ) {
            hashObj = jsonObj.property("lastViewedByMeDate");
        } else {
            hashObj = jsonObj.property("modifiedDate");
        }
    } else {
        hashObj = jsonObj.property("modifiedDate");
    }

    parsedObj.setProperty("name", jsonObj.property("title"));
    parsedObj.setProperty("absolutePath", jsonObj.property("id"));
    parsedObj.setProperty("parentPath", jsonObj.property("parents").property(0).isValid() ? jsonObj.property("parents").property(0).property("id") : QScriptValue(""));
    parsedObj.setProperty("size", jsonObj.property("fileSize").isValid() ? jsonObj.property("fileSize") : QScriptValue(0));
    parsedObj.setProperty("isDeleted", QScriptValue(jsonObj.property("explicitlyTrashed").toBool() || jsonObj.property("labels").property("trashed").toBool()));
    parsedObj.setProperty("isDir", QScriptValue(jsonObj.property("mimeType").toString() == "application/vnd.google-apps.folder"));
    parsedObj.setProperty("lastModified", jsonObj.property("modifiedDate"));
    parsedObj.setProperty("hash", hashObj);
    parsedObj.setProperty("source", QScriptValue());
    parsedObj.setProperty("downloadUrl", downloadUrlObj);
    parsedObj.setProperty("webContentLink", jsonObj.property("webContentLink"));
    parsedObj.setProperty("embedLink", jsonObj.property("embedLink"));
    parsedObj.setProperty("alternative", jsonObj.property("alternateLink"));
    parsedObj.setProperty("thumbnail", jsonObj.property("thumbnailLink"));
    parsedObj.setProperty("thumbnail128", jsonObj.property("thumbnailLink"));
    parsedObj.setProperty("preview", jsonObj.property("thumbnailLink")); // NOTE Use same URL as thumbnail as it return 220 x 220 picture.
    parsedObj.setProperty("fileType", QScriptValue(getFileType(jsonObj.property("title").toString())));
    parsedObj.setProperty("mimeType", jsonObj.property("mimeType"));
    parsedObj.setProperty("isCloudOnly", QScriptValue(isCloudOnly(jsonObj)));
    parsedObj.setProperty("exportLinks", jsonObj.property("exportLinks"));
    parsedObj.setProperty("iconLink", jsonObj.property("iconLink"));

    // Cache downloadUrl for furthur usage.
    if (downloadUrlObj.isValid()) {
        m_downloadUrlHash->insert(jsonObj.property("id").toString(), downloadUrlObj.toString());
    }

    return parsedObj;
}

void GCDClient::authorize(QString nonce, QString hostname)
{
    qDebug() << "----- GCDClient::authorize -----";

    QMap<QString, QString> sortMap;
    sortMap["response_type"] = "code";
    sortMap["client_id"] = consumerKey;
    sortMap["redirect_uri"] = "urn:ietf:wg:oauth:2.0:oob";
    sortMap["scope"] = authorizationScope;
    sortMap["state"] = nonce;
    sortMap["access_type"] = "offline";
    QString queryString = createQueryString(sortMap);
    qDebug() << "queryString " << queryString;

    // Send signal to redirect to URL.
    emit authorizeRedirectSignal(nonce, authorizeURI + "?" + queryString, objectName());
}

void GCDClient::accessToken(QString nonce, QString pin)
{
    qDebug() << "----- GCDClient::accessToken -----" << pin;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["code"] = pin;
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["redirect_uri"] = "urn:ietf:wg:oauth:2.0:oob";
    sortMap["grant_type"] = "authorization_code";
    QString queryString = createQueryString(sortMap);
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

void GCDClient::refreshToken(QString nonce, QString uid)
{
    qDebug() << "----- GCDClient::refreshToken -----";

    if (accessTokenPairMap[uid].secret == "") {
        qDebug() << "GCDClient::refreshToken refreshToken is empty. Operation is aborted.";
        emit accessTokenReplySignal(nonce, -1, "Refresh token is missing", "Refresh token is missing. Please authorize GoogleDrive account again.");
        return;
    }

    refreshTokenUid = uid;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["refresh_token"] = accessTokenPairMap[uid].secret;
    sortMap["client_id"] = consumerKey;
    sortMap["client_secret"] = consumerSecret;
    sortMap["grant_type"] = "refresh_token";
    QString queryString = createQueryString(sortMap);
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

void GCDClient::accountInfo(QString nonce, QString uid)
{
    qDebug() << "----- GCDClient::accountInfo -----" << uid;

    QString accessToken;
    if (uid == "") {
        // After accessToken, then uses temp access token..
        accessToken = m_paramMap["access_token"];
    } else {
        accessToken = accessTokenPairMap[uid].token;
    }

    QString uri = accountInfoURI;
    qDebug() << "GCDClient::accountInfo uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(accountInfoReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QByteArray().append("Bearer ").append(accessToken));
    manager->get(req);
}

void GCDClient::quota(QString nonce, QString uid)
{
    qDebug() << "----- GCDClient::quota ----- uid" << uid;

    QString uri = quotaURI;
    qDebug() << "GCDClient::quota uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(quotaReplyFinished(QNetworkReply*)));
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    manager->get(req);
}

void GCDClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::metadata -----" << uid << remoteFilePath;
    if (remoteFilePath.isEmpty()) {
        emit metadataReplySignal(nonce, -1, "remoteFilePath is empty.", "");
        return;
    }

    property(nonce, uid, remoteFilePath, false, false, "metadata");
    files(nonce, uid, remoteFilePath, 0, false, "metadata");
}

void GCDClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::browse -----" << uid << remoteFilePath;

    // remoteFilePath can be empty while browsing from DrivePage.
    property(nonce, uid, remoteFilePath, true, false, "browse");
    files(nonce, uid, remoteFilePath, 0, false, "browse");
}

QString GCDClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    qDebug() << "----- GCDClient::createFolder -----" << remoteParentPath << newRemoteFolderName;

    if (remoteParentPath.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return "";
    }

    if (newRemoteFolderName.isEmpty()) {
        if (!synchronous) emit createFolderReplySignal(nonce, -1, "newRemoteFolderName is empty.", "");
        return "";
    }

    // Search for existing folder.
    QString folderId = searchFileId(nonce, uid, remoteParentPath, newRemoteFolderName);
    if (folderId != "") {
        qDebug() << "GCDClient::createFolder" << nonce << "found existing folderId" << folderId;
        return createFolderReplyFinished(property(nonce, uid, folderId, true), synchronous);
    }

    QString uri = createFolderURI;
    qDebug() << "GCDClient::createFolder uri " << uri;
/*
    {
      "title": "pets",
      "parents": [{"id":"0ADK06pfg"}]
      "mimeType": "application/vnd.google-apps.folder"
    }
*/
    QByteArray postData;
    postData.append("{");
    postData.append(" \"title\": \"" + newRemoteFolderName.toUtf8() + "\", ");
    postData.append(" \"parents\": [{\"id\":\"" + remoteParentPath.toUtf8() + "\"}], ");
    postData.append(" \"mimeType\": \"application/vnd.google-apps.folder\" ");
    postData.append("}");
    qDebug() << "GCDClient::createFolder postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(createFolderReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
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

void GCDClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- GCDClient::moveFile -----" << uid << remoteFilePath << targetRemoteParentPath << newRemoteFileName;

    QByteArray postData;
    if (newRemoteFileName != "" && targetRemoteParentPath == "") {
        // Renaming.
        postData.append("{ \"title\": \"" + newRemoteFileName.toUtf8() + "\" }");
    } else {
        // Moving.
        postData.append("{ \"parents\": [{ \"id\": \"" + targetRemoteParentPath.toUtf8() + "\" }] }");
    }

    // Send request.
    QNetworkReply * reply = patchFile(nonce, uid, remoteFilePath, postData);

    // Proceed to slot.
    moveFileReplyFinished(reply);
}

QNetworkReply * GCDClient::patchFile(QString nonce, QString uid, QString remoteFilePath, QByteArray postData)
{
    qDebug() << "----- GCDClient::patchFile -----" << nonce << uid << remoteFilePath << postData;

    QString uri = patchFileURI.arg(remoteFilePath).arg(m_settings.value("GCDClient.patchFile.setModifiedDate.enabled", false).toString());
    qDebug() << "GCDClient::patchFile" << nonce << "uri" << uri;

    // Insert buffer to hash.
    QBuffer *postDataBuf = new QBuffer();
    postDataBuf->open(QIODevice::WriteOnly);
    postDataBuf->write(postData);
    postDataBuf->close();

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(postData));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
    QNetworkReply *reply = manager->sendCustomRequest(req, "PATCH", postDataBuf);

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    postDataBuf->deleteLater();

    return reply;
}

QIODevice *GCDClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- GCDClient::fileGet -----" << nonce << uid << remoteFilePath << "offset" << offset << "synchronous" << synchronous;

    QString uri = remoteFilePath;
    // TODO It should be downloadUrl because it will not be able to create connection in CloudDriveModel.fileGetReplyFilter.
    if (!remoteFilePath.startsWith("http")) {
        qDebug() << "GCDClient::fileGet" << nonce << uid << remoteFilePath << "getting property to get download URL";

        // remoteFilePath is not a URL. Procees getting property to get downloadUrl.
        if (m_downloadUrlHash->contains(remoteFilePath)) {
            uri = m_downloadUrlHash->value(remoteFilePath);
            qDebug() << "GCDClient::fileGet" << nonce << uid << remoteFilePath << "found cached download URL" << uri;
        } else {
            QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "fileGet");
            if (propertyReply->error() == QNetworkReply::NoError) {
                // Stores property for using in fileGetReplyFinished().
                m_propertyReplyHash->insert(nonce, propertyReply->readAll());
                qDebug() << "GCDClient::fileGet" << nonce << uid << remoteFilePath << "propertyReplyBody" << QString::fromUtf8(m_propertyReplyHash->value(nonce));

                QScriptEngine engine;
                QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
                if (sc.property("downloadUrl").isValid()) {
                    uri = sc.property("downloadUrl").toString();
                    // Cache downloadUrl for furthur usage.
                    m_downloadUrlHash->insert(sc.property("id").toString(), uri);
                } else if (isCloudOnly(sc)) {
                    qDebug() << "GCDClient::fileGet" << nonce << uid << remoteFilePath << "is cloud document format. It can't be downloaded.";
                    emit logRequestSignal(nonce,
                                          "warn",
                                          "GoogleDrive " + tr("File Get"),
                                          tr("%1 is cloud document format.\nIt can't be downloaded.").arg(sc.property("title").toString()),
                                          2000);
                } else {
                    qDebug() << "GCDClient::fileGet" << nonce << uid << remoteFilePath << "downloadUrl is not available.";
                }
                propertyReply->deleteLater();
            } else {
                if (!synchronous) {
                    emit fileGetReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
                    propertyReply->deleteLater();
                    return 0;
                } else {
                    return propertyReply;
                }
            }
        }
    }
    qDebug() << "GCDClient::fileGet" << nonce << uid << remoteFilePath << "download uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "GCDClient::fileGet rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
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

QNetworkReply *GCDClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- GCDClient::filePut -----" << nonce << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    // Search for fileId.
    QString fileId = searchFileId(nonce, uid, remoteParentPath, remoteFileName);
    fileId = (fileId != "") ? ("/" + fileId) : fileId;
    qDebug() << "GCDClient::filePut nonce" << nonce << "fileId" << fileId;

    QString uri = filePutURI.arg(fileId) + "?uploadType=media";
    qDebug() << "GCDClient::filePut nonce" << nonce << "uri" << uri;

    QString contentType = getContentType(remoteFileName);
    qDebug() << "GCDClient::filePut nonce" << nonce << "remoteFileName" << remoteFileName << "contentType" << contentType;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteParentPath));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(remoteFileName));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 4), QVariant(fileId));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(contentType));
    req.setHeader(QNetworkRequest::ContentLengthHeader, bytesTotal);
    QNetworkReply *reply = (fileId != "") ? manager->put(req, source) : manager->post(req, source);
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

        if (reply->error() == QNetworkReply::NoError) {
            QScriptEngine engine;
            QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(reply->readAll()) + ")");
            QString remoteFilePath = sc.property("id").toString();

            QByteArray metadata;
            metadata.append("{");
            metadata.append(" \"title\": \"" + remoteFileName.toUtf8() + "\", ");
            if (m_sourceFileTimestampHash.contains(nonce)) {
                metadata.append(" \"modifiedDate\": \"" + formatJSONDateString(m_sourceFileTimestampHash[nonce]) + "\", ");
            }
            metadata.append(" \"parents\": [{ \"id\": \"" + remoteParentPath.toUtf8() + "\" }] ");
            metadata.append("}");
            qDebug() << "GCDClient::filePut nonce" << nonce << "patch metadata" << metadata;

            // Remove used source file timestamp.
            m_sourceFileTimestampHash.remove(nonce);

            reply = patchFile(nonce, uid, remoteFilePath, metadata);
        }
    }

    return reply;
}

QNetworkReply *GCDClient::filePutMulipart(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    qDebug() << "----- GCDClient::filePutMulipart -----" << remoteParentPath << remoteFileName << "synchronous" << synchronous << "source->bytesAvailable()" << source->bytesAvailable() << "bytesTotal" << bytesTotal;

    // Search for fileId.
    QString fileId = searchFileId(nonce, uid, remoteParentPath, remoteFileName);
    fileId = (fileId != "") ? ("/" + fileId) : fileId;

    QString uri = filePutURI.arg(fileId) + "?uploadType=multipart";
    qDebug() << "GCDClient::filePutMulipart uri " << uri;

    if (source->open(QIODevice::ReadOnly)) {
        qint64 fileSize = source->size();
        QString contentType = getContentType(remoteFileName);
        qDebug() << "GCDClient::filePutMulipart remoteFileName" << remoteFileName << "contentType" << contentType << "fileSize" << fileSize;

        // Requires to submit job with multipart.
        QString boundary = "----------" + nonce;
        QString CRLF = "\r\n";

        QByteArray metadata;
        metadata.append("{");
        metadata.append(" \"title\": \"" + remoteFileName.toUtf8() + "\", ");
        if (m_sourceFileTimestampHash.contains(nonce)) {
            metadata.append(" \"modifiedDate\": \"" + formatJSONDateString(m_sourceFileTimestampHash[nonce]) + "\", ");
        }
        metadata.append(" \"parents\": [{ \"id\": \"" + remoteParentPath.toUtf8() + "\" }] ");
        metadata.append("}");
        qDebug() << "GCDClient::filePutMulipart metadata " << metadata;

        QByteArray postData;
        postData.append("--" + boundary + CRLF);
        postData.append("Content-Type: application/json; charset=UTF-8" + CRLF);
        postData.append(CRLF);
        postData.append(metadata);
        postData.append(CRLF);
        postData.append("--" + boundary + CRLF);
        postData.append("Content-Type: " + contentType + CRLF);
        postData.append(CRLF);
        postData.append(source->readAll());
        postData.append(CRLF);
        postData.append("--" + boundary + "--" + CRLF);
        qDebug() << "postData size" << postData.size();

        // Insert buffer to hash.
        m_bufferHash.insert(nonce, new QBuffer());
        m_bufferHash[nonce]->open(QIODevice::WriteOnly);
        m_bufferHash[nonce]->write(postData);
        m_bufferHash[nonce]->close();

        if (m_bufferHash[nonce]->open(QIODevice::ReadOnly)) {
            // Send request.
            CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
            if (!synchronous) {
                connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutMultipartReplyFinished(QNetworkReply*)));
            }
            QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
            req.setAttribute(QNetworkRequest::User, QVariant(nonce));
            req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
            req.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/related; boundary=\"" + boundary + "\"");
            req.setHeader(QNetworkRequest::ContentLengthHeader, postData.length());
            QNetworkReply *reply = (fileId != "") ? manager->put(req, m_bufferHash[nonce]->readAll()) : manager->post(req, m_bufferHash[nonce]->readAll());
            connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)));
            QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

            // Store reply for further usage.
            m_replyHash->insert(nonce, reply);

            while (synchronous && !reply->isFinished()) {
                QApplication::processEvents(QEventLoop::AllEvents, 100);
                Sleeper::msleep(100);
            }

            // TODO Delete buffer.
            m_bufferHash.remove(nonce);

            return reply;
        }
    } else {
        qDebug() << "GCDClient::filePutMulipart source can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open source.", "Can't open source.");
    }

    return 0;
}

QIODevice *GCDClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- GCDClient::fileGetResume -----" << remoteFilePath << "to" << localFilePath << "offset" << offset;

    QString uri = remoteFilePath;
    // TODO It should be downloadUrl because it will not be albe to create connection in CloudDriveModel.fileGetReplyFilter.
    if (!remoteFilePath.startsWith("http")) {
        // remoteFilePath is not a URL. Procees getting property to get downloadUrl.
        QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "fileGetResume");
        if (propertyReply->error() == QNetworkReply::NoError) {
            // For further using in fileGetReplyFinished.
            m_propertyReplyHash->insert(nonce, propertyReply->readAll());

            QScriptEngine engine;
            QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
            if (sc.property("downloadUrl").isValid()) {
                uri = sc.property("downloadUrl").toString();
                // Cache downloadUrl for furthur usage.
                m_downloadUrlHash->insert(sc.property("id").toString(), uri);
            } else if (isCloudOnly(sc)) {
                qDebug() << "GCDClient::fileGetResume" << nonce << uid << remoteFilePath << "is cloud document format. It can't be downloaded.";
                emit logRequestSignal(nonce,
                                      "warn",
                                      "GoogleDrive " + tr("File Get"),
                                      tr("%1 is cloud document format.\nIt can't be downloaded.").arg(sc.property("title").toString()),
                                      2000);
            } else {
                qDebug() << "GCDClient::fileGetResume" << nonce << uid << remoteFilePath << "downloadUrl is not available.";
            }
            propertyReply->deleteLater();
        } else {
            emit fileGetReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()));
            propertyReply->deleteLater();
            return 0;
        }
    }
    qDebug() << "GCDClient::fileGetResume uri " << uri;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetResumeReplyFinished(QNetworkReply*)) );
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    if (offset >= 0) {
        QString rangeHeader = QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1);
        qDebug() << "GCDClient::fileGetResume rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    QNetworkReply *reply = manager->get(req);
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    return reply;
}

QNetworkReply *GCDClient::filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset)
{
    qDebug() << "----- GCDClient::filePutResume -----" << nonce << uid << localFilePath << remoteParentPath << remoteFileName << uploadId << offset;

    if (localFilePath.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "localFilePath is empty.", "");
        return 0;
    }

    if (remoteParentPath.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return 0;
    }

    if (remoteFileName.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "remoteFileName is empty.", "");
        return 0;
    }

    QFileInfo localFileInfo(localFilePath);
    m_sourceFileTimestampHash[nonce] = localFileInfo.lastModified();

    if (uploadId == "") {
        qDebug() << "GCDClient::filePutResume redirect to filePutResumeStart" << uploadId << offset;
        filePutResumeStart(nonce, uid, remoteFileName, localFileInfo.size(), remoteParentPath, false);
    } else {
        if (offset < 0) {
            // Request latest uploading status if offset = -1.
            qDebug() << "GCDClient::filePutResume redirect to filePutResumeStatus" << uploadId << offset;
            filePutResumeStatus(nonce, uid, remoteFileName, localFileInfo.size(), uploadId, offset, false);
        } else {
            qDebug() << "GCDClient::filePutResume redirect to filePutResumeUpload" << uploadId << offset;
            m_localFileHash[nonce] = new QFile(localFilePath);
            QFile *localSourceFile = m_localFileHash[nonce];
            if (localSourceFile->open(QIODevice::ReadOnly)) {
                qint64 fileSize = localSourceFile->size();

                localSourceFile->seek(offset);

                filePutResumeUpload(nonce, uid, localSourceFile, remoteFileName, fileSize, uploadId, offset, false);
            } else {
                qDebug() << "GCDClient::filePutResumeUpload file " << localFilePath << " can't be opened.";
                emit filePutResumeReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
            }
        }
    }
    return 0;
}

QString GCDClient::filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous)
{
    qDebug() << "----- GCDClient::filePutResumeStart -----" << nonce << uid << fileName << bytesTotal << remoteParentPath << "synchronous" << synchronous;

    if (remoteParentPath.isEmpty()) {
        emit filePutResumeReplySignal(nonce, -1, "remoteParentPath is empty.", "");
        return 0;
    }

    QString contentType = getContentType(fileName);

    // Search for fileId.
    QString fileId = searchFileId(nonce, uid, remoteParentPath, fileName);
    fileId = (fileId != "") ? ("/" + fileId) : fileId;

    QString uri = startResumableUploadURI.arg(fileId);
    qDebug() << "GCDClient::filePutResumeStart uri " << uri;
/*
    {
      "title": "pets",
      "parents": [{"id":"0ADK06pfg"}]
    }
*/
    // NOTE It can't support unicode file name. Stores fileName to be patched in filePutResumeUploadReplyFinished().
    m_sourceFileNameHash[nonce] = fileName;
    QString postString = QString("{ \"parents\": [{\"id\":\"%1\"}] }").arg(remoteParentPath);
    qDebug() << "GCDClient::filePutResumeStart postString" << postString;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeStartReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setRawHeader("X-Upload-Content-Type", contentType.toAscii() );
//    req.setRawHeader("X-Upload-Content-Length", QString("%1").arg(bytesTotal).toAscii() ); // NOTE It's optional.
    req.setHeader(QNetworkRequest::ContentLengthHeader, postString.toUtf8().length());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
    QNetworkReply *reply;
    if (fileId != "") {
        reply = manager->put(req, postString.toUtf8());
        qDebug() << "GCDClient::filePutResumeStart" << nonce << "found existing file id" << fileId << ". PUT";
    } else {
        reply = manager->post(req, postString.toUtf8());
        qDebug() << "GCDClient::filePutResumeStart" << nonce << "no existing file. POST";
    }

    // Return immediately if it's not synchronous.
    if (!synchronous) return "";

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Construct result.
    // TODO To patch title, modified date.
    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        QString uploadUrl = reply->header(QNetworkRequest::LocationHeader).toString();
        result = QString("{ \"upload_id\": \"%1\" }").arg(uploadUrl);
    } else {
        qDebug() << "GCDClient::filePutResumeStart" << nonce << uid << fileName << bytesTotal << remoteParentPath << "error" << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString GCDClient::filePutResumeUpload(QString nonce, QString uid, QIODevice *source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    /*
     *NOTE
     *source must be seeked to required offset before invoking this method.
     *bytesTotal is total source file size.
     *offset is uploading offset.
     */
    qDebug() << "----- GCDClient::filePutResumeUpload -----" << nonce << uid << fileName << bytesTotal << uploadId << offset << "synchronous" << synchronous;

    QString uri = resumeUploadURI.arg(uploadId);
    QUrl url(uri);
    qDebug() << "GCDClient::filePutResumeUpload url " << url;

    qint64 availableBytes = qMin(source->bytesAvailable(), bytesTotal-offset);
    qint64 chunkSize = qMin(availableBytes, getChunkSize());
    // NOTE Adjust last chunk's bytesTotal to support BOX,SkyDrive,WebDAV. As its downloaded file size may not be the same as reported by browsing.
    qint64 actualBytesTotal = ((offset+chunkSize) < getChunkSize()) ? (offset+chunkSize) : bytesTotal;
    QString contentRange = QString("bytes %1-%2/%3").arg(offset).arg(offset+chunkSize-1).arg(actualBytesTotal);
    qDebug() << "GCDClient::filePutResumeUpload source->bytesAvailable()" << source->bytesAvailable()
             << "source->size()" << source->size() << "source->pos()" << source->pos()
             << "bytesTotal" << bytesTotal
             << "offset" << offset << "chunkSize" << chunkSize
             << "contentRange" << contentRange;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeUploadReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(url);
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant("")); // NOTE remoteParentPath is empty as it's set on uploadId since filePutResumeStart()
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3), QVariant(fileName));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, chunkSize);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(getContentType(fileName)));
    req.setRawHeader("Content-Range", contentRange.toAscii() );
    QNetworkReply *reply = manager->put(req, source);
    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage. Ex. abort from job page.
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
    QString result = filePutResumeUploadReplyFinished(reply, synchronous);

    return result;
}

QString GCDClient::filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    qDebug() << "----- GCDClient::filePutResumeStatus -----" << nonce << uid << fileName << bytesTotal << uploadId << offset << "synchronous" << synchronous;

    QString uri = resumeUploadURI.arg(uploadId);
    qDebug() << "GCDClient::filePutResumeStatus uri " << uri;

    // Send request.
    // Put empty request with Content-Length: 0, Content-Range: bytes */<fileSize>.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(filePutResumeStatusReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentLengthHeader, 0);
    req.setRawHeader("Content-Range", QString("bytes */*").toAscii() );
    QNetworkReply *reply = manager->put(req, QByteArray());

    // Return immediately if it's not synchronous.
    if (!synchronous) return "";

    while (synchronous && !reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    // Construct result.
    QString result = "";
    if (reply->error() == QNetworkReply::NoError) {
        if (reply->hasRawHeader("Range")) {
            QStringList ranges = QString::fromAscii(reply->rawHeader("Range")).split("-");
            qint64 offset = ranges.at(1).toUInt() + 1;
            qDebug() << "GCDClient::filePutResumeStatus ranges" << ranges << "offset" << offset;
            result = QString("{ \"offset\": %1 }").arg(offset);
        } else {
            result = QString::fromUtf8(reply->readAll());
        }
    } else {
        result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

QString GCDClient::filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous)
{
    return "";
}

QString GCDClient::delta(QString nonce, QString uid, bool synchronous)
{
    qDebug() << "----- GCDClient::delta -----" << nonce << uid << synchronous;

    // Construct query string.
    QMap<QString, QString> sortMap;
    if (m_settings.value(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid)).isValid()) {
        sortMap["startChangeId"] = m_settings.value(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid)).toString();
    }
    sortMap["maxResults"] = m_settings.value(objectName() + "." + uid + ".changesPerPage", QVariant(100)).toString();
//    sortMap["pageToken"] = ""; // Omits and always use nextDeltaCursor for next delta request?
    QString queryString = createQueryString(sortMap);
    qDebug() << "GCDClient::delta queryString" << queryString;

    QString uri = deltaURI + "?" + queryString;
    qDebug() << "GCDClient::delta uri " << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deltaReplyFinished(QNetworkReply*)));
    }
    QNetworkRequest req = QNetworkRequest(QUrl(uri));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);

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

QString GCDClient::media(QString nonce, QString uid, QString remoteFilePath)
{
    qDebug() << "----- GCDClient::media -----" << nonce << uid << remoteFilePath;

    QString uri = remoteFilePath;
    // NOTE It shouldn't be downloadUrl because it will not be able to create connection in CloudDriveModel.fileGetReplyFilter.
    if (!remoteFilePath.startsWith("http")) {
        // remoteFilePath is not a URL. Procees getting property to get downloadUrl.
        if (m_downloadUrlHash->contains(remoteFilePath)) {
            uri = m_downloadUrlHash->value(remoteFilePath);
            qDebug() << "GCDClient::media" << nonce << uid << remoteFilePath << "found cached download URL" << uri;
        } else {
            QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "media");
            if (propertyReply->error() == QNetworkReply::NoError) {
                QScriptEngine engine;
                QScriptValue sc = engine.evaluate("(" + QString::fromUtf8(propertyReply->readAll()) + ")");
                uri = sc.property("downloadUrl").toString();
                // Cache downloadUrl for furthur usage.
                m_downloadUrlHash->insert(sc.property("id").toString(), uri);
                propertyReply->deleteLater();
            } else {
                qDebug() << "GCDClient::media" << nonce << "propertyReply" << propertyReply->error() << propertyReply->errorString() << QString::fromUtf8(propertyReply->readAll());
                uri = "";
            }
        }
    }

    // Append with OAuth token.
    if (uri != "") {
        uri += "&access_token=" + accessTokenPairMap[uid].token;
    }

    qDebug() << "GCDClient::media" << nonce << "uri" << uri;
    return uri;
}

bool GCDClient::isFilePutResumable(qint64 fileSize)
{
    return (fileSize == -1 || fileSize >= getChunkSize());
}

bool GCDClient::isFileGetResumable(qint64 fileSize)
{
    return (fileSize == -1 || fileSize >= getChunkSize());
}

bool GCDClient::isDeltaSupported()
{
    return true;
}

bool GCDClient::isDeltaEnabled(QString uid)
{
    return m_settings.value(QString("%1.%2.delta.enabled").arg(objectName()).arg(uid), QVariant(false)).toBool();
}

bool GCDClient::isViewable()
{
    return true;
}

void GCDClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName)
{
    qDebug() << "----- GCDClient::copyFile -----" << uid << remoteFilePath << targetRemoteParentPath;

    QString uri = copyFileURI.arg(remoteFilePath);
    qDebug() << "GCDClient::copyFile uri " << uri;

    QByteArray postData;
    postData.append("{");
    if (newRemoteFileName != "") {
        postData.append(" \"title\": \"" + newRemoteFileName.toUtf8() + "\", ");
    }
    postData.append(" \"parents\": [{ \"id\": \"" + targetRemoteParentPath.toUtf8() + "\" }] ");
    postData.append("}");
    qDebug() << "GCDClient::copyFile postData" << postData;

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
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
        manager->sendCustomRequest(req, "POST", m_bufferHash[nonce]);
    }
}

QString GCDClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- GCDClient::deleteFile -----" << nonce << uid << remoteFilePath << synchronous;

    QString uri;
    if (m_settings.value("GCDClient.deleteFile.permanently.enabled", false).toBool()) {
        uri = deleteFileURI.arg(remoteFilePath);
    } else {
        uri = trashFileURI.arg(remoteFilePath);
    }
    qDebug() << "GCDClient::deleteFile uri " << uri;

    QByteArray postData;
    qDebug() << "GCDClient::deleteFile postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(deleteFileReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply;
    if (m_settings.value("GCDClient.deleteFile.permanently.enabled", false).toBool()) {
        reply = manager->deleteResource(req);
    } else {
        reply = manager->post(req, postData);
    }

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

QString GCDClient::shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    qDebug() << "----- GCDClient::shareFile -----" << nonce << uid << remoteFilePath << synchronous;
    if (remoteFilePath.isEmpty()) {
        if (!synchronous) emit shareFileReplySignal(nonce, -1, "remoteFilePath is empty.", "", "", 0);
        return "";
    }

    QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, true, "shareFile");
    if (propertyReply->error() == QNetworkReply::NoError) {
        // For further using in shareFileReplyFinished.
        m_propertyReplyHash->insert(nonce, propertyReply->readAll());
        propertyReply->deleteLater();
    } else {
        if (!synchronous) emit shareFileReplySignal(nonce, propertyReply->error(), propertyReply->errorString(), QString::fromUtf8(propertyReply->readAll()), "", 0);
        propertyReply->deleteLater();
        return "";
    }

    QString uri = sharesURI.arg(remoteFilePath);
    qDebug() << "GCDClient::shareFile uri " << uri;

    QByteArray postData;
    postData.append("{");
    postData.append(" \"role\": \"reader\", ");
    postData.append(" \"type\": \"anyone\", ");
    postData.append(" \"value\": \"me\", ");
    postData.append(" \"withLink\": true ");
    postData.append("}");
    qDebug() << "GCDClient::shareFile postData" << postData;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    if (!synchronous) {
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(shareFileReplyFinished(QNetworkReply*)) );
    }
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=UTF-8");
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

QNetworkReply * GCDClient::files(QString nonce, QString uid, QString remoteFilePath, int offset, bool synchronous, QString callback)
{
    qDebug() << "----- GCDClient::files -----" << nonce << uid << remoteFilePath << synchronous << callback;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["key"] = consumerKey;
    sortMap["maxResults"] = "10000"; // TODO Implement using pageToken. Workaround, using default value 10000 (the same as Dropbox).
    sortMap["q"] = QUrl::toPercentEncoding(QString("'%1' in parents and trashed = false").arg((remoteFilePath == "") ? RemoteRoot : remoteFilePath));
    QString queryString = createQueryString(sortMap);
    qDebug() << "GCDClient::files" << nonce << "queryString" << queryString;

    QString uri = filesURI + "?" + queryString;
    qDebug() << "GCDClient::files" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
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

QString GCDClient::searchFileId(QString nonce, QString uid, QString remoteParentPath, QString remoteFileName)
{
    qDebug() << "----- GCDClient::search -----" << nonce << uid << remoteParentPath << remoteFileName;

    // Construct normalized query string.
    QMap<QString, QString> sortMap;
    sortMap["key"] = consumerKey;
    sortMap["q"] = QUrl::toPercentEncoding(QString("'%1' in parents and title = '%2' and trashed = false").arg((remoteParentPath == "") ? RemoteRoot : remoteParentPath).arg(remoteFileName));
    QString queryString = createQueryString(sortMap);
    qDebug() << "GCDClient::search" << nonce << "queryString" << queryString;

    QString uri = filesURI + "?" + queryString;
    qDebug() << "GCDClient::search" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setRawHeader("Authorization", QString("Bearer " + accessTokenPairMap[uid].token).toAscii() );
    QNetworkReply *reply = manager->get(req);

    while (!reply->isFinished()) {
        QApplication::processEvents(QEventLoop::AllEvents, 100);
        Sleeper::msleep(100);
    }

    QString fileId;
    QString replyBody = QString::fromUtf8(reply->readAll());
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue filesObj = engine.evaluate("(" + replyBody + ")");
        int contentsCount = filesObj.property("items").property("length").toInt32();
        qDebug() << "GCDClient::search" << nonce << "replyBody" << replyBody << "contentsCount" << contentsCount;
        if (contentsCount > 0) {
            fileId = filesObj.property("items").property(0).property("id").toString();
            // TODO Cache.
        }
    } else {
        qDebug() << "GCDClient::search" << nonce << "error" << reply->error() << replyBody;
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return fileId;
}

QNetworkReply * GCDClient::property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous, QString callback)
{
    qDebug() << "----- GCDClient::property -----" << nonce << uid << remoteFilePath << synchronous << callback;

    QString uri = propertyURI.arg((remoteFilePath == "") ? RemoteRoot : remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "GCDClient::property" << nonce << "uri" << uri;

    // Send request.
    CustomQNetworkAccessManager *manager = new CustomQNetworkAccessManager(this);
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

void GCDClient::accessTokenReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::accessTokenReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
/*
 * Example response
{
  "access_token" : "ya29.AHES6ZQaY6RXd94kCKwCgppM3CKWlYXQ9BSR3t_7VpbWn86DPn_FWQ",
  "token_type" : "Bearer",
  "expires_in" : 3600,
  "id_token" : "eyJhbGciOiJSUzI1NiIsImtpZCI6IjNiNDcwODgyYjMwZTU5NTk5OTc2MzY5ZDdiMzFjZjBiOTBhMmEzNzkifQ.eyJpc3MiOiJhY2NvdW50cy5nb29nbGUuY29tIiwiYXVkIjoiMTk2NTczMzc5NDk0LmFwcHMuZ29vZ2xldXNlcmNvbnRlbnQuY29tIiwiZW1haWwiOiJ0YXdlZXNhay5raXR0aXB1cmVlQGdtYWlsLmNvbSIsInRva2VuX2hhc2giOiJMTFMwbUZtb2ZuVVFSb3o2UUlHVmhRIiwiY2lkIjoiMTk2NTczMzc5NDk0LmFwcHMuZ29vZ2xldXNlcmNvbnRlbnQuY29tIiwidmVyaWZpZWRfZW1haWwiOiJ0cnVlIiwiaWQiOiIxMTIwMzIzMTI3NjEyMTM1ODkzMDgiLCJpYXQiOjEzNTY1Nzg1ODMsImV4cCI6MTM1NjU4MjQ4M30.TVZLBHkR1oRbAA2_kl-Mx7oBg8iNwUXgvGDEKZGxJCy4eZno_gSTbozePAK24N2OrJTVgQ-yPGX5d9JrY9nmdOG1KWkahgLL_Hl1vaOfjr4ELadBo5Vj8nvQTeu35meRw275NFQvMjKKdRM3aR8AYPKIgUvG4nMuHPwb9khkDwg",
  "refresh_token" : "1/iF_z2SVQwnsU7LLmQpPZHM1PkjIBG_QL1qN-bkb0_2Q"
}
 */
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        m_paramMap["access_token"] = sc.property("access_token").toString();
        m_paramMap["refresh_token"] = sc.property("refresh_token").toString();
        m_paramMap["id_token"] = sc.property("id_token").toString();
        qDebug() << "GCDClient::accessTokenReplyFinished m_paramMap " << m_paramMap;

        if (refreshTokenUid != "" && sc.property("access_token").toString() != "") {
            // NOTE first accessTokenReply will contain both accessToken and refreshToken. But for refresh's accessTokenReply will contain only accessToken.
            // Update accessToken but retains refreshToken.
            accessTokenPairMap[refreshTokenUid].token = sc.property("access_token").toString();
            qDebug() << "GCDClient::accessTokenReplyFinished accessTokenPairMap" << accessTokenPairMap;

            // Reset refreshTokenUid.
            refreshTokenUid = "";

            // Save tokens.
            saveAccessPairMap();
        }

        // NOTE uid and email will be requested to accountInfo by CloudDriveModel.accessTokenReplyFilter().
    }

    emit accessTokenReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::accountInfoReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::accountInfoReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = QString(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
/*
 * Example response.
{
 "id": "112032312761213589308",
 "email": "taweesak.kittipuree@gmail.com",
 "verified_email": true,
 "name": "Taweesak Kittipuree",
 "given_name": "Taweesak",
 "family_name": "Kittipuree",
 "link": "https://plus.google.com/112032312761213589308",
 "gender": "male",
 "locale": "en"
}
 */
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + replyBody + ")");
        QString uid = sc.property("id").toString();
        QString name = sc.property("name").toString();
        QString email = sc.property("email").toString();
        qDebug() << "GCDClient::accountInfoReplyFinished m_paramMap" << m_paramMap;

        if (accessTokenPairMap.contains(uid)) {
            qDebug() << "GCDClient::accountInfoReplyFinished found existing accessToken for uid" << uid << accessTokenPairMap[uid];

            accessTokenPairMap[uid].email = email;

            if (m_paramMap.contains("access_token")) {
                accessTokenPairMap[uid].token = m_paramMap["access_token"];
                if (m_paramMap.contains("refresh_token") && m_paramMap["refresh_token"] != "") {
                    accessTokenPairMap[uid].secret = m_paramMap["refresh_token"];
                }

                // Reset temp. accessToken and refreshToken.
                m_paramMap.remove("access_token");
                m_paramMap.remove("refresh_token");
            }
        } else {
            qDebug() << "GCDClient::accountInfoReplyFinished not found existing accessToken for uid" << uid;

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

//        qDebug() << "GCDClient::accountInfoReplyFinished accessTokenPairMap" << accessTokenPairMap;

        // Save account after got id and email.
        saveAccessPairMap();
    }

    emit accountInfoReplySignal(nonce, reply->error(), reply->errorString(), replyBody );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::quotaReplyFinished(QNetworkReply *reply)
{
/*
 * Example response
{ ...
 "quotaBytesTotal": "5368709120",
 "quotaBytesUsed": "4687520",
 "quotaBytesUsedAggregate": "4687520",
 "quotaBytesUsedInTrash": "4679290",
... }
 */
    qDebug() << "GCDClient::quotaReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString replyBody = QString::fromUtf8(reply->readAll());

    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
        qint64 sharedValue = 0;
        qint64 normalValue = jsonObj.property("quotaBytesUsed").toInteger();
        qint64 quotaValue = jsonObj.property("quotaBytesTotal").toInteger();

        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, normalValue, sharedValue, quotaValue);
    } else {
        emit quotaReplySignal(nonce, reply->error(), reply->errorString(), replyBody, 0, 0, -1);
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString GCDClient::fileGetReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
        if (m_propertyReplyHash->contains(nonce)) {
            QString propertyReplyBody = QString::fromUtf8(m_propertyReplyHash->value(nonce));
            qDebug() << "GCDClient::fileGetReplySave propertyReplyBody" << propertyReplyBody;

            // Return common json.
            QScriptEngine engine;
            QScriptValue jsonObj = engine.evaluate("(" + propertyReplyBody + ")");
            QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
            result = stringifyScriptValue(engine, parsedObj);
        } else {
            QNetworkReply *propertyReply = property(nonce, uid, remoteFilePath, false, true, "fileGetReplyResult");
            if (propertyReply != 0 && propertyReply->error() == QNetworkReply::NoError) {
                QScriptEngine engine;
                QScriptValue jsonObj = engine.evaluate("(" + QString::fromUtf8(propertyReply->readAll()) + ")");
                QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
                result = stringifyScriptValue(engine, parsedObj);
            } else if (propertyReply == 0) {
                result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(-1).arg("Service is not implemented.");
            } else {
                result = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(propertyReply->error()).arg(propertyReply->errorString());
            }
            propertyReply->deleteLater();
            propertyReply->manager()->deleteLater();
        }
    } else {
        qDebug() << "CloudDriveClient::fileGetReplyResult" << objectName() << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\", \"http_status_code\": %3 }")
                .arg(reply->error())
                .arg(reply->errorString())
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    }

    return result;
}

void GCDClient::filePutMultipartReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::filePutMultipartReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Close source file.
    QFile *localTargetFile = m_localFileHash[nonce];
    localTargetFile->close();
    m_localFileHash.remove(nonce);

    // Remove request buffer.
    if (m_bufferHash.contains(nonce)) {
        m_bufferHash[nonce]->close();
        m_bufferHash.remove(nonce);
    }

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "GCDClient::filePutMultipartReplyFinished replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Remove used source file timestamp.
    m_sourceFileTimestampHash.remove(nonce);

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::metadataReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::metadataReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll() );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::browseReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::browseReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll() );

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::mergePropertyAndFilesJson(QString nonce, QString callback)
{
    if (m_propertyReplyHash->contains(nonce) && m_filesReplyHash->contains(nonce)) {
        QScriptEngine engine;
        QScriptValue mergedObj;
        QScriptValue propertyObj;
        QScriptValue filesObj;
        qDebug() << "GCDClient::mergePropertyAndFilesJson" << nonce << "started.";
        if (m_settings.value("Logging.enabled", false).toBool()) {
            qDebug() << "GCDClient::mergePropertyAndFilesJson" << nonce << "propertyJson" << QString::fromUtf8(m_propertyReplyHash->value(nonce));
            qDebug() << "GCDClient::mergePropertyAndFilesJson" << nonce << "filesJson" << QString::fromUtf8(m_filesReplyHash->value(nonce));
        }
        propertyObj = engine.evaluate("(" + QString::fromUtf8(m_propertyReplyHash->value(nonce)) + ")");
        filesObj = engine.evaluate("(" + QString::fromUtf8(m_filesReplyHash->value(nonce)) + ")");

        mergedObj = parseCommonPropertyScriptValue(engine, propertyObj);
        mergedObj.setProperty("children", engine.newArray());
        int contentsCount = filesObj.property("items").property("length").toInteger();
        for (int i = 0; i < contentsCount; i++) {
            QApplication::processEvents();

            mergedObj.property("children").setProperty(i, parseCommonPropertyScriptValue(engine, filesObj.property("items").property(i)));
        }

        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "GCDClient::mergePropertyAndFilesJson" << nonce << "stringifyScriptValue started.";
        QString replyBody = stringifyScriptValue(engine, mergedObj);
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "GCDClient::mergePropertyAndFilesJson" << nonce << "stringifyScriptValue done." << replyBody.size();

        // Remove once used.
        m_propertyReplyHash->remove(nonce);
        m_filesReplyHash->remove(nonce);
        qDebug() << "GCDClient::mergePropertyAndFilesJson m_propertyReplyHash" << m_propertyReplyHash->size() << "m_filesReplyHash" << m_filesReplyHash->size();

        if (callback == "browse") {
            emit browseReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, QNetworkReply::NoError, "", replyBody);
        } else {
            qDebug() << "GCDClient::mergePropertyAndFilesJson invalid callback" << callback;
        }
    }
}

void GCDClient::propertyReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::propertyReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    if (reply->error() == QNetworkReply::NoError) {
        m_propertyReplyHash->insert(nonce, reply->readAll());

        // Merge and emit signal if both property and files are available.
        mergePropertyAndFilesJson(nonce, callback);
    } else {
        // Remove once used.
        m_propertyReplyHash->remove(nonce);
        m_filesReplyHash->remove(nonce);
        qDebug() << "GCDClient::propertyReplyFinished m_propertyReplyHash" << m_propertyReplyHash->size() << "m_filesReplyHash" << m_filesReplyHash->size();

        // Property is mandatory. Emit error if error occurs.
        if (callback == "browse") {
            emit browseReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());
        } else if (callback == "metadata") {
            emit metadataReplySignal(nonce, reply->error(), reply->errorString(), reply->readAll());
        } else {
            qDebug() << "GCDClient::propertyReplyFinished invalid callback" << callback;
        }
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::filesReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::filesReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString callback = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    if (reply->error() == QNetworkReply::NoError) {
        m_filesReplyHash->insert(nonce, reply->readAll());

        // Merge and emit signal if both property and files are available.
        mergePropertyAndFilesJson(nonce, callback);
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
            qDebug() << "GCDClient::filesReplyFinished invalid callback" << callback;
        }
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString GCDClient::createFolderReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    qDebug() << "GCDClient::createFolderReplyFinished" << nonce << reply << QString(" Error=%1").arg(reply->error());

    // Parse common property json.
    // NOTE createFolder() has already searched for existing folder.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "GCDClient::createFolderReplyFinished" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    // Emit signal only for asynchronous request. To avoid invoking CloudDriveModel.createFolderReplyFilter() as it's not required. And also avoid invoking jobDone() which causes issue #FP20130232.
    if (!synchronous) emit createFolderReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

void GCDClient::moveFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::moveFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "GCDClient::moveFileReplyFinished replyBody" << replyBody;
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
        m_bufferHash[nonce]->deleteLater();
        m_bufferHash.remove(nonce);
    }

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::copyFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::copyFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "GCDClient::copyFileReplyFinished replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody  + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit copyFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Remove request buffer.
    m_bufferHash[nonce]->close();
    m_bufferHash[nonce]->deleteLater();
    m_bufferHash.remove(nonce);

    // TODO scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

void GCDClient::deleteFileReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::deleteFileReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    // Parse common property json.
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "GCDClient::deleteFileReplyFinished replyBody" << replyBody;
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

QString GCDClient::shareFileReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "GCDClient::shareFileReplyFinished" << nonce << reply << QString("Error=%1").arg(reply->error());

    QString replyBody = QString::fromUtf8(reply->readAll());
    QScriptEngine engine;
    QScriptValue sc;
    QString url = "";
    int expires = 0;

    if (reply->error() == QNetworkReply::NoError) {
        replyBody = QString::fromUtf8(m_propertyReplyHash->value(nonce));

        sc = engine.evaluate("(" + replyBody + ")");
        // TODO Should it use alternateLink as using in parseCommonPropertyScriptValue()?
        if (sc.property("webContentLink").isValid()) {
            // For file.
            url = sc.property("webContentLink").toString();
        } else if (sc.property("webViewLink").isValid()) {
            // For folder.
            url = sc.property("webViewLink").toString();
        } else if (sc.property("alternateLink").isValid()) {
            // Default web link.
            url = sc.property("alternateLink").toString();
        }
        expires = -1;
    }

    if (!synchronous) emit shareFileReplySignal(nonce, reply->error(), reply->errorString(), replyBody, url, expires);

    // Remove temp property.
    m_propertyReplyHash->remove(nonce);

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return url;
}

void GCDClient::filePutResumeStartReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QByteArray replyBody = reply->readAll();
    qDebug() << "GCDClient::filePutResumeStartReplyFinished" << reply << QString(" Error=%1").arg(reply->error()) << "replyBody" << QString::fromUtf8(replyBody);

    if (reply->error() == QNetworkReply::NoError) {
        QString uploadId = reply->header(QNetworkRequest::LocationHeader).toString();

        // Emit signal with upload_location to start uploading.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString("{ \"upload_id\": \"%1\" }").arg(uploadId) );
    } else {
        // REMARK Use QString::fromUtf8() to support unicode text.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(replyBody));
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString GCDClient::filePutResumeUploadReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QString replyBody = filePutReplyResult(reply);

    if (!synchronous) emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // Close source file.
    if (m_localFileHash.contains(nonce)) {
        QFile *localTargetFile = m_localFileHash[nonce];
        localTargetFile->close();
        m_localFileHash.remove(nonce);
    }

    // Remove used source file timestamp.
    m_sourceFileTimestampHash.remove(nonce);

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

QString GCDClient::filePutReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteParentPath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString(); // NOTE It's empty if invoked from filePutResumeUpload().
    QString remoteFileName = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 3)).toString();
    qDebug() << "GCDClient::filePutReplyResult" << nonce << uid << remoteParentPath << remoteFileName;

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "GCDClient::filePutReplyResult" << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        // Get range and check if resume upload is required.
        if (reply->hasRawHeader("Range")) {
            QStringList ranges = QString::fromAscii(reply->rawHeader("Range")).split("-");
            qint64 offset = ranges.at(1).toUInt() + 1;
            qDebug() << "GCDClient::filePutReplyResult" << nonce << "Range header" << reply->rawHeader("Range") << "ranges" << ranges << "offset" << offset;
            replyBody = QString("{ \"offset\": %1 }").arg(offset);
        } else {
            QScriptEngine engine;
            QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
            QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);

            // Patch uploaded file's name, modified date.
            QByteArray metadata;
            metadata.append("{");
            if (remoteParentPath != "") {
                metadata.append(" \"parents\": [{ \"id\": \"" + remoteParentPath.toUtf8() + "\" }], ");
            }
            if (m_sourceFileTimestampHash.contains(nonce)) {
                metadata.append(" \"modifiedDate\": \"" + formatJSONDateString(m_sourceFileTimestampHash[nonce]) + "\", ");
            }
            metadata.append(" \"title\": \"" + remoteFileName.toUtf8() + "\" ");
            metadata.append("}");

            // Remove used source file timestamp.
            m_sourceFileTimestampHash.remove(nonce);

            QNetworkReply *patchReply = patchFile(nonce, uid, parsedObj.property("absolutePath").toString(), metadata);
            replyBody = QString::fromUtf8(patchReply->readAll());
            if (patchReply->error() == QNetworkReply::NoError) {
                qDebug() << "GCDClient::filePutReplyResult" << nonce << "patchFile success replyBody" << replyBody;
                jsonObj = engine.evaluate("(" + replyBody + ")");
                parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
                replyBody = stringifyScriptValue(engine, parsedObj);
            } else {
                qDebug() << "GCDClient::filePutReplyResult" << nonce << "patchFile error" << patchReply->error() << patchReply->errorString() << "replyBody" << replyBody;
                replyBody = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(patchReply->error()).arg(patchReply->errorString());
            }

            // Scheduled to delete later.
            patchReply->deleteLater();
            patchReply->manager()->deleteLater();
        }
    } else {
        qDebug() << "GCDClient::filePutReplyResult" << nonce << "error" << reply->error() << reply->errorString() << "replyBody" << replyBody;
        replyBody = QString("{ \"error\": %1, \"error_string\": \"%2\" }").arg(reply->error()).arg(reply->errorString());
    }

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

void GCDClient::filePutResumeStatusReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    QByteArray replyBody = reply->readAll();
    qDebug() << "GCDClient::filePutResumeStatusReplyFinished" << reply << QString(" Error=%1").arg(reply->error()) << "replyBody" << QString::fromUtf8(replyBody);

    if (reply->error() == QNetworkReply::NoError) {
        qint64 offset = 0;
        if (reply->hasRawHeader("Range")) {
            QStringList ranges = QString::fromAscii(reply->rawHeader("Range")).split("-");
            offset = ranges.at(1).toUInt() + 1;
            qDebug() << "GCDClient::filePutResumeStatusReplyFinished ranges" << ranges << "offset" << offset;
        }

        // Emit signal with offset to resume upload.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString("{ \"offset\": %1 }").arg(offset) );
    } else {
        // REMARK Use QString::fromUtf8() to support unicode text.
        emit filePutResumeReplySignal(nonce, reply->error(), reply->errorString(), QString::fromUtf8(replyBody));
    }

    // Scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString GCDClient::deltaReplyFinished(QNetworkReply *reply)
{
    qDebug() << "GCDClient::deltaReplyFinished" << reply << QString(" Error=%1").arg(reply->error());

    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();

    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "GCDClient::deltaReplyFinished replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue sourceObj = engine.evaluate("(" + replyBody + ")");

        // Check reset state.
        bool syncOnReset = m_settings.value(QString("%1.%2.syncOnReset").arg(objectName()).arg(uid), QVariant(false)).toBool();
        bool isReset = !m_settings.value(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid)).isValid()  || m_settings.value(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(false)).toBool();
        if (isReset) {
            qDebug() << "GCDClient::deltaReplyFinished isReset" << isReset << "set to update state only.";
            m_settings.setValue(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(true));
        }

        // Get entries count.
        int entriesCount = sourceObj.property("items").property("length").toInteger();
        qDebug() << "GCDClient::deltaReplyFinished entriesCount" << entriesCount;

        // Process sourceObj.
        QScriptValue parsedObj = engine.newObject();
        QScriptValue childrenObj = engine.newArray();
        for (int i = 0; (!isReset || syncOnReset) && i < entriesCount; i++) {
            QScriptValue sourceChildObj = sourceObj.property("items").property(i);
            QString parentPath = sourceChildObj.property("file").property("parents").property(0).isValid() ? sourceChildObj.property("file").property("parents").property(0).property("id").toString() : "";

            QScriptValue parsedChildObj = engine.newObject();
            parsedChildObj.setProperty("changeId", sourceChildObj.property("id"));
            parsedChildObj.setProperty("isDeleted", sourceChildObj.property("deleted"));
            parsedChildObj.setProperty("absolutePath", sourceChildObj.property("fileId"));
            parsedChildObj.setProperty("parentPath", QScriptValue(parentPath));
            // Suppress as it's not used in CDM.deltaReplyFilter().
//            if (!sourceChildObj.property("deleted").toBool()) {
//                parsedChildObj.setProperty("property", parseCommonPropertyScriptValue(engine, sourceChildObj.property("file")));
//            }
            childrenObj.setProperty(i, parsedChildObj);
        }
        QScriptValue lastChangeId = (entriesCount > 0) ? sourceObj.property("items").property(entriesCount-1).property("id") : sourceObj.property("largestChangeId");
        // Skip to last if it's in reset state and doesn't opt to process.
        if (isReset && !syncOnReset) {
            lastChangeId = sourceObj.property("largestChangeId");
        }
        parsedObj.setProperty("children", childrenObj);
        parsedObj.setProperty("nextDeltaCursor", QScriptValue(lastChangeId.toInteger() + 1));
        parsedObj.setProperty("latestDeltaCursor", sourceObj.property("largestChangeId"));
        parsedObj.setProperty("reset", QScriptValue(isReset));

        // Check hasMore and reset state.
        bool hasMore = (lastChangeId.toInteger() < sourceObj.property("largestChangeId").toInteger());
        parsedObj.setProperty("hasMore", QScriptValue(hasMore));
        if (isReset) {
            if (hasMore) {
                qDebug() << "GCDClient::deltaReplyFinished hasMore" << hasMore << "proceed update state only.";
            } else {
                qDebug() << "GCDClient::deltaReplyFinished hasMore" << hasMore << "reset to normal state.";
                m_settings.setValue(QString("%1.%2.reset").arg(objectName()).arg(uid), QVariant(false));
            }
        }

        // Save nextDeltaCursor
        m_settings.setValue(QString("%1.%2.nextDeltaCursor").arg(objectName()).arg(uid), QVariant(parsedObj.property("nextDeltaCursor").toInteger()));

//        qDebug() << "GCDClient::deltaReplyFinished parsedObj" << stringifyScriptValue(engine, parsedObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    emit deltaReplySignal(nonce, reply->error(), reply->errorString(), replyBody);

    // scheduled to delete later.
    reply->deleteLater();
    reply->manager()->deleteLater();

    return replyBody;
}

int GCDClient::compareFileMetadata(CloudDriveJob &job, QScriptValue &jsonObj, QString localFilePath, CloudDriveItem &item)
{
    QFileInfo localFileInfo(localFilePath);
    QDateTime jsonObjLastModified = parseJSONDateString(jsonObj.property("lastModified").toString());
    int result = 0;

    // If ((rev is newer and size is changed) or there is no local file), get from remote.
    if (job.forceGet
            || (jsonObj.property("hash").toString() > item.hash && jsonObj.property("size").toInt32() != localFileInfo.size())
            || !localFileInfo.isFile()) {
        // Download changed remote item to localFilePath.
        result = -1;
    } else if (job.forcePut
               || (jsonObj.property("hash").toString() < item.hash && jsonObj.property("size").toInt32() != localFileInfo.size())
               || (jsonObjLastModified < localFileInfo.lastModified() && jsonObj.property("size").toInt32() != localFileInfo.size())
               ) {
        // ISSUE Once downloaded a file, its local timestamp will be after remote immediately. This approach may not work.
        // Upload changed local item to remoteParentPath with item name.
        result = 1;
    } else {
        // Update remote has to local hash on cloudDriveItem.
    }

    return result;
}
