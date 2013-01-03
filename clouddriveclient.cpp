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
    qDebug() << objectName() << "::removeUid accessTokenPairMap" << accessTokenPairMap;

    return n;
}

bool CloudDriveClient::isRemoteAbsolutePath()
{
    return false;
}

QString CloudDriveClient::getRemoteRoot()
{
    return "";
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

bool CloudDriveClient::testConnection(QString hostname, quint16 port, QString username, QString password)
{
    return false;
}

void CloudDriveClient::saveConnection(QString id, QString hostname, quint16 port, QString username, QString password)
{
}

void CloudDriveClient::requestToken(QString nonce)
{
    emit requestTokenReplySignal(nonce, -1, objectName() + " " + tr("Request Token"), tr("Service is not implemented."));
}

void CloudDriveClient::authorize(QString nonce)
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

void CloudDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath)
{
    emit fileGetReplySignal(nonce, -1, objectName() + " " + tr("File Get"), tr("Service is not implemented."));
}

void CloudDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
    emit filePutReplySignal(nonce, -1, objectName() + " " + tr("File Put"), tr("Service is not implemented."));
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

QNetworkReply *CloudDriveClient::createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous)
{
}
