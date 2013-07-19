#include "clouddriveclient.h"
#include <QDesktopServices>
#include <QScriptValueIterator>
#include <QApplication>
#include "qnetworkreplywrapper.h"
#include "sleeper.h"

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString CloudDriveClient::KeyStoreFilePath = "/home/user/.filesplus/%1.dat";
const int CloudDriveClient::FileWriteBufferSize = 32768;
#else
const QString CloudDriveClient::KeyStoreFilePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/%1.dat";
const int CloudDriveClient::FileWriteBufferSize = 32768;
#endif
const qint64 CloudDriveClient::DefaultChunkSize = 4194304; // 4MB
const qint64 CloudDriveClient::DefaultMaxUploadSize = 20971520; // 20MB which support ~20 sec(s) video clip.

CloudDriveClient::CloudDriveClient(QObject *parent) :
    QObject(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    m_replyHash = new QHash<QString, QNetworkReply*>();
    m_propertyReplyHash = new QHash<QString, QByteArray>();
    m_filesReplyHash = new QHash<QString, QByteArray>();
}

CloudDriveClient::~CloudDriveClient()
{
    m_replyHash = 0;
    m_propertyReplyHash = 0;
    m_filesReplyHash = 0;
}

bool CloudDriveClient::isAuthorized()
{
    return (!accessTokenPairMap.isEmpty());
}

bool CloudDriveClient::isAuthorized(QString uid)
{
    return accessTokenPairMap.contains(uid);
}

QStringList CloudDriveClient::getStoredUidList()
{
    QStringList list;
    foreach (QString s, accessTokenPairMap.keys()) {
        list.append(getStoredUid(s));
    }
    return list;
}

QString CloudDriveClient::getStoredUid(QString uid)
{
    if (!accessTokenPairMap.contains(uid)) {
        return "";
    }

    TokenPair t = accessTokenPairMap[uid];

    QString jsonText = "{ ";
    jsonText.append( QString("\"uid\": \"%1\", ").arg(uid) );
    jsonText.append( QString("\"email\": \"%1\", ").arg(t.email) );
    jsonText.append( QString("\"token\": \"%1\", ").arg(t.token) );
    jsonText.append( QString("\"secret\": \"%1\", ").arg(t.secret) );
    jsonText.append( QString("\"type\": \"%1\"").arg(objectName()) );
    jsonText.append(" }");

    return jsonText;
}

QString CloudDriveClient::getEmail(QString uid)
{
    if (!accessTokenPairMap.contains(uid)) {
        return "";
    }

    TokenPair t = accessTokenPairMap[uid];

    return t.email;
}

int CloudDriveClient::removeUid(QString uid)
{
    if (!accessTokenPairMap.contains(uid)) {
        return -1;
    }

    qDebug() << QString(objectName()) << "::removeUid uid" << uid;
    int n = accessTokenPairMap.remove(uid);

    saveAccessPairMap();

    return n;
}

QDateTime CloudDriveClient::parseReplyDateString(QString dateString)
{
    return QDateTime::fromString(dateString, Qt::ISODate);
}

bool CloudDriveClient::isRemoteAbsolutePath()
{
    return false;
}

bool CloudDriveClient::isRemotePathCaseInsensitive()
{
    return false;
}

QString CloudDriveClient::getRemoteRoot(QString id)
{
    return RemoteRoot;
}

bool CloudDriveClient::isFilePutResumable(qint64 fileSize)
{
    return false;
}

bool CloudDriveClient::isFileGetResumable(qint64 fileSize)
{
    return false;
}

bool CloudDriveClient::isDeltaSupported()
{
    return false;
}

bool CloudDriveClient::isDeltaEnabled(QString uid)
{
    return false;
}

bool CloudDriveClient::isConfigurable()
{
    return false;
}

bool CloudDriveClient::isViewable()
{
    return false;
}

bool CloudDriveClient::isImageUrlCachable()
{
    return false;
}

bool CloudDriveClient::isUnicodeSupported()
{
    return true;
}

bool CloudDriveClient::isMediaEnabled(QString uid)
{
    return true;
}

bool CloudDriveClient::isFileGetRedirected()
{
    return false;
}

void CloudDriveClient::setMediaEnabled(QString uid, bool flag)
{
    m_settings.setValue(QString("%1.%2.media.enabled").arg(objectName()).arg(uid), QVariant(flag));
}

qint64 CloudDriveClient::getChunkSize()
{
    return m_settings.value(QString("%1.resumable.chunksize").arg(objectName()), DefaultChunkSize).toInt();
}

