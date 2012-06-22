#include "clouddrivemodelthread.h"

CloudDriveModelThread::CloudDriveModelThread(QObject *parent) :
    QThread(parent)
{
//    initializeDropboxClient();
//    initializeGCDClient();
}

QString CloudDriveModelThread::getHashFilePath() const
{
    return m_hashFilePath;
}

void CloudDriveModelThread::setHashFilePath(const QString hashFilePath)
{
    m_hashFilePath = hashFilePath;
}

QMultiMap<QString, CloudDriveItem> CloudDriveModelThread::getCloudDriveItems()
{
    return m_cloudDriveItems;
}

//bool CloudDriveModelThread::isAuthorized()
//{
//    return dbClient->isAuthorized() || gcdClient->isAuthorized();
//}

//bool CloudDriveModelThread::isAuthorized(CloudDriveModelThread::ClientTypes type)
//{
//    switch (type) {
//    case Dropbox:
//        return dbClient->isAuthorized();
//    case GoogleDrive:
//        return gcdClient->isAuthorized();
//    }

//    return false;
//}

//QStringList CloudDriveModelThread::getStoredUidList(CloudDriveModelThread::ClientTypes type)
//{
//    switch (type) {
//    case Dropbox:
//        return dbClient->getStoredUidList();
//    case GoogleDrive:
//        return gcdClient->getStoredUidList();
//    }

//    return QStringList();
//}

void CloudDriveModelThread::setNonce(QString nonce)
{
    m_nonce = nonce;
}

//void CloudDriveModelThread::setClientType(ClientTypes clientType)
//{
//    m_clientType = clientType;
//}

//void CloudDriveModelThread::setUid(QString uid)
//{
//    m_uid = uid;
//}

//void CloudDriveModelThread::setLocalFilePath(QString localFilePath)
//{
//    m_localFilePath = localFilePath;
//}

//void CloudDriveModelThread::setRemoteFilePath(QString remoteFilePath)
//{
//    m_remoteFilePath = remoteFilePath;
//}

void CloudDriveModelThread::setRunMethod(int method)
{
    m_runMethod = method;
}

//void CloudDriveModelThread::initializeDropboxClient() {
//    dbClient = new DropboxClient();
//    connect(dbClient, SIGNAL(uploadProgress(QString,qint64,qint64)), SIGNAL(uploadProgress(QString,qint64,qint64)) );
//    connect(dbClient, SIGNAL(downloadProgress(QString,qint64,qint64)), SIGNAL(downloadProgress(QString,qint64,qint64)) );
//    connect(dbClient, SIGNAL(requestTokenReplySignal(int,QString,QString)), SIGNAL(requestTokenReplySignal(int,QString,QString)) );
//    connect(dbClient, SIGNAL(authorizeRedirectSignal(QString,QString)), SIGNAL(authorizeRedirectSignal(QString,QString)) );
//    connect(dbClient, SIGNAL(accessTokenReplySignal(int,QString,QString)), SIGNAL(accessTokenReplySignal(int,QString,QString)) );
//    connect(dbClient, SIGNAL(accountInfoReplySignal(int,QString,QString)), SIGNAL(accountInfoReplySignal(int,QString,QString)) );
//    connect(dbClient, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SIGNAL(fileGetReplySignal(QString,int,QString,QString)) );
//    connect(dbClient, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SIGNAL(filePutReplySignal(QString,int,QString,QString)) );
//    connect(dbClient, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SIGNAL(metadataReplySignal(QString,int,QString,QString)) );
//    connect(dbClient, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SIGNAL(createFolderReplySignal(QString,int,QString,QString)) );
//}

//void CloudDriveModelThread::initializeGCDClient() {
//    connect(gcdClient, SIGNAL(uploadProgress(qint64,qint64)), SIGNAL(uploadProgress(qint64,qint64)) );
//    connect(gcdClient, SIGNAL(downloadProgress(qint64,qint64)), SIGNAL(downloadProgress(qint64,qint64)) );
//    connect(gcdClient, SIGNAL(authorizeRedirectSignal(QString,QString)), SIGNAL(authorizeRedirectSignal(QString,QString)) );
//    connect(gcdClient, SIGNAL(accessTokenReplySignal(int,QString,QString)), SIGNAL(accessTokenReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(accountInfoReplySignal(int,QString,QString)), SIGNAL(accountInfoReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(fileGetReplySignal(int,QString,QString)), SIGNAL(fileGetReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(filePutReplySignal(int,QString,QString)), SIGNAL(filePutReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(metadataReplySignal(int,QString,QString)), SIGNAL(metadataReplySignal(int,QString,QString)) );
//}

void CloudDriveModelThread::run()
{
    qDebug() << "CloudDriveModelThread::run m_runMethod" << m_runMethod;

    switch (m_runMethod) {
//    case FileGet:
//        fileGet(m_clientType, m_uid, m_remoteFilePath, m_localFilePath);
//        break;
//    case FilePut:
//        filePut(m_clientType, m_uid, m_localFilePath, m_remoteFilePath);
//        break;
//    case Metadata:
//        metadata(m_clientType, m_uid, m_remoteFilePath);
//        break;
//    case CreateFolder:
//        createFolder(m_clientType, m_uid, m_localFilePath, m_remoteFilePath);
//        break;
    case LoadCloudDriveItems:
        loadCloudDriveItems();
        break;
    }
}

//void CloudDriveModelThread::requestToken(CloudDriveModelThread::ClientTypes type)
//{
//    switch (type) {
//    case Dropbox:
//        dbClient->requestToken();
//        break;
//    }
//}

//void CloudDriveModelThread::authorize(CloudDriveModelThread::ClientTypes type)
//{
//    switch (type) {
//    case Dropbox:
//        dbClient->authorize();
//        break;
//    }
//}

//void CloudDriveModelThread::accessToken(CloudDriveModelThread::ClientTypes type)
//{
//    switch (type) {
//    case Dropbox:
//        dbClient->accessToken();
//        break;
//    }
//}

//void CloudDriveModelThread::accountInfo(CloudDriveModelThread::ClientTypes type, QString uid)
//{
//    switch (type) {
//    case Dropbox:
//        dbClient->accountInfo(uid);
//        break;
//    }
//}

//void CloudDriveModelThread::fileGet(CloudDriveModelThread::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath) {
//    switch (type) {
//    case Dropbox:
//        dbClient->fileGet(m_nonce, uid, remoteFilePath, localFilePath);
//        break;
//    }
//}

//void CloudDriveModelThread::filePut(CloudDriveModelThread::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath) {
//    switch (type) {
//    case Dropbox:
//        dbClient->filePut(m_nonce, uid, localFilePath, remoteFilePath);
//        break;
//    }
//}

//void CloudDriveModelThread::metadata(CloudDriveModelThread::ClientTypes type, QString uid, QString remoteFilePath) {
//    switch (type) {
//    case Dropbox:
//        dbClient->metadata(m_nonce, uid, remoteFilePath);
//        break;
//    }
//}

//void CloudDriveModelThread::createFolder(CloudDriveModelThread::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath) {
//    switch (type) {
//    case Dropbox:
//        dbClient->createFolder(m_nonce, uid, localFilePath, remoteFilePath);
//        break;
//    }
//}

void CloudDriveModelThread::loadCloudDriveItems() {
    QFile file(m_hashFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> m_cloudDriveItems;
    }

    qDebug() << QTime::currentTime() << "CloudDriveModel::loadCloudDriveItems " << m_cloudDriveItems.size();

    emit loadCloudDriveItemsFinished(m_nonce);
}
