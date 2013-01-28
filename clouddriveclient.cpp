#include "clouddriveclient.h"

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString CloudDriveClient::KeyStoreFilePath = "/home/user/.filesplus/%1.dat";
#else
const QString CloudDriveClient::KeyStoreFilePath = "C:/%1.dat";
#endif

CloudDriveClient::CloudDriveClient(QObject *parent) :
    QObject(parent)
{
}

CloudDriveClient::~CloudDriveClient()
{
}

bool CloudDriveClient::isAuthorized()
{
    return (!accessTokenPairMap.isEmpty());
}

QStringList CloudDriveClient::getStoredUidList()
{
    QStringList list;
    foreach (QString s, accessTokenPairMap.keys()) {
        TokenPair t = accessTokenPairMap[s];

        QString jsonText = "{ ";
        jsonText.append( QString("\"uid\": \"%1\", ").arg(s) );
        jsonText.append( QString("\"email\": \"%1\", ").arg(t.email) );
        jsonText.append( QString("\"token\": \"%1\", ").arg(t.token) );
        jsonText.append( QString("\"secret\": \"%1\", ").arg(t.secret) );
        jsonText.append( QString("\"type\": \"%1\"").arg(objectName()) );
        jsonText.append(" }");

        list.append(jsonText);
    }
    return list;
}

int CloudDriveClient::removeUid(QString uid)
{
    qDebug() << QString(objectName()) << "::removeUid uid" << uid;
    int n = accessTokenPairMap.remove(uid);

    saveAccessPairMap();

    return n;
}

bool CloudDriveClient::isRemoteAbsolutePath()
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

void CloudDriveClient::loadAccessPairMap() {
    qDebug() << QTime::currentTime() << objectName() << "::loadAccessPairMap";

    QFile file(KeyStoreFilePath.arg(objectName()));
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> accessTokenPairMap;

        qDebug() << QTime::currentTime() << objectName() << "::loadAccessPairMap" << accessTokenPairMap;
    }
}

void CloudDriveClient::saveAccessPairMap() {
    // TODO workaround fix to remove tokenPair with key="".
    accessTokenPairMap.remove("");

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
    }    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << accessTokenPairMap;

        qDebug() << objectName() << "::saveAccessPairMap " << accessTokenPairMap;
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

bool CloudDriveClient::testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname)
{
    return false;
}

void CloudDriveClient::saveConnection(QString id, QString hostname, QString username, QString password, QString token)
{
}

void CloudDriveClient::requestToken(QString nonce)
{
    emit requestTokenReplySignal(nonce, -1, objectName() + " " + tr("Request Token"), tr("Service is not implemented."));
}

void CloudDriveClient::authorize(QString nonce, QString hostname)
{
}

void CloudDriveClient::accessToken(QString nonce, QString pin)
{
    emit accessTokenReplySignal(nonce, -1, objectName() + " " + tr("Access Token"), tr("Service is not implemented."));
}

void CloudDriveClient::refreshToken(QString nonce, QString uid)
{
    emit accessTokenReplySignal(nonce, -1, objectName() + " " + tr("Refresh Token"), tr("Service is not implemented."));
}

void CloudDriveClient::accountInfo(QString nonce, QString uid)
{
    emit accountInfoReplySignal(nonce, -1, objectName() + " " + tr("Account Info"), tr("Service is not implemented."));
}

void CloudDriveClient::quota(QString nonce, QString uid)
{
    emit quotaReplySignal(nonce, -1, objectName() + " " + tr("Quota"), tr("Service is not implemented."));
}

QString CloudDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous)
{
    emit fileGetReplySignal(nonce, -1, objectName() + " " + tr("File Get"), tr("Service is not implemented."));
    return "";
}

QIODevice *CloudDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset, bool synchronous)
{
    emit fileGetReplySignal(nonce, -1, objectName() + " " + tr("File Get"), tr("Service is not implemented."));
    return 0;
}

void CloudDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName)
{
    emit filePutReplySignal(nonce, -1, objectName() + " " + tr("File Put"), tr("Service is not implemented."));
}

QNetworkReply *CloudDriveClient::filePut(QString nonce, QString uid, QIODevice *source, qint64 bytesTotal,  QString remoteParentPath, QString remoteFileName, bool synchronous)
{
    emit filePutReplySignal(nonce, -1, objectName() + " " + tr("File Put"), tr("Service is not implemented."));
    return 0;
}

QIODevice *CloudDriveClient::fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset)
{
    emit fileGetResumeReplySignal(nonce, -1, objectName() + " " + tr("File Get Resume"), tr("Service is not implemented."));
    return 0;
}

QNetworkReply *CloudDriveClient::filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + tr("File Put Resume"), tr("Service is not implemented."));
    return 0;
}

QString CloudDriveClient::filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + tr("File Put Resume"), tr("Service is not implemented."));
    return "";
}

QString CloudDriveClient::filePutResumeUpload(QString nonce, QString uid, QIODevice *source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + tr("File Put Resume"), tr("Service is not implemented."));
    return "";
}

QString CloudDriveClient::filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous)
{
    emit filePutResumeReplySignal(nonce, -1, objectName() + " " + tr("File Put Resume"), tr("Service is not implemented."));
    return "";
}

QString CloudDriveClient::filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous)
{
    emit filePutCommitReplySignal(nonce, -1, objectName() + " " + tr("File Put Commit"), tr("Service is not implemented."));
    return "";
}

QString CloudDriveClient::thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format, QString size)
{
    return "";
}

void CloudDriveClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
    emit metadataReplySignal(nonce, -1, objectName() + " " + tr("Metadata"), tr("Service is not implemented."));
}

void CloudDriveClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
    emit browseReplySignal(nonce, -1, objectName() + " " + tr("Browse"), tr("Service is not implemented."));
}

void CloudDriveClient::createFolder(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFolderName)
{
    emit createFolderReplySignal(nonce, -1, objectName() + " " + tr("Create Folder"), tr("Service is not implemented."));
}

void CloudDriveClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit moveFileReplySignal(nonce, -1, objectName() + " " + tr("Move"), tr("Service is not implemented."));
}

void CloudDriveClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
    emit copyFileReplySignal(nonce, -1, objectName() + " " + tr("Copy"), tr("Service is not implemented."));
}

void CloudDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
    emit deleteFileReplySignal(nonce, -1, objectName() + " " + tr("Delete"), tr("Service is not implemented."));
}

void CloudDriveClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
    emit shareFileReplySignal(nonce, -1, objectName() + " " + tr("Share Link"), tr("Service is not implemented."));
}

QString CloudDriveClient::delta(QString nonce, QString uid, bool synchronous)
{
    emit deltaReplySignal(nonce, -1, objectName() + " " + tr("Delta"), tr("Service is not implemented."));
    return "";
}

QString CloudDriveClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
    emit createFolderReplySignal(nonce, -1, objectName() + " " + tr("Create Folder"), tr("Service is not implemented."));
    return "";
}