void CloudDriveClient::loadAccessPairMap() {
//    qDebug() << QTime::currentTime() << objectName() << "::loadAccessPairMap";

    QFile file(KeyStoreFilePath.arg(objectName()));
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> accessTokenPairMap;

        qDebug() << QTime::currentTime() << objectName() << "::loadAccessPairMap" << accessTokenPairMap;
    }
}

void CloudDriveClient::saveAccessPairMap() {
    // TODO workaround fix to remove tokenPair with key="" or tokenPair without both email and token.
    accessTokenPairMap.remove("");
    foreach (QString uid, accessTokenPairMap.keys()) {
        if (accessTokenPairMap[uid].email == "" && accessTokenPairMap[uid].token == "") {
            accessTokenPairMap.remove(uid);
        }
    }

    // TODO To prevent invalid code to save damage data for testing only.
//    if (accessTokenPairMap.isEmpty()) return;

    QFile file(KeyStoreFilePath.arg(objectName()));
    QFileInfo info(file);
    if (!info.absoluteDir().exists()) {
        qDebug() << objectName() << "::saveAccessPairMap dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(info.absolutePath());
        if (!res) {
            qDebug() << objectName() << "::saveAccessPairMap can't make dir" << info.absolutePath();
        } else {
            qDebug() << objectName() << "::saveAccessPairMap make dir" << info.absolutePath();
        }
    }

    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << accessTokenPairMap;

        qDebug() << objectName() << "::saveAccessPairMap" << accessTokenPairMap;
    }
}

QString CloudDriveClient::createNonce() {
    QString ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString nonce;

    for(int i = 0; i <= 16; ++i)
    {
        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
    }

    return nonce;
}

QString CloudDriveClient::createTimestamp() {
    qint64 seconds = QDateTime::currentMSecsSinceEpoch() / 1000;

    return QString("%1").arg(seconds);
}

QString CloudDriveClient::createNormalizedQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(QUrl::toPercentEncoding(key)).append("=").append(QUrl::toPercentEncoding(sortMap[key]));
    }

    return queryString;
}

QString CloudDriveClient::encodeURI(const QString uri) {
    // Example: https://api.dropbox.com/1/metadata/sandbox/C/B/NES/Solomon's Key (E) [!].nes
    // All non-alphanumeric except : and / must be encoded.
    return QUrl::toPercentEncoding(uri, ":/");
}

QString CloudDriveClient::createQueryString(QMap<QString, QString> sortMap) {
    QString queryString;
    foreach (QString key, sortMap.keys()) {
        if (queryString != "") queryString.append("&");
        queryString.append(key).append("=").append(sortMap[key]);
    }

    return queryString;
}

QByteArray CloudDriveClient::encodeMultiPart(QString boundary, QMap<QString, QString> paramMap, QString fileParameter, QString fileName, QByteArray fileData, QString contentType) {
    //Encodes list of parameters and files for HTTP multipart format.
    QByteArray postData;
    QString CRLF = "\r\n";

    foreach (QString key, paramMap.keys()) {
        postData.append("--" + boundary).append(CRLF);
        postData.append(QString("Content-Disposition: form-data; name=\"%1\"").arg(key)).append(CRLF);
        postData.append(CRLF);
        postData.append(paramMap[key]).append(CRLF);
    }

    postData.append("--" + boundary).append(CRLF);
    postData.append(QString("Content-Disposition: form-data; name=\"%1\"; filename=\"%2\"").arg(fileParameter).arg(fileName) ).append(CRLF);
    postData.append(QString("Content-Type: %1").arg(contentType) ).append(CRLF);
    postData.append(CRLF);
    postData.append(fileData).append(CRLF);
    postData.append("--" + boundary + "--").append(CRLF);
    postData.append(CRLF);

    return postData;
}

QString CloudDriveClient::removeDoubleSlash(QString remoteFilePath)
{
    QString path = remoteFilePath;

    // Replace double slash.
    path = path.replace(QRegExp("/{2,}"), "/");

    return path;
}

QString CloudDriveClient::getFileType(QString localPath)
{
    int i = localPath.lastIndexOf(".");
    QString fileType;
    if (i > -1 && i < localPath.length()) {
        fileType = localPath.mid(i + 1);
    } else {
        fileType = "";
    }

//    qDebug() << "CloudDriveClient::getFileType" << localPath << "fileType" << fileType;
    return fileType;
}

QString CloudDriveClient::getContentType(QString fileName) {
    QString contentType = ContentTypeHelper::getContentType(fileName);
    if (contentType == "") {
        contentType = "application/octet-stream";
    }

    return contentType;
}

