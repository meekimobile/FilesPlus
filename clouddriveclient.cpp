#include "clouddriveclient.h"
#include <QDesktopServices>

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString CloudDriveClient::KeyStoreFilePath = "/home/user/.filesplus/%1.dat";
const int CloudDriveClient::FileWriteBufferSize = 32768;
#else
const QString CloudDriveClient::KeyStoreFilePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/%1.dat";
const int CloudDriveClient::FileWriteBufferSize = 32768;
#endif

CloudDriveClient::CloudDriveClient(QObject *parent) :
    QObject(parent)
{
    // Set object name for further reference.
    setObjectName(this->metaObject()->className());

    m_replyHash = new QHash<QString, QNetworkReply*>();
}

CloudDriveClient::~CloudDriveClient()
{
    m_replyHash = 0;
}

bool CloudDriveClient::isAuthorized()
{
    return (!accessTokenPairMap.isEmpty());
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
    return "";
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
    return engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << jsonObj).toString();
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
    emit fileGetReplySignal(nonce, -1, objectName() + " " + "File Get", "Service is not implemented.");
    return "";
}

QIODevice *CloudDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    emit fileGetReplySignal(nonce, -1, objectName() + " " + "File Get", "Service is not implemented.");
    return 0;
}

void CloudDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName)
{
    emit filePutReplySignal(nonce, -1, objectName() + " " + "File Put", "Service is not implemented.");
}

QNetworkReply *CloudDriveClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal,  QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    emit filePutReplySignal(nonce, -1, objectName() + " " + "File Put", "Service is not implemented.");
    return 0;
}

QIODevice *CloudDriveClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    emit fileGetResumeReplySignal(nonce, -1, objectName() + " " + "File Get Resume", "Service is not implemented.");
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
    if (m_replyHash->contains(nonce)) {
        QNetworkReply *reply = m_replyHash->value(nonce);
        reply->abort();
        m_replyHash->remove(nonce);
        qDebug() << "CloudDriveClient::abort nonce" << nonce << "is aborted.";
        qDebug() << "CloudDriveClient::abort m_replyHash count" << m_replyHash->count();
    } else {
        qDebug() << "CloudDriveClient::abort nonce" << nonce << "is not found. Operation is ignored.";
    }
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

void CloudDriveClient::createFolder(QString nonce, QString uid, QString newRemoteParentPath, QString newRemoteFolderName)
{
    emit createFolderReplySignal(nonce, -1, objectName() + " " + "Create folder", "Service is not implemented.");
}

void CloudDriveClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    emit moveFileReplySignal(nonce, -1, objectName() + " " + "Move", "Service is not implemented.");
}

void CloudDriveClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    emit copyFileReplySignal(nonce, -1, objectName() + " " + "Copy", "Service is not implemented.");
}

void CloudDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    emit deleteFileReplySignal(nonce, -1, objectName() + " " + "Delete", "Service is not implemented.");
}

void CloudDriveClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    emit shareFileReplySignal(nonce, -1, objectName() + " " + "Share link", "Service is not implemented.", "", 0);
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
