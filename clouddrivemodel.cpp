#include "clouddrivemodel.h"
#include <QFile>

const QString CloudDriveModel::HashFilePath = "C:/CloudDriveModel.dat";

CloudDriveModel::CloudDriveModel(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    loadCloudDriveItems();

    // Initialize cloud drive clients.
    initializeDropboxClient();
}

CloudDriveModel::~CloudDriveModel()
{
    saveCloudDriveItems();
}

void CloudDriveModel::loadCloudDriveItems() {
    QFile file(HashFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> m_cloudDriveItems;

        qDebug() << "CloudDriveModel::loadCloudDriveItems " << m_cloudDriveItems;
    }
}

void CloudDriveModel::saveCloudDriveItems() {
    QFile file(HashFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << m_cloudDriveItems;
    }
}

void CloudDriveModel::initializeDropboxClient() {
    dbClient = new DropboxClient();
    connect(dbClient, SIGNAL(uploadProgress(qint64,qint64)), SIGNAL(uploadProgress(qint64,qint64)) );
    connect(dbClient, SIGNAL(downloadProgress(qint64,qint64)), SIGNAL(downloadProgress(qint64,qint64)) );
    connect(dbClient, SIGNAL(requestTokenReplySignal(int,QString,QString)), SIGNAL(requestTokenReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(authorizeRedirectSignal(QString)), SIGNAL(authorizeRedirectSignal(QString)) );
    connect(dbClient, SIGNAL(accessTokenReplySignal(int,QString,QString)), SIGNAL(accessTokenReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(accountInfoReplySignal(int,QString,QString)), SIGNAL(accountInfoReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(fileGetReplySignal(int,QString,QString)), SIGNAL(fileGetReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(filePutReplySignal(int,QString,QString)), SIGNAL(filePutReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(metadataReplySignal(int,QString,QString)), SIGNAL(metadataReplySignal(int,QString,QString)) );
}

CloudDriveItem CloudDriveModel::getItem(QString localPath, ClientTypes type, QString uid)
{
//    qDebug() << "CloudDriveModel::getItem " << localPath << ", " << type << ", " << uid;
    CloudDriveItem item;
    foreach (item, getItemList(localPath)) {
        if (item.type == type && item.uid == uid) {
//            qDebug() << "CloudDriveModel::getItem " << item;
            return item;
        }
    }

    return CloudDriveItem();
}

QList<CloudDriveItem> CloudDriveModel::getItemList(QString localPath) {
    return m_cloudDriveItems.values(localPath);
}

bool CloudDriveModel::isConnected(QString localPath)
{
    return m_cloudDriveItems.contains(localPath);
}

void CloudDriveModel::addItem(QString localPath, CloudDriveItem item)
{
    m_cloudDriveItems.insert(localPath, item);

    qDebug() << "CloudDriveModel::addItem m_cloudDriveItems " << m_cloudDriveItems;
}

void CloudDriveModel::addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath != "") {
        qDebug() << "CloudDriveModel::addItem remove " << item;
        removeItem(type, uid, localPath);
    }
    item = CloudDriveItem(type, uid, localPath, remotePath, hash);
    addItem(localPath, item);
}

void CloudDriveModel::removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item =  getItem(localPath, type, uid);
    m_cloudDriveItems.remove(localPath, item);
}

QString CloudDriveModel::getItemListJson(QString localPath)
{
    QList<CloudDriveItem> list = getItemList(localPath);
    qDebug() << "CloudDriveModel::getItemListJson getItemList " << list;

    QString jsonText;
    foreach (CloudDriveItem item, list) {
        if (jsonText != "") jsonText.append(", ");
        jsonText.append( QString("{ \"type\": \"%1\", \"uid\": \"%2\", \"hash\": \"%3\" }").arg(item.type).arg(item.uid).arg(item.hash) );
    }

    return "[ " + jsonText + " ]";
}

QString CloudDriveModel::getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return item.remotePath;
}

QString CloudDriveModel::getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return item.hash;
}

QString CloudDriveModel::getDefaultLocalFilePath(const QString &remoteFilePath)
{
    QRegExp rx("^([C-F])(/.+)$");
    rx.indexIn(remoteFilePath);
    if (rx.captureCount() == 2) {
        return rx.cap(1).append(":").append(rx.cap(2));
    }
    return "";
}

QString CloudDriveModel::getDefaultRemoteFilePath(const QString &localFilePath)
{
    QRegExp rx("^([C-F])(:)(/.+)$");
    rx.indexIn(localFilePath);
    if (rx.captureCount() == 3) {
        return rx.cap(1).append(rx.cap(3));
    }
    return "";
}

bool CloudDriveModel::isAuthorized()
{
    // TODO check if any cloud drive is authorized.
    return dbClient->isAuthorized();
}

bool CloudDriveModel::isAuthorized(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        return dbClient->isAuthorized();
    }

    return false;
}

QStringList CloudDriveModel::getStoredUidList(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        return dbClient->getStoredUidList();
    }

    return QStringList();
}

void CloudDriveModel::requestToken(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        dbClient->requestToken();
    }
}

void CloudDriveModel::authorize(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        dbClient->authorize();
    }
}

void CloudDriveModel::accessToken(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        dbClient->accessToken();
    }
}

void CloudDriveModel::accountInfo(CloudDriveModel::ClientTypes type, QString uid)
{
    switch (type) {
    case Dropbox:
        dbClient->accountInfo(uid);
    }
}

void CloudDriveModel::fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath)
{
    switch (type) {
    case Dropbox:
        dbClient->fileGet(uid, remoteFilePath, localFilePath);
    }
}

void CloudDriveModel::filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath)
{
    switch (type) {
    case Dropbox:
        dbClient->filePut(uid, localFilePath, remoteFilePath);
    }
}

void CloudDriveModel::metadata(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath)
{
    switch (type) {
    case Dropbox:
        dbClient->metadata(uid, remoteFilePath);
    }
}