QScriptValue CloudDriveClient::parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj)
{
    // NOTE Date string must be in JSON format.
    return QScriptValue();
}

QString CloudDriveClient::stringifyScriptValue(QScriptEngine &engine, QScriptValue &jsonObj)
{
    // ISSUE It's very slow while running on Symbian with 500+ items. And also on Meego with 100+ items.
    if (m_settings.value("CloudDriveClient.stringifyScriptValue.useCustomLogic", false).toBool()) {
        QString jsonText;
        QString jsonArrayText;
        QScriptValueIterator it(jsonObj);
        while (it.hasNext()) {
            QApplication::processEvents();

            it.next();

            QScriptValue propertyValue = it.value();

            if (jsonText != "") {
                jsonText.append(", ");
            }

            if(propertyValue.isArray()) {
                jsonArrayText = "";
                for (int i=0; i<propertyValue.property("length").toInt32(); i++) {
                    QApplication::processEvents();

                    if (jsonArrayText != "") {
                        jsonArrayText.append(", ");
                    }
                    QScriptValue v = propertyValue.property(i);
                    jsonArrayText.append(stringifyScriptValue(engine, v));
                }
                jsonText.append(QString("\"%1\": [%2]").arg(it.name()).arg(jsonArrayText));
            } else if (propertyValue.isObject()) {
                jsonText.append(QString("\"%1\": %2").arg(it.name()).arg(stringifyScriptValue(engine, propertyValue)));
            } else if (propertyValue.isDate()) {
                jsonText.append(QString("\"%1\": \"%2\"").arg(it.name()).arg(formatJSONDateString(propertyValue.toDateTime())));
            } else if (propertyValue.isBool()) {
                jsonText.append(QString("\"%1\": %2").arg(it.name()).arg(propertyValue.toBool()));
            } else if (propertyValue.isNumber()) {
                jsonText.append(QString("\"%1\": %2").arg(it.name()).arg(propertyValue.toNumber()));
            } else {
                // Default as string.
                jsonText.append(QString("\"%1\": \"%2\"").arg(it.name()).arg(propertyValue.toString()));
            }
        }

        return "{ " + jsonText + " }";
    } else {
        return engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << jsonObj).toString();
    }
}

QString CloudDriveClient::formatJSONDateString(QDateTime datetime)
{
    // Format to JSON which match with javascript, QML format.
    // NOTE It's RFC 3339 formatted timestamp
    return datetime.toUTC().toString("yyyy-MM-ddThh:mm:ss.zzzZ");
}

bool CloudDriveClient::testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname)
{
    return false;
}

bool CloudDriveClient::saveConnection(QString id, QString hostname, QString username, QString password, QString token)
{
    return false;
}

qint64 CloudDriveClient::getOffsetFromRange(QString rangeHeader)
{
    qint64 offset = 0;
    if (!rangeHeader.isEmpty()) {
        QStringList tokens = rangeHeader.split(QRegExp("=|-"));
        qDebug() << "CloudDriveClient::getOffsetFromRange range" << rangeHeader << "tokens" << tokens;
        offset = tokens.at(1).toInt();
    }

    return offset;
}

QString CloudDriveClient::getPathFromUrl(QString urlString)
{
    if (urlString.startsWith("http")) {
        // Use RegExp to parse path from url string. To support # in path.
        QRegExp rx("^(http|https|ftp)://([^/]+)(/.*)");
        rx.indexIn(urlString);
        qDebug() << "CloudDriveClient::getPathFromUrl" << urlString << "capturedTexts" << rx.capturedTexts();
        if (rx.captureCount() >= 3) {
            return rx.cap(3);
        } else {
            return "";
        }
    } else {
        return urlString;
    }
}

QDateTime CloudDriveClient::parseJSONDateString(QString jsonString)
{
    return QDateTime::fromString(jsonString, Qt::ISODate);
}

