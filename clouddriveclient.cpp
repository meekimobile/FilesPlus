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
}

void CloudDriveClient::authorize(QString nonce)
{
}

void CloudDriveClient::accessToken(QString nonce, QString pin)
{
}

void CloudDriveClient::refreshToken(QString nonce, QString uid)
{
}

void CloudDriveClient::accountInfo(QString nonce, QString uid)
{
}

void CloudDriveClient::quota(QString nonce, QString uid)
{
}

void CloudDriveClient::fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath)
{
}

void CloudDriveClient::filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
}

void CloudDriveClient::metadata(QString nonce, QString uid, QString remoteFilePath)
{
}

void CloudDriveClient::browse(QString nonce, QString uid, QString remoteFilePath)
{
}

void CloudDriveClient::createFolder(QString nonce, QString uid, QString localFilePath, QString remoteFilePath)
{
}

void CloudDriveClient::moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
}

void CloudDriveClient::copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName)
{
}

void CloudDriveClient::deleteFile(QString nonce, QString uid, QString remoteFilePath)
{
}

void CloudDriveClient::shareFile(QString nonce, QString uid, QString remoteFilePath)
{
}