CloudDriveModelItem CloudDriveClient::parseCloudDriveModelItem(QScriptEngine &engine, QScriptValue jsonObj)
{
    CloudDriveModelItem modelItem;
    modelItem.name = jsonObj.property("name").toString();
    modelItem.absolutePath = jsonObj.property("absolutePath").toString();
    modelItem.parentPath = jsonObj.property("parentPath").toString();
    modelItem.size = jsonObj.property("size").toInteger();
    modelItem.lastModified = parseJSONDateString(jsonObj.property("lastModified").toString());
    modelItem.isDir = jsonObj.property("isDir").toBool();
    modelItem.hash = jsonObj.property("hash").toString();
    modelItem.fileType = jsonObj.property("fileType").toString();
    modelItem.isDeleted = jsonObj.property("isDeleted").toBool();
    modelItem.isHidden = jsonObj.property("isHidden").toBool();
    modelItem.isReadOnly = jsonObj.property("isReadOnly").toBool();
    modelItem.source = jsonObj.property("source").toString();
    modelItem.alternative = jsonObj.property("alternative").toString();
    modelItem.thumbnail = jsonObj.property("thumbnail").toString();
    modelItem.thumbnail128 = jsonObj.property("thumbnail128").toString();
    modelItem.preview = jsonObj.property("preview").toString();
    modelItem.downloadUrl = jsonObj.property("downloadUrl").toString();
    modelItem.webContentLink = jsonObj.property("webContentLink").toString();
    modelItem.embedLink = jsonObj.property("embedLink").toString();

    return modelItem;
}

void CloudDriveClient::requestToken(QString nonce)
{
    emit requestTokenReplySignal(nonce, -1, objectName() + " " + "Request Token", "Service is not implemented.");
}

void CloudDriveClient::authorize(QString nonce, QString hostname)
{
    emit authorizeRedirectSignal(nonce, hostname, "");
}

void CloudDriveClient::accessToken(QString nonce, QString pin)
{
    emit accessTokenReplySignal(nonce, -1, objectName() + " " + "Access Token", "Service is not implemented.");
}

void CloudDriveClient::refreshToken(QString nonce, QString uid)
{
    emit accessTokenReplySignal(nonce, -1, objectName() + " " + "Refresh Token", "Service is not implemented.");
}

void CloudDriveClient::accountInfo(QString nonce, QString uid)
{
    emit accountInfoReplySignal(nonce, -1, objectName() + " " + "Account Info", "Service is not implemented.");
}

void CloudDriveClient::quota(QString nonce, QString uid)
{
    emit quotaReplySignal(nonce, -1, objectName() + " " + "Quota", "Service is not implemented.", 0, 0, -1);
}

QString CloudDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    qDebug() << "----- CloudDriveClient::fileGet -----" << objectName() << nonce << uid << remoteFilePath << localFilePath << synchronous;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);
    qint64 offset = (m_localFileHash[nonce]->exists()) ? m_localFileHash[nonce]->size() : -1;

    // Send request.
    QNetworkReply *reply = dynamic_cast<QNetworkReply *>( fileGet(nonce, uid, remoteFilePath, offset, synchronous) );

    if (!synchronous) {
        return "";
    }

    // Construct result.
    QString result = fileGetReplyFinished(reply, synchronous);

    return result;
}

QIODevice *CloudDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    qDebug() << "----- CloudDriveClient::fileGet -----" << objectName() << nonce << uid << remoteFilePath << offset << "synchronous" << synchronous;

    QString uri = fileGetURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "CloudDriveClient::fileGet" << objectName() << nonce << "uri" << uri;

    // Get redirected request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", createAuthHeader(uid));
    if (offset >= 0) {
        QString rangeHeader = isFileGetResumable()
                ? QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1)
                : QString("bytes=%1-").arg(offset);
        qDebug() << "CloudDriveClient::fileGet" << objectName() << nonce << "rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    req.setRawHeader("Accept-Encoding", "gzip, deflate");
    QNetworkReply *reply = manager->get(req);
    reply = getRedirectedReply(reply);

    if (!synchronous) {
        // Wait until readyRead().
        QEventLoop loop;
        connect(reply, SIGNAL(readyRead()), &loop, SLOT(quit()));
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
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

QIODevice *CloudDriveClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    qDebug() << "----- CloudDriveClient::fileGetResume -----" << objectName() << nonce << uid << remoteFilePath << "to" << localFilePath << "offset" << offset;

    QString uri = fileGetURI.arg(remoteFilePath);
    uri = encodeURI(uri);
    qDebug() << "CloudDriveClient::fileGetResume" << objectName() << nonce << "uri" << uri;

    // Create localTargetFile for file getting.
    m_localFileHash[nonce] = new QFile(localFilePath);

    // Get redirected request.
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest req = QNetworkRequest(QUrl::fromEncoded(uri.toAscii()));
    req.setAttribute(QNetworkRequest::User, QVariant(nonce));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1), QVariant(uid));
    req.setAttribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2), QVariant(remoteFilePath));
    req.setRawHeader("Authorization", createAuthHeader(uid));
    if (offset >= 0) {
        QString rangeHeader = isFileGetResumable()
                ? QString("bytes=%1-%2").arg(offset).arg(offset+getChunkSize()-1)
                : QString("bytes=%1-").arg(offset);
        qDebug() << "CloudDriveClient::fileGetResume" << objectName() << nonce << "rangeHeader" << rangeHeader;
        req.setRawHeader("Range", rangeHeader.toAscii() );
    }
    req.setRawHeader("Accept-Encoding", "gzip, deflate");
    QNetworkReply *reply = manager->get(req);
    reply = getRedirectedReply(reply);
    connect(reply->manager(), SIGNAL(finished(QNetworkReply*)), this, SLOT(fileGetResumeReplyFinished(QNetworkReply*)));

    return reply;
}

void CloudDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName)
{
    qDebug() << "----- CloudDriveClient::filePut -----" << objectName() << nonce << uid << localFilePath << remoteParentPath << remoteFileName;

    m_localFileHash[nonce] = new QFile(localFilePath);
    QFile *localSourceFile = m_localFileHash[nonce];
    if (localSourceFile->open(QIODevice::ReadOnly)) {
        qint64 fileSize = localSourceFile->size();

        // Send request.
        filePut(nonce, uid, localSourceFile, fileSize, remoteParentPath, remoteFileName, false);
    } else {
        qDebug() << "CloudDriveClient::filePut" << objectName() << nonce << "file" << localFilePath << "can't be opened.";
        emit filePutReplySignal(nonce, -1, "Can't open file", localFilePath + " can't be opened.");
    }
}

QNetworkReply *CloudDriveClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal,  QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    emit filePutReplySignal(nonce, -1, objectName() + " " + "File Put", "Service is not implemented.");
    return 0;
}

QNetworkReply *CloudDriveClient::filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + "File Put Resume", "Service is not implemented.");
    return 0;
}

QString CloudDriveClient::filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + "File Put Resume", "Service is not implemented.");
    return "";
}

QString CloudDriveClient::filePutResumeUpload(QString nonce, QString uid, QIODevice *source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + "File Put Resume", "Service is not implemented.");
    return "";
}

QString CloudDriveClient::filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + "File Put Resume", "Service is not implemented.");
    return "";
}

QString CloudDriveClient::filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous)
{
    emit filePutCommitReplySignal(nonce, -1, objectName() + " " + "File Put Commit", "Service is not implemented.");
    return "";
}

bool CloudDriveClient::abort(QString nonce)
{
    bool res = false;
    if (m_replyHash->contains(nonce)) {
        QNetworkReply *reply = m_replyHash->value(nonce);
        if (!reply->isFinished()) {
            reply->abort();
            res = true;
            qDebug() << "CloudDriveClient::abort nonce" << nonce << "is aborted.";
        }
        m_replyHash->remove(nonce);
        qDebug() << "CloudDriveClient::abort m_replyHash count" << m_replyHash->count();
    } else {
        qDebug() << "CloudDriveClient::abort nonce" << nonce << "is not found. Operation is ignored.";
    }

    return res;
}

QNetworkReply *CloudDriveClient::files(QString nonce, QString uid, QString remoteFilePath, int offset, bool synchronous, QString callback)
{
    return 0;
}

QNetworkReply *CloudDriveClient::property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous, QString callback)
{
    return 0;
}

QString CloudDriveClient::thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format, QString size)
{
    return "";
}

QString CloudDriveClient::media(QString nonce, QString uid, QString remoteFilePath)
{
    return "";
}

void CloudDriveClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    emit metadataReplySignal(nonce, -1, objectName() + " " + "Metadata", "Service is not implemented.");
}

void CloudDriveClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    emit browseReplySignal(nonce, -1, objectName() + " " + "Browse", "Service is not implemented.");
}

void CloudDriveClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    emit moveFileReplySignal(nonce, -1, objectName() + " " + "Move", "Service is not implemented.");
}

void CloudDriveClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    emit copyFileReplySignal(nonce, -1, objectName() + " " + "Copy", "Service is not implemented.");
}

QString CloudDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    emit deleteFileReplySignal(nonce, -1, objectName() + " " + "Delete", "Service is not implemented.");
    return "";
}

QString CloudDriveClient::shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous)
{
    emit shareFileReplySignal(nonce, -1, objectName() + " " + "Share link", "Service is not implemented.", "", 0);
    return "";
}

QString CloudDriveClient::delta(QString nonce, QString uid, bool synchronous)
{
    // Emit empty message as default.
    if (!synchronous) {
        emit deltaReplySignal(nonce, 0, objectName() + " " + "Delta", "{ }");
    }
    return "";
}

QString CloudDriveClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    emit createFolderReplySignal(nonce, -1, objectName() + " " + "Create folder", "Service is not implemented.");
    return "";
}

qint64 CloudDriveClient::writeToFile(QIODevice *source, QString targetFilePath, qint64 offset)
{
    qDebug() << "CloudDriveClient::writeToFile" << source << targetFilePath << offset;

    // Stream replyBody to a file on localPath.
    qint64 totalBytes = source->bytesAvailable();
    qint64 writtenBytes = 0;
    char buf[FileWriteBufferSize];
    QFile *localTargetFile = new QFile(targetFilePath);
    if (localTargetFile->open(QIODevice::ReadWrite)) {
        // Issue: Writing to file with QDataStream << QByteArray will automatically prepend with 4-bytes prefix(size).
        // Solution: Use QIODevice to write directly.

        // Move to offset.
        localTargetFile->seek(offset);

        // Read first buffer.
        qint64 c = source->read(buf, sizeof(buf));
        while (c > 0) {
            qint64 bytes = localTargetFile->write(buf, c);
            if (bytes < 0) {
                qDebug() << "CloudDriveClient::writeToFile can't write file" << targetFilePath << " Operation is aborted.";
                // Close target file.
                localTargetFile->flush();
                localTargetFile->close();
                return -2;
            }

            writtenBytes += bytes;

            // Tell event loop to process event before it will process time consuming task.
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

            // Read next buffer.
            c = source->read(buf, sizeof(buf));
        }
        qDebug() << "CloudDriveClient::writeToFile done writtenBytes" << writtenBytes << "totalBytes" << totalBytes << "localTargetFile size" << localTargetFile->size();

        // Close target file.
        localTargetFile->flush();
        localTargetFile->close();

        return writtenBytes;
    } else {
        qDebug() << "CloudDriveClient::writeToFile can't open file" << targetFilePath << " Operation is aborted.";
        // Close target file.
        localTargetFile->flush();
        localTargetFile->close();
        return -1;
    }

}

void CloudDriveClient::uploadProgressFilter(qint64 bytesSent, qint64 bytesTotal)
{
    QString nonce = dynamic_cast<QNetworkReply *>(sender())->request().attribute(QNetworkRequest::User).toString();

//    qDebug() << "CloudDriveClient::uploadProgressFilter" << sender() << nonce << bytesSent << bytesTotal;

    emit uploadProgress(nonce, bytesSent, bytesTotal);
}

void CloudDriveClient::downloadProgressFilter(qint64 bytesReceived, qint64 bytesTotal)
{
    QString nonce = dynamic_cast<QNetworkReply *>(sender())->request().attribute(QNetworkRequest::User).toString();

//    qDebug() << "CloudDriveClient::downloadProgressFilter" << sender() << nonce << bytesReceived << bytesTotal;

    emit downloadProgress(nonce, bytesReceived, bytesTotal);
}

QString CloudDriveClient::fileGetReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    QString result;
    if (reply->error() == QNetworkReply::NoError) {
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
    } else {
        qDebug() << "CloudDriveClient::fileGetReplyResult" << objectName() << nonce << reply->error() << reply->errorString() << QString::fromUtf8(reply->readAll());
        result = QString("{ \"error\": %1, \"error_string\": \"%2\", \"http_status_code\": %3 }")
                .arg(reply->error())
                .arg(reply->errorString())
                .arg(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt());
    }

    return result;
}

QString CloudDriveClient::fileGetReplyFinished(QNetworkReply *reply, bool synchronous)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qDebug() << "CloudDriveClient::fileGetReplyFinished" << objectName() << nonce << reply << QString("Error=%1").arg(reply->error());

    // Stream to file.
    QFile *localTargetFile = m_localFileHash[nonce];
    qint64 writtenBytes = fileGetReplySaveStream(reply, localTargetFile);
    qDebug() << "CloudDriveClient::fileGetReplyFinished" << objectName() << nonce << "writtenBytes" << writtenBytes;

    // Return common json.
    QString result = fileGetReplyResult(reply);

    if (!synchronous) emit fileGetReplySignal(nonce, reply->error(), reply->errorString(), result);

    // Scheduled to delete later.
    m_propertyReplyHash->remove(nonce);
    m_localFileHash.remove(nonce);
    localTargetFile->deleteLater();
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();

    return result;
}

void CloudDriveClient::fileGetResumeReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qDebug() << "CloudDriveClient::fileGetResumeReplyFinished" << objectName() << nonce << reply << QString("Error=%1").arg(reply->error());

    // Stream to file.
    QFile *localTargetFile = m_localFileHash[nonce];
    qint64 writtenBytes = fileGetReplySaveChunk(reply, localTargetFile);
    qDebug() << "CloudDriveClient::fileGetResumeReplyFinished" << objectName() << nonce << "writtenBytes" << writtenBytes;

    // Return common json.
    QString result = fileGetReplyResult(reply);

    emit fileGetResumeReplySignal(nonce, reply->error(), reply->errorString(), result);

    // Scheduled to delete later.
    m_propertyReplyHash->remove(nonce);
    m_localFileHash.remove(nonce);
    localTargetFile->deleteLater();
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();
}

QString CloudDriveClient::filePutReplyResult(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString replyBody = QString::fromUtf8(reply->readAll());
    qDebug() << "CloudDriveClient::filePutReplyResult" << objectName() << nonce << "replyBody" << replyBody;
    if (reply->error() == QNetworkReply::NoError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + replyBody + ")");
        QScriptValue parsedObj = parseCommonPropertyScriptValue(engine, jsonObj);
        replyBody = stringifyScriptValue(engine, parsedObj);
    }

    return replyBody;
}

void CloudDriveClient::filePutReplyFinished(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "CloudDriveClient::filePutReplyFinished" << objectName() << nonce << reply << QString("Error=%1").arg(reply->error());

    // Close source file.
    if (m_localFileHash.contains(nonce)) {
        QFile *localTargetFile = m_localFileHash[nonce];
        localTargetFile->close();
        m_localFileHash.remove(nonce);
    }

    QString result = filePutReplyResult(reply);

    emit filePutReplySignal(nonce, reply->error(), reply->errorString(), result);

    // Remove request buffer.
    if (m_bufferHash.contains(nonce)) {
        m_bufferHash[nonce]->deleteLater();
        m_bufferHash.remove(nonce);
    }

    // Scheduled to delete later.
    m_replyHash->remove(nonce);
    reply->deleteLater();
    reply->manager()->deleteLater();
}

int CloudDriveClient::compareDirMetadata(CloudDriveJob &job, QScriptValue &jsonObj, QString localFilePath, CloudDriveItem &item)
{
    int result = 0;

    QDir localDir(localFilePath);
    localDir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    int itemCount = localDir.entryList().count();
    // If (hash is different), get from remote.
    if (job.forceGet
            || (jsonObj.property("hash").toString() > item.hash)
            || (jsonObj.property("children").property("length").toInteger() != itemCount)) {
        // Proceed getting metadata.
        result = -1;
    } else if (job.forcePut
               || (jsonObj.property("hash").toString() < item.hash)
               || (jsonObj.property("children").property("length").toInteger() != itemCount)) {
        // Proceed getting metadata.
        result = 1;
    } else {
        // Update remote has to local hash on cloudDriveItem.
    }

    return result;
}

int CloudDriveClient::compareFileMetadata(CloudDriveJob &job, QScriptValue &jsonObj, QString localFilePath, CloudDriveItem &item)
{
    QFileInfo localFileInfo(localFilePath);
    int result = 0;

    // If ((rev is newer) or there is no local file), get from remote.
    if (job.forceGet
            || (jsonObj.property("hash").toString() > item.hash)
            || !localFileInfo.isFile()) {
        // Download changed remote item to localFilePath.
        result = -1;
    } else if (job.forcePut
               || (jsonObj.property("hash").toString() < item.hash)) {
        // ISSUE Once downloaded a file, its local timestamp will be after remote immediately. This approach may not work.
        // Upload changed local item to remoteParentPath with item name.
        result = 1;
    } else {
        // Update remote has to local hash on cloudDriveItem.
    }

    return result;
}

QNetworkReply * CloudDriveClient::getRedirectedReply(QNetworkReply *reply)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();

    if (isFileGetRedirected()) {
        // Wait until finished().
        while (!reply->isFinished()) {
            QApplication::processEvents(QEventLoop::AllEvents, 100);
            Sleeper::msleep(100);
        }

        QVariant possibleRedirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if(!possibleRedirectUrl.toUrl().isEmpty()) {
            qDebug() << "CloudDriveClient::getRedirectedReply" << nonce << "redirectUrl" << possibleRedirectUrl.toUrl();

            QNetworkRequest redirectedRequest = reply->request();
            redirectedRequest.setUrl(possibleRedirectUrl.toUrl());
            redirectedRequest.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, QVariant(true));

            // Delete existing reply.
            m_replyHash->remove(nonce);
            reply->deleteLater();

            reply = reply->manager()->get(redirectedRequest);
        }
    }
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressFilter(qint64,qint64)));
    QNetworkReplyWrapper *w = new QNetworkReplyWrapper(reply);

    // Store reply for further usage.
    m_replyHash->insert(nonce, reply);

    return reply;
}

qint64 CloudDriveClient::fileGetReplySaveChunk(QNetworkReply *reply, QFile *localTargetFile)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qint64 totalBytes = reply->bytesAvailable();
    qint64 writtenBytes = 0;
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "CloudDriveClient::fileGetReplySaveChunk" << objectName() << nonce << "reply bytesAvailable" << reply->bytesAvailable();

        // Find offset.
        qint64 offset = getOffsetFromRange(QString::fromAscii(reply->request().rawHeader("Range")));
        qDebug() << "CloudDriveClient::fileGetReplySaveChunk" << objectName() << nonce << "reply request offset" << offset;

        // Stream replyBody to a file on localPath.
        char buf[FileWriteBufferSize];
        QFile *localTargetFile = m_localFileHash[nonce];
        if (localTargetFile->open(QIODevice::ReadWrite)) {
            // Issue: Writing to file with QDataStream << QByteArray will automatically prepend with 4-bytes prefix(size).
            // Solution: Use QIODevice to write directly.

            // Move to offset.
            localTargetFile->seek(offset);

            // Read first buffer.
            qint64 c = reply->read(buf, sizeof(buf));
            while (c > 0) {
//                qDebug() << "CloudDriveClient::fileGetReplySaveChunk" << objectName() << nonce << "reply readBytes" << c;

                writtenBytes += localTargetFile->write(buf, c);
//                qDebug() << "CloudDriveClient::fileGetReplySaveChunk" << objectName() << nonce << "reply writtenBytes" << writtenBytes;

                // Tell event loop to process event before it will process time consuming task.
                QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

                // Read next buffer.
                c = reply->read(buf, sizeof(buf));
            }
        }

        qDebug() << "CloudDriveClient::fileGetReplySaveChunk" << objectName() << nonce << "reply writtenBytes" << writtenBytes << "totalBytes" << totalBytes << "localTargetFile size" << localTargetFile->size();

        // Close target file.
        localTargetFile->flush();
        localTargetFile->close();
    }

    return writtenBytes;
}

qint64 CloudDriveClient::fileGetReplySaveStream(QNetworkReply *reply, QFile *localTargetFile)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    QString uid = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 1)).toString();
    QString remoteFilePath = reply->request().attribute(QNetworkRequest::Attribute(QNetworkRequest::User + 2)).toString();

    qint64 totalBytes = reply->request().header(QNetworkRequest::ContentLengthHeader).toInt();
    qint64 writtenBytes = 0;
    if (reply->error() == QNetworkReply::NoError) {
        // Find offset.
        qint64 offset = getOffsetFromRange(QString::fromAscii(reply->request().rawHeader("Range")));
        qDebug() << "CloudDriveClient::fileGetReplySaveStream" << objectName() << nonce << "reply request offset" << offset;

        // Stream replyBody to a file on localPath.
        char buf[FileWriteBufferSize];
        if (localTargetFile->open(QIODevice::ReadWrite)) {
            // Move to offset.
            localTargetFile->seek(offset);

            while (!reply->isFinished() || reply->bytesAvailable() > 0) {
                if (reply->bytesAvailable() == 0) {
                    QApplication::processEvents();
                    continue;
                }

                qint64 c = reply->read(buf, sizeof(buf));
                while (c > 0) {
//                    qDebug() << "CloudDriveClient::fileGetReplySaveStream" << objectName() << nonce << "reply readBytes" << c;

                    writtenBytes += localTargetFile->write(buf, c);
//                    qDebug() << "CloudDriveClient::fileGetReplySaveStream" << objectName() << nonce << "reply writtenBytes" << writtenBytes;

                    // Tell event loop to process event before it will process time consuming task.
                    QApplication::processEvents();

                    // Read next buffer.
                    c = reply->read(buf, sizeof(buf));
                }
            }
        }

        qDebug() << "CloudDriveClient::fileGetReplySaveStream" << objectName() << nonce << "reply writtenBytes" << writtenBytes << "totalBytes" << totalBytes << "localTargetFile size" << localTargetFile->size();

        // Close target file.
        localTargetFile->flush();
        localTargetFile->close();
    }

    return writtenBytes;
}

QByteArray CloudDriveClient::createAuthHeader(QString uid)
{
    return QString("Bearer " + accessTokenPairMap[uid].token).toAscii();
}
