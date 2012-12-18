#include "clouddrivemodel.h"

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString CloudDriveModel::ITEM_DAT_PATH = "/home/user/.filesplus/CloudDriveModel.dat";
const QString CloudDriveModel::ITEM_DB_PATH = "/home/user/.filesplus/CloudDriveModel.db";
const QString CloudDriveModel::ITEM_DB_CONNECTION_NAME = "cloud_drive_model";
const int CloudDriveModel::MaxRunningJobCount = 3;
#else
const QString CloudDriveModel::ITEM_DAT_PATH = "C:/CloudDriveModel.dat";
const QString CloudDriveModel::ITEM_DB_PATH = "CloudDriveModel.db";
const QString CloudDriveModel::ITEM_DB_CONNECTION_NAME = "cloud_drive_model";
const int CloudDriveModel::MaxRunningJobCount = 2;
#endif
const QString CloudDriveModel::DirtyHash = "FFFFFFFF";
//const QStringList CloudDriveModel::restrictFileTypes = QString("SIS,SISX,DEB").split(",");
const QStringList CloudDriveModel::restrictFileTypes;

CloudDriveModel::CloudDriveModel(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    // Issue: Qt on Belle doesn't support QtConcurrent and QFuture.
    // Example code: QFuture<void> loadDataFuture = QtConcurrent::run(this, &CloudDriveModel::loadCloudDriveItems);

    // Connect job queue chain.
    connect(this, SIGNAL(proceedNextJobSignal()), SLOT(proceedNextJob()) );

    // Connect with permanent thread.
    connect(&m_thread, SIGNAL(finished()), this, SLOT(threadFinishedFilter()) );
    connect(&m_thread, SIGNAL(terminated()), this, SLOT(threadFinishedFilter()) );
    connect(&m_thread, SIGNAL(loadCloudDriveItemsFinished(QString)), this, SLOT(loadCloudDriveItemsFilter(QString)) );
//    connect(&m_thread, SIGNAL(uploadProgress(QString,qint64,qint64)), SLOT(uploadProgressFilter(QString,qint64,qint64)) );
//    connect(&m_thread, SIGNAL(downloadProgress(QString,qint64,qint64)), SLOT(downloadProgressFilter(QString,qint64,qint64)) );
//    connect(&m_thread, SIGNAL(requestTokenReplySignal(int,QString,QString)), SIGNAL(requestTokenReplySignal(int,QString,QString)) );
//    connect(&m_thread, SIGNAL(authorizeRedirectSignal(QString,QString)), SIGNAL(authorizeRedirectSignal(QString,QString)) );
//    connect(&m_thread, SIGNAL(accessTokenReplySignal(int,QString,QString)), SIGNAL(accessTokenReplySignal(int,QString,QString)) );
//    connect(&m_thread, SIGNAL(accountInfoReplySignal(int,QString,QString)), SIGNAL(accountInfoReplySignal(int,QString,QString)) );
//    connect(&m_thread, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SLOT(fileGetReplyFilter(QString,int,QString,QString)) );
//    connect(&m_thread, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SLOT(filePutReplyFilter(QString,int,QString,QString)) );
//    connect(&m_thread, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SLOT(metadataReplyFilter(QString,int,QString,QString)) );
//    connect(&m_thread, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SLOT(createFolderReplyFilter(QString,int,QString,QString)) );

    // Fetch first nonce and trash it.
    createNonce();

    // Initialize DAT and jobQueue.
    m_cloudDriveItems = new QMultiMap<QString, CloudDriveItem>();
    m_cloudDriveJobs = new QHash<QString, CloudDriveJob>();
    m_jobQueue = new QQueue<QString>();

    // Initialize scheduler queue.
    initScheduler();
    m_scheduledItems = new QQueue<CloudDriveItem>();

    // Enqueue initialization jobs. Queued jobs will proceed after foldePage is loaded.
    CloudDriveJob loadCloudDriveItemsJob(createNonce(), LoadCloudDriveItems, -1, "", "", "", -1);
    m_cloudDriveJobs->insert(loadCloudDriveItemsJob.jobId, loadCloudDriveItemsJob);
    m_jobQueue->enqueue(loadCloudDriveItemsJob.jobId);
    CloudDriveJob initializeDBJob(createNonce(), InitializeDB, -1, "", "", "", -1);
    m_cloudDriveJobs->insert(initializeDBJob.jobId, initializeDBJob);
    m_jobQueue->enqueue(initializeDBJob.jobId);

    // Initialize itemCache.
    m_itemCache = new QHash<QString, CloudDriveItem>();
    m_isConnectedCache = new QHash<QString, bool>();
    m_isDirtyCache = new QHash<QString, bool>();
    m_isSyncingCache = new QHash<QString, bool>();

    // Initialize cloud storage clients.
    initializeDropboxClient();
    initializeSkyDriveClient();
    initializeFtpClient();
}

CloudDriveModel::~CloudDriveModel()
{
    // TODO Migrate DAT to DB.
    saveCloudDriveItems();

    // Close DB.
    cleanDB();
    closeDB();
}

QString CloudDriveModel::dirtyHash() const
{
    return DirtyHash;
}

bool CloudDriveModel::getDropboxFullAccess()
{
    return m_dropboxFullAccess;
}

void CloudDriveModel::setDropboxFullAccess(bool flag)
{
    if (m_dropboxFullAccess != flag) {
        m_dropboxFullAccess = flag;

        // Re-initialize DropboxClient.
        initializeDropboxClient();
    }
}

void CloudDriveModel::saveCloudDriveItems() {
    // Prevent save for testing only.
//    if (m_cloudDriveItems.isEmpty()) return;

    // Cleanup before save.
    cleanItems();

    QFile file(ITEM_DAT_PATH);
    QFileInfo info(file);
    if (!info.absoluteDir().exists()) {
        qDebug() << "CloudDriveModel::saveCloudDriveItems dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(info.absolutePath());
        if (!res) {
            qDebug() << "CloudDriveModel::saveCloudDriveItems can't make dir" << info.absolutePath();
        } else {
            qDebug() << "CloudDriveModel::saveCloudDriveItems make dir" << info.absolutePath();
        }
    }
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << *m_cloudDriveItems;
    }

    qDebug() << "CloudDriveModel::saveCloudDriveItems" << m_cloudDriveItems->size();
}

void CloudDriveModel::initializeDropboxClient() {
    qDebug() << "CloudDriveModel::initializeDropboxClient" << m_dropboxFullAccess;

    if (dbClient != 0) {
        dbClient->deleteLater();
    }

    dbClient = new DropboxClient(this, m_dropboxFullAccess);
    connect(dbClient, SIGNAL(uploadProgress(QString,qint64,qint64)), SLOT(uploadProgressFilter(QString,qint64,qint64)) );
    connect(dbClient, SIGNAL(downloadProgress(QString,qint64,qint64)), SLOT(downloadProgressFilter(QString,qint64,qint64)) );
    connect(dbClient, SIGNAL(requestTokenReplySignal(QString,int,QString,QString)), SLOT(requestTokenReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(authorizeRedirectSignal(QString,QString,QString)), SLOT(authorizeRedirectFilter(QString,QString,QString)) );
    connect(dbClient, SIGNAL(accessTokenReplySignal(QString,int,QString,QString)), SLOT(accessTokenReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(accountInfoReplySignal(QString,int,QString,QString)), SLOT(accountInfoReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SLOT(fileGetReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SLOT(filePutReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SLOT(metadataReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(browseReplySignal(QString,int,QString,QString)), SLOT(browseReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SLOT(createFolderReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(moveFileReplySignal(QString,int,QString,QString)), SLOT(moveFileReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(copyFileReplySignal(QString,int,QString,QString)), SLOT(copyFileReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(deleteFileReplySignal(QString,int,QString,QString)), SLOT(deleteFileReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(shareFileReplySignal(QString,int,QString,QString)), SLOT(shareFileReplyFilter(QString,int,QString,QString)) );
}

void CloudDriveModel::initializeSkyDriveClient()
{
    qDebug() << "CloudDriveModel::initializeSkyDriveClient";

    if (skdClient != 0) {
        skdClient->deleteLater();
    }

    skdClient = new SkyDriveClient(this);
    connect(skdClient, SIGNAL(uploadProgress(QString,qint64,qint64)), SLOT(uploadProgressFilter(QString,qint64,qint64)) );
    connect(skdClient, SIGNAL(downloadProgress(QString,qint64,qint64)), SLOT(downloadProgressFilter(QString,qint64,qint64)) );
    connect(skdClient, SIGNAL(authorizeRedirectSignal(QString,QString,QString)), SLOT(authorizeRedirectFilter(QString,QString,QString)) );
    connect(skdClient, SIGNAL(accessTokenReplySignal(QString,int,QString,QString)), SLOT(accessTokenReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(accountInfoReplySignal(QString,int,QString,QString)), SLOT(accountInfoReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(quotaReplySignal(QString,int,QString,QString)), SLOT(quotaReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SLOT(fileGetReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SLOT(filePutReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SLOT(metadataReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(browseReplySignal(QString,int,QString,QString)), SLOT(browseReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SLOT(createFolderReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(moveFileReplySignal(QString,int,QString,QString)), SLOT(moveFileReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(copyFileReplySignal(QString,int,QString,QString)), SLOT(copyFileReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(deleteFileReplySignal(QString,int,QString,QString)), SLOT(deleteFileReplyFilter(QString,int,QString,QString)) );
    connect(skdClient, SIGNAL(shareFileReplySignal(QString,int,QString,QString)), SLOT(shareFileReplyFilter(QString,int,QString,QString)) );
}

//void CloudDriveModel::initializeGCDClient() {
//    gcdClient = new GCDClient(this);
////    connect(gcdClient, SIGNAL(uploadProgress(qint64,qint64)), SIGNAL(uploadProgress(qint64,qint64)) );
////    connect(gcdClient, SIGNAL(downloadProgress(qint64,qint64)), SIGNAL(downloadProgress(qint64,qint64)) );
//    connect(gcdClient, SIGNAL(authorizeRedirectSignal(QString,QString)), SIGNAL(authorizeRedirectSignal(QString,QString)) );
//    connect(gcdClient, SIGNAL(accessTokenReplySignal(int,QString,QString)), SIGNAL(accessTokenReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(accountInfoReplySignal(int,QString,QString)), SIGNAL(accountInfoReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(fileGetReplySignal(int,QString,QString)), SIGNAL(fileGetReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(filePutReplySignal(int,QString,QString)), SIGNAL(filePutReplySignal(int,QString,QString)) );
//    connect(gcdClient, SIGNAL(metadataReplySignal(int,QString,QString)), SIGNAL(metadataReplySignal(int,QString,QString)) );
//}

void CloudDriveModel::initializeFtpClient()
{
    qDebug() << "CloudDriveModel::initializeFtpClient";

    ftpClient = new FtpClient(this);
    connect(ftpClient, SIGNAL(uploadProgress(QString,qint64,qint64)), SLOT(uploadProgressFilter(QString,qint64,qint64)) );
    connect(ftpClient, SIGNAL(downloadProgress(QString,qint64,qint64)), SLOT(downloadProgressFilter(QString,qint64,qint64)) );
    connect(ftpClient, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SLOT(fileGetReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SLOT(filePutReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SLOT(metadataReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(browseReplySignal(QString,int,QString,QString)), SLOT(browseReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SLOT(createFolderReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(deleteFileReplySignal(QString,int,QString,QString)), SLOT(deleteFileReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(moveFileReplySignal(QString,int,QString,QString)), SLOT(moveFileReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(copyFileReplySignal(QString,int,QString,QString)), SLOT(copyFileReplyFilter(QString,int,QString,QString)) );
    connect(ftpClient, SIGNAL(shareFileReplySignal(QString,int,QString,QString)), SLOT(shareFileReplyFilter(QString,int,QString,QString)) );
}

QString CloudDriveModel::createNonce() {
    QString ALPHANUMERIC = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    QString nonce;

    for(int i = 0; i <= 8; ++i)
    {
        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
    }

    nonce = nonce.append(QString("%1").arg(QDateTime::currentMSecsSinceEpoch()));
//    qDebug() << "CloudDriveModel::createNonce" << nonce;

    return nonce;
}

CloudDriveItem CloudDriveModel::getItem(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
//    qDebug() << "CloudDriveModel::getItem " << localPath << ", " << type << ", " << uid;

    // Get from DB first if it's not found find from DAT.
    CloudDriveItem item = selectItemByPrimaryKeyFromDB(type, uid, localPath);

    if (item.localPath == "") {
        foreach (CloudDriveItem datItem, m_cloudDriveItems->values(localPath)) {
            if (datItem.localPath == localPath && datItem.type == type && datItem.uid == uid) {
                // TODO (Realtime Migration) Insert to DB.
                qDebug() << "CloudDriveModel::getItem migrate" << datItem;
                if (insertItemToDB(datItem) > 0) {
                    m_cloudDriveItems->remove(datItem.localPath, datItem);
                    qDebug() << "CloudDriveModel::getItem migrate m_cloudDriveItems.count()" << m_cloudDriveItems->count();
                }
                return datItem;
            }
        }
    }

    return item;
}

QList<CloudDriveItem> CloudDriveModel::findItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    QList<CloudDriveItem> list;

    // Get from DB first if it's not found find from DAT.
    // Get localPath
    list.append(selectItemByPrimaryKeyFromDB(type, uid, localPath));
    qDebug() << "CloudDriveModel::findItemWithChildren localPath" << localPath << "list.count" << list.count();
    // Get it's children
    list.append(selectChildrenByPrimaryKeyFromDB(type, uid, localPath));
    qDebug() << "CloudDriveModel::findItemWithChildren localPath" << localPath << " with children list.count" << list.count();

    // TODO Not work yet. If not found localPath, it will migrate all remains items which cause UI freezing.
    // Find from DAT.
    bool isFound = false;
    foreach (CloudDriveItem item, m_cloudDriveItems->values()) {
        qDebug() << "CloudDriveModel::findItemWithChildren" << item.localPath << item.type << item.uid;
        if (item.type == type && item.uid == uid) {
            if (item.localPath == localPath || item.localPath.startsWith(localPath + "/")) {
    //            qDebug() << "CloudDriveModel::findItemWithChildren add" << item;
                list.append(item);
                isFound = true;
            } else if (isFound) {
                // End of sub tree in same type, uid.
                break;
            }
        }

        // TODO (Realtime Migration) Insert to DB.
        if (insertItemToDB(item) > 0) {
            m_cloudDriveItems->remove(item.localPath, item);
            qDebug() << "CloudDriveModel::findItemWithChildren migrate" << item << "m_cloudDriveItems count" << m_cloudDriveItems->count();
        }
    }

    return list;
}

QList<CloudDriveItem> CloudDriveModel::findItems(CloudDriveModel::ClientTypes type, QString uid)
{
//    qDebug() << "CloudDriveModel::findItems type" << type << "uid" << uid;
    QList<CloudDriveItem> list;

    // As this request is not specified by localPath. There are some items haven't been migrated yet.
    // Needs to combine with existing in DAT.
    foreach (CloudDriveItem item, m_cloudDriveItems->values()) {
        QString key = item.localPath;
//        qDebug() << "CloudDriveModel::findItems" << key << item.type << item.uid;
        if (item.type == type && item.uid == uid) {
            qDebug() << "CloudDriveModel::findItems add" << key << item;
            list.append(item);

            // TODO (Realtime Migration) Insert to DB.
//            qDebug() << "CloudDriveModel::findItems migrate" << item;
//            insertItemToDB(item);
//            m_cloudDriveItems.remove(item.localPath, item);
//            qDebug() << "CloudDriveModel::findItems migrate m_cloudDriveItems.count()" << m_cloudDriveItems.count();
        }
    }

    // Get from DB first if it's not found find from DAT.
    list.append(selectItemsByTypeAndUidFromDB(type, uid));

    qDebug() << "CloudDriveModel::findItems list.count()" << list.count();

    return list;
}

QList<CloudDriveItem> CloudDriveModel::getItemList(QString localPath) {
    QList<CloudDriveItem> list;

    // Get from DB first if it's not found find from DAT.
    // TODO Cache result for each localPath.
    list.append(selectItemsByLocalPathFromDB(localPath));

    if (list.isEmpty()) {
        foreach (CloudDriveItem item, m_cloudDriveItems->values(localPath)) {
            list.append(item);

            // TODO (Realtime Migration) Insert to DB.
            qDebug() << "CloudDriveModel::getItemList migrate" << item;
            if (insertItemToDB(item)) {
                m_cloudDriveItems->remove(item.localPath, item);
                qDebug() << "CloudDriveModel::getItemList migrate m_cloudDriveItems count" << m_cloudDriveItems->count();
            }
        }
    }

    return list;
}

bool CloudDriveModel::isConnected(QString localPath)
{
    if (m_isConnectedCache->contains(localPath)) {
        return m_isConnectedCache->value(localPath);
    }

    // Get from DB first if it's not found find from DAT.
    int c = countItemByLocalPathDB(localPath);

    if (c <= 0) {
        // TODO Don't migrate here because isDirty will trigger migration in getItemList().
        c = m_cloudDriveItems->values(localPath).count();
    }

//    qDebug() << "CloudDriveModel::isConnected localPath" << localPath << "recordCount" << c;
    m_isConnectedCache->insert(localPath, (c > 0));
    return (c > 0);
}

bool CloudDriveModel::isConnected(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return (item.localPath == localPath);
}

bool CloudDriveModel::isRemotePathConnected(CloudDriveModel::ClientTypes type, QString uid, QString remotePath, bool showDebug)
{
    // Show connections.
    if (showDebug) {
        QSqlQuery qry(m_db);
        qry.prepare("SELECT * FROM cloud_drive_item where type = :type AND uid = :uid AND remote_path = :remote_path");
        qry.bindValue(":type", type);
        qry.bindValue(":uid", uid);
        qry.bindValue(":remote_path", remotePath);
        bool res = qry.exec();
        if (res) {
            while (qry.next()) {
                QString localPath = qry.value(qry.record().indexOf("local_path")).toString();
                qDebug() << "CloudDriveModel::isRemotePathConnected" << type << uid << remotePath << "localPath" << localPath;
            }
        }
    }

    int c = countItemByTypeAndUidAndRemotePathFromDB(type, uid, remotePath);
//    qDebug() << "CloudDriveModel::isRemotePathConnected" << type << uid << remotePath << c;
    return (c > 0);
}

bool CloudDriveModel::isDirty(QString localPath, QDateTime lastModified)
{
    if (m_isDirtyCache->contains(localPath)) {
        return m_isDirtyCache->value(localPath);
    }

    bool res = false;
    // Get from DB first if it's not found find from DAT.
    foreach (CloudDriveItem item, getItemList(localPath)) {
        if (item.hash == DirtyHash) {
//                qDebug() << "CloudDriveModel::isDirty item" << item.localPath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
            res = true;
        } else if (lastModified > item.lastModified) {
            // It's also dirty if file/folder 's lastModified is newer. (It's changed.)
            // Signal will be emitted onyl once, next time it will be catched by item.hash==DirtyHash block above.
            qDebug() << "CloudDriveModel::isDirty item" << item.localPath << "type" << item.type << "uid" << item.uid << "lastModified" << item.lastModified << "file.lastModified" << lastModified;

            // TODO Emit signal to update item's hash.
            emit localChangedSignal(localPath);

            res = true;
        }

    }

//    qDebug() << "CloudDriveModel::isDirty localPath" << localPath << res;
    m_isDirtyCache->insert(localPath, res);
    return res;
}

bool CloudDriveModel::isSyncing(QString localPath)
{
    if (m_isSyncingCache->contains(localPath)) {
        return m_isSyncingCache->value(localPath);
    }

    bool res = false;
    foreach (CloudDriveJob job, m_cloudDriveJobs->values()) {
        if (job.localFilePath == localPath) {
            res = true;
        }
    }

//    qDebug() << "CloudDriveModel::isSyncing localPath" << localPath << res;
    m_isSyncingCache->insert(localPath, res);
    return res;
}

bool CloudDriveModel::isParentConnected(QString localPath)
{
    // TODO
    QString parentPath = getParentLocalPath(localPath);
    if (parentPath != "") {
        // Return isConnected if parentPath is valid.
        return isConnected(parentPath);
    }

    return false;
}

QString CloudDriveModel::getParentLocalPath(QString localPath)
{
    QString parentPath = "";
    if (localPath != "") {
        parentPath = localPath.mid(0, localPath.lastIndexOf("/"));
    }

    return parentPath;
}

CloudDriveModel::ClientTypes CloudDriveModel::getJobType(int jobType)
{
    switch (jobType) {
    case Dropbox:
        return Dropbox;
    case SkyDrive:
        return SkyDrive;
    case Ftp:
        return Ftp;
    }
}

QString CloudDriveModel::getParentRemotePath(QString remotePath)
{
    QString remoteParentPath = "";
    if (remotePath != "") {
        remoteParentPath = remotePath.mid(0, remotePath.lastIndexOf("/"));
    }

    return remoteParentPath;
}

void CloudDriveModel::initScheduler()
{
    connect(&m_schedulerTimer, SIGNAL(timeout()), this, SLOT(schedulerTimeoutFilter()) );
    m_schedulerTimer.setInterval(60000);
    m_schedulerTimer.start();
}

QString CloudDriveModel::getFileType(QString localPath)
{
    int i = localPath.lastIndexOf(".");
    QString fileType;
    if (i > -1 && i < localPath.length()) {
        fileType = localPath.mid(i + 1);
    } else {
        fileType = "";
    }

//    qDebug() << "CloudDriveModel::getFileType" << localPath << "fileType" << fileType;
    return fileType;
}

bool CloudDriveModel::canSync(QString localPath)
{
    if (localPath == "") return false;

    QString fileType = getFileType(localPath);
    if (fileType != "") {
        if (restrictFileTypes.contains(fileType, Qt::CaseInsensitive)) {
            return false;
        }
    }

    QString remotePath = getDefaultRemoteFilePath(localPath);
//    qDebug() << "CloudDriveModel::canSync valid" << (remotePath != "") << "localPath" << localPath;

    return remotePath != "";
}

QString CloudDriveModel::getFirstJobJson(QString localPath)
{
    foreach (CloudDriveJob job, m_cloudDriveJobs->values()) {
        if (job.localFilePath == localPath) {
            QString jsonText = getJobJson(job.jobId);

            return jsonText;
        }
    }

    return "";
}

QString CloudDriveModel::getJobJson(QString jobId)
{
    CloudDriveJob job = m_cloudDriveJobs->value(jobId);

    return job.toJsonText();
}

void CloudDriveModel::removeJob(QString nonce)
{
    mutex.lock();
    QString localPath = m_cloudDriveJobs->value(nonce).localFilePath;
    m_isSyncingCache->remove(localPath);
    int removeCount = m_cloudDriveJobs->remove(nonce);
    mutex.unlock();

    qDebug() << "CloudDriveModel::removeJob nonce" << nonce << "removeCount" << removeCount << "m_cloudDriveJobs->count()" << m_cloudDriveJobs->count();
}

int CloudDriveModel::getQueuedJobCount() const
{
    return m_jobQueue->count();
}

void CloudDriveModel::cancelQueuedJobs()
{
    m_jobQueue->clear();
    // TODO Should it also clear jobs? Some QNAM requests may need it after this method call.
    m_cloudDriveJobs->clear();

    qDebug() << "CloudDriveModel::cancelQueuedJobs done m_jobQueue->count()" << m_jobQueue->count() << "m_cloudDriveJobs->count()" << m_cloudDriveJobs->count();
}

void CloudDriveModel::addItem(QString localPath, CloudDriveItem item)
{
    if (insertItemToDB(item) <= 0) {
        qDebug() << "CloudDriveModel::addItem insert failed" << item;
        if (updateItemToDB(item) <= 0) {
            qDebug() << "CloudDriveModel::addItem update failed" << item;
        } else {
            qDebug() << "CloudDriveModel::addItem update done" << item;
        }
    } else {
        qDebug() << "CloudDriveModel::addItem insert done" << item;
    }

    // Remove cache for furthur refresh.
    m_isConnectedCache->remove(localPath);
    m_isDirtyCache->remove(localPath);
    m_isSyncingCache->remove(localPath);
}

void CloudDriveModel::addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash, bool addOnly)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath != "") {
        // Found existing item. Update it.
        if (!addOnly) {
            qDebug() << "CloudDriveModel::addItem found." << item;
            item = CloudDriveItem(type, uid, localPath, remotePath, hash, QDateTime::currentDateTime());
            updateItemToDB(item);
        }
    } else {
        qDebug() << "CloudDriveModel::addItem not found.";
        // Not found, add it right away.
        item = CloudDriveItem(type, uid, localPath, remotePath, hash, QDateTime::currentDateTime());
        insertItemToDB(item);
    }

    // Remove cache for furthur refresh.
    m_isConnectedCache->remove(localPath);
    m_isDirtyCache->remove(localPath);
    m_isSyncingCache->remove(localPath);
}

void CloudDriveModel::removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item =  getItem(localPath, type, uid);
    int removeCount = m_cloudDriveItems->remove(item.localPath, item);
    int deleteCount = deleteItemToDB(type, uid, localPath);

    // Remove cache for furthur refresh.
    m_isConnectedCache->remove(localPath);
    m_isDirtyCache->remove(localPath);
    m_isSyncingCache->remove(localPath);

    qDebug() << "CloudDriveModel::removeItem item" << item << "removeCount" << removeCount << "deleteCount" << deleteCount;
}

void CloudDriveModel::removeItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    if (localPath == "") return;

    int removeCount = 0;
    int deleteCount = 0;

    foreach (CloudDriveItem item, findItemWithChildren(type, uid, localPath)) {
//        qDebug() << "CloudDriveModel::removeItemWithChildren item" << item;

        // TODO Remove only localPath's children which have specified type and uid.
        removeCount += m_cloudDriveItems->remove(item.localPath, item);
        deleteCount += deleteItemToDB(type, uid, item.localPath);

        // Remove cache for furthur refresh.
        m_isConnectedCache->remove(item.localPath);
        m_isDirtyCache->remove(item.localPath);
        m_isSyncingCache->remove(item.localPath);
    }

    qDebug() << "CloudDriveModel::removeItemWithChildren removeCount" << removeCount << "deleteCount" << deleteCount;
}

void CloudDriveModel::removeItems(QString localPath)
{
    QList<CloudDriveItem> items = getItemList(localPath);
    foreach (CloudDriveItem item, items) {
        m_cloudDriveItems->remove(item.localPath, item);

        // Remove cache for furthur refresh.
        m_isConnectedCache->remove(item.localPath);
        m_isDirtyCache->remove(item.localPath);
        m_isSyncingCache->remove(item.localPath);
    }

    if (getItemList(localPath).isEmpty()) {
        qDebug() << "CloudDriveModel::removeItems items" << getItemList(localPath);
    }
}

int CloudDriveModel::removeItemByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath)
{
    QSqlQuery qry(m_db);
    qry.prepare("DELETE FROM cloud_drive_item where type = :type AND uid = :uid AND remote_path = :remote_path");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remotePath);
    bool res = qry.exec();
    if (res) {
        qDebug() << "CloudDriveModel::removeItemByRemotePath" << type << uid << remotePath << "success numRowsAffected" << qry.numRowsAffected();
        return qry.numRowsAffected();
    } else {
        qDebug() << "CloudDriveModel::removeItemByRemotePath" << type << uid << remotePath << "failed" << qry.lastError() << "lastQuery" << qry.lastQuery();
        return -1;
    }
}

void CloudDriveModel::updateItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString hash)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    item.hash = hash;
    m_cloudDriveItems->replace(item.localPath, item);
    int updateCount = updateItemToDB(item);

    // Remove cache for furthur refresh.
    m_isConnectedCache->remove(localPath);
    m_isDirtyCache->remove(localPath);
    m_isSyncingCache->remove(localPath);

    qDebug() << "CloudDriveModel::updateItem updateCount" << updateCount;
}

void CloudDriveModel::updateItems(QString localPath, QString hash)
{
    QList<CloudDriveItem> items = getItemList(localPath);
    foreach (CloudDriveItem item, items) {
        item.hash = hash;
        m_cloudDriveItems->replace(item.localPath, item);
    }

    // Update hash to DB.
    updateItemHashByLocalPathToDB(localPath, hash);

    // Remove cache for furthur refresh.
    m_isConnectedCache->remove(localPath);
    m_isDirtyCache->remove(localPath);
    m_isSyncingCache->remove(localPath);

    if (!items.isEmpty()) {
        qDebug() << "CloudDriveModel::updateItems items" << getItemList(localPath);
    }
}

void CloudDriveModel::updateItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString newLocalPath, QString newRemotePath, QString newChildrenHash, QString newHash)
{
    qDebug() << "CloudDriveModel::updateItemWithChildren localPath" << localPath << "remotePath" << remotePath << "newLocalPath" << newLocalPath << "newRemotePath" << newRemotePath << "newChildrenHash" << newChildrenHash;

    int updateCount = 0;

    foreach (CloudDriveItem item, findItemWithChildren(type, uid, localPath)) {
        qDebug() << "CloudDriveModel::updateItemWithChildren before item" << item;
        // Set hash for children only.
        if (item.localPath != localPath) {
            // Set hash = DirtyHash to hint syncFromLocal to put files.
            // Set hash = empty to hint metadata to get files, otherwise syncFromLocal to put local files which are remained with empty hash.
            item.hash = newChildrenHash;
        } else if (newHash != ""){
            item.hash = newHash;
        }

        item.localPath = item.localPath.replace(QRegExp("^" + localPath), newLocalPath);
        item.remotePath = item.remotePath.replace(QRegExp("^" + remotePath), newRemotePath);
        qDebug() << "CloudDriveModel::updateItemWithChildren after item" << item;

        // Update to DB.
        int res = updateItemToDB(item);
        if (res <= 0) {
            res = insertItemToDB(item);
        }
        updateCount += res;

        // Remove cache for furthur refresh.
        m_isConnectedCache->remove(item.localPath);
        m_isDirtyCache->remove(item.localPath);
        m_isSyncingCache->remove(item.localPath);
    }

    qDebug() << "CloudDriveModel::updateItemWithChildren updateCount" << updateCount;
}

int CloudDriveModel::updateItemCronExp(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString cronExp)
{
    QSqlQuery qry(m_db);
    qry.prepare("UPDATE cloud_drive_item SET cron_exp = :cron_exp WHERE type = :type AND uid = :uid AND local_path = :local_path");
    qry.bindValue(":cron_exp", cronExp);
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":local_path", localPath);
    bool res = qry.exec();
    if (res) {
        qDebug() << "CloudDriveModel::updateItemCronExpToDB done" << qry.numRowsAffected();
        return qry.numRowsAffected();
    } else {
        qDebug() << "CloudDriveModel::updateItemCronExpToDB failed" << qry.lastError();
        return qry.numRowsAffected();
    }
}

QString CloudDriveModel::getItemCronExp(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    QString cronExp = "";

    QSqlQuery qry(m_db);
    qry.prepare("SELECT cron_exp FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":local_path", localPath);
    bool res = qry.exec();
    if (res && qry.next()) {
        cronExp = qry.value(qry.record().indexOf("cron_exp")).toString();
        qDebug() << "CloudDriveModel::getItemCronExp done" << cronExp;
    } else {
        qDebug() << "CloudDriveModel::getItemCronExp failed" << qry.lastError();
    }

    return cronExp;
}

void CloudDriveModel::loadScheduledItems(QString cronValue)
{
    // TODO match each item in next minute. Cache matched items for next firing.
    QString cronExp;
    CloudDriveItem item;
    QSqlQuery ps(m_db);
    ps.prepare("SELECT * FROM cloud_drive_item WHERE cron_exp <> ''");
    if (ps.exec()) {
        QSqlRecord rec = ps.record();
        while (ps.next()) {
            if (ps.isValid()) {
                item.type = ps.value(rec.indexOf("type")).toInt();
                item.uid = ps.value(rec.indexOf("uid")).toString();
                item.localPath = ps.value(rec.indexOf("local_path")).toString();
                item.remotePath = ps.value(rec.indexOf("remote_path")).toString();
                item.hash = ps.value(rec.indexOf("hash")).toString();
                item.lastModified = ps.value(rec.indexOf("last_modified")).toDateTime();

                cronExp = ps.value(rec.indexOf("cron_exp")).toString();
                // TODO Match cronExp with cronValue. Ex. cronValue 0 8 12 10 5 <-- 8:00 on 12-Oct Friday
                if (matchCronExp(cronExp, cronValue)) {
                    qDebug() << "CloudDriveModel::loadScheduledItems schedule sync item" << item;
                    m_scheduledItems->enqueue(item);
                } else {
                    qDebug() << "CloudDriveModel::loadScheduledItems discard item" << item;
                }
            } else {
                qDebug() << "CloudDriveModel::loadScheduledItems record position is invalid. ps.lastError()" << ps.lastError();
                break;
            }
        }
    }
}

void CloudDriveModel::syncScheduledItems()
{
    while (!m_scheduledItems->isEmpty()) {
        CloudDriveItem item = m_scheduledItems->dequeue();
        switch (item.type) {
        case Dropbox:
            qDebug() << "CloudDriveModel::syncScheduledItems dequeue and sync item" << item;
            metadata(Dropbox, item.uid, item.localPath, item.remotePath, -1);
            break;
        case SkyDrive:
            qDebug() << "CloudDriveModel::syncScheduledItems dequeue and sync item" << item;
            metadata(SkyDrive, item.uid, item.localPath, item.remotePath, -1);
            break;
        }
    }
}

bool CloudDriveModel::matchCronExp(QString cronExp, QString cronValue)
{
    qDebug() << "CloudDriveModel::matchCronExp start cronExp" << cronExp << "cronValue" << cronValue;

    QStringList cronExpList = cronExp.split(" ");
    QStringList cronValueList = cronValue.split(" ");

    if (cronExpList.length() != cronValueList.length()) {
        qDebug() << "CloudDriveModel::matchCronExp failed cronValueList" << cronValueList << "length" << cronValueList.length();
        return false;
    }

    QString e;
    QString v;
    for (int i = 0; i < cronExpList.length(); i++) {
        e = cronExpList.at(i);
        v = cronValueList.at(i);
        qDebug() << "CloudDriveModel::matchCronExp i" << i << "e" << e << "v" << v;

        // TODO Support - and , in e.
        if (e == "*") {
            // continue.
        } else if (e.indexOf("*/") == 0) {
            int div = e.mid(e.indexOf("/")+1).toInt();
            int mod = v.toInt() % div;
            if (mod == 0) {
                // continue.
            } else {
                qDebug() << "CloudDriveModel::matchCronExp failed i" << i << "e" << e << "v" << v << "div" << div << "mod" << mod;
                return false;
            }
        } else if (e.toInt() == v.toInt()) {
            // continue.
        } else {
            qDebug() << "CloudDriveModel::matchCronExp failed i" << i << "e" << e << "v" << v;
            return false;
        }
    }

    qDebug() << "CloudDriveModel::matchCronExp success cronExp" << cronExp << "cronValue" << cronValue;
    return true;
}

int CloudDriveModel::getItemCount()
{
    // Get count from DB and DAT.
    int dbCount = countItemDB();
    int datCount = m_cloudDriveItems->count();
    qDebug() << "CloudDriveModel::getItemCount db" << dbCount << "dat" << datCount << "total" << (dbCount + datCount);
    return (dbCount + datCount);
}

QString CloudDriveModel::getItemListJson(QString localPath)
{
    QList<CloudDriveItem> list = getItemList(localPath);

    QString jsonText;
    foreach (CloudDriveItem item, list) {
        if (jsonText != "") jsonText.append(", ");
        jsonText.append( item.toJsonText() );
    }

    return "[ " + jsonText + " ]";
}

QString CloudDriveModel::getItemJson(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return item.toJsonText();
}

QString CloudDriveModel::getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return item.remotePath;
}

QString CloudDriveModel::getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
//    qDebug() << "CloudDriveModel::getItemHash type" << type << "uid" << uid << "localPath" << localPath << "item.hash" << item.hash;
    return item.hash;
}

QString CloudDriveModel::getDefaultLocalFilePath(const QString &remoteFilePath)
{
#if defined(Q_WS_SIMULATOR)
    // TODO Support Simulator on Mac, Linux.
    QRegExp rx("^(/*)([C-F])(.+)$");
    rx.indexIn(remoteFilePath);
    if (rx.captureCount() == 3) {
        return rx.cap(2).append(":").append(rx.cap(3));
    } else if (rx.captureCount() == 2) {
        return rx.cap(1).append(":").append(rx.cap(2));
    }
#elif defined(Q_OS_SYMBIAN)
    QRegExp rx("^(/*)([C-F])(.+)$");
    rx.indexIn(remoteFilePath);
    if (rx.captureCount() == 3) {
        return rx.cap(2).append(":").append(rx.cap(3));
    } else if (rx.captureCount() == 2) {
        return rx.cap(1).append(":").append(rx.cap(2));
    }
#elif defined(Q_WS_HARMATTAN)
    // Map /home/user/MyDocs/ to /E/ on remote to share the same remote root as E:/ on Symbian.
    // TODO Make it more configuration.
    QRegExp rx("^(/E/)(.+)$");
    rx.indexIn(remoteFilePath);
    if (rx.captureCount() == 2 && rx.cap(1) != "" & rx.cap(2) != "") {
        return "/home/user/MyDocs/" + rx.cap(2);
    }
#endif
    return "";
}

QString CloudDriveModel::getDefaultRemoteFilePath(const QString &localFilePath)
{
#if defined(Q_WS_SIMULATOR)
    // TODO Support Simulator on Mac, Linux.
    QRegExp rx("^([C-F])(:)(.+)$");
    rx.indexIn(localFilePath);
    if (rx.captureCount() == 3) {
        return "/" + rx.cap(1).append(rx.cap(3));
    }
#elif defined(Q_OS_SYMBIAN)
    QRegExp rx("^([C-F])(:)(.+)$");
    rx.indexIn(localFilePath);
    if (rx.captureCount() == 3) {
        return "/" + rx.cap(1).append(rx.cap(3));
    }
#elif defined(Q_WS_HARMATTAN)
    // Map /home/user/MyDocs/ to /E/ on remote to share the same remote root as E:/ on Symbian.
    // TODO Make it more configuration.
    QRegExp rx("^(/home/user/MyDocs/)(.+)$");
    rx.indexIn(localFilePath);
//    for (int i=1; i<=rx.captureCount(); i++) {
//        qDebug() << "CloudDriveModel::getDefaultRemoteFilePath rx.cap i" << i << rx.cap(i);
//    }
    if (rx.captureCount() == 2 && rx.cap(1) != "" & rx.cap(2) != "") {
        return "/E/" + rx.cap(2);
    }
#endif
    return "";
}

bool CloudDriveModel::isAuthorized()
{
    // TODO check if any cloud drive is authorized.
    return dbClient->isAuthorized() || skdClient->isAuthorized() || ftpClient->isAuthorized();
}

bool CloudDriveModel::isAuthorized(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        return dbClient->isAuthorized();
//    case GoogleDrive:
//        return gcdClient->isAuthorized();
    case SkyDrive:
        return skdClient->isAuthorized();
    case Ftp:
        return ftpClient->isAuthorized();
    }

    return false;
}

QStringList CloudDriveModel::getStoredUidList()
{
    QStringList uidList;

    uidList.append(dbClient->getStoredUidList());
//    uidList.append(gcdClient->getStoredUidList());
    uidList.append(skdClient->getStoredUidList());
    uidList.append(ftpClient->getStoredUidList());

    return uidList;
}

QStringList CloudDriveModel::getStoredUidList(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        return dbClient->getStoredUidList();
//    case GoogleDrive:
//        return gcdClient->getStoredUidList();
    case SkyDrive:
        return skdClient->getStoredUidList();
    case Ftp:
        return ftpClient->getStoredUidList();
    }

    return QStringList();
}

int CloudDriveModel::removeUid(CloudDriveModel::ClientTypes type, QString uid)
{
    switch (type) {
    case Dropbox:
        return dbClient->removeUid(uid);
    case SkyDrive:
        return skdClient->removeUid(uid);
    case Ftp:
        return ftpClient->removeUid(uid);
    }

    return -1;
}

void CloudDriveModel::requestJobQueueStatus()
{
    emit jobQueueStatusSignal(runningJobCount, m_jobQueue->count(), getItemCount());
}

void CloudDriveModel::suspendNextJob()
{
    QApplication::processEvents();

    m_isSuspended = true;
}

void CloudDriveModel::resumeNextJob()
{
    m_isSuspended = false;

    proceedNextJob();
}

void CloudDriveModel::cleanItems()
{
    foreach (CloudDriveItem item, m_cloudDriveItems->values()) {
        cleanItem(item);
    }
}

bool CloudDriveModel::cleanItem(const CloudDriveItem &item)
{
//    qDebug() << "CloudDriveModel::cleanItem item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
    bool isInvalid = false;
    QFileInfo info(item.localPath);
    if (!info.exists()) {
        isInvalid = true;
    } else if (item.localPath.startsWith(":") || item.localPath == "" || item.remotePath == "") {
        // TODO override remove for unwanted item.
        isInvalid = true;
    }

    if (isInvalid) {
        qDebug() << "CloudDriveModel::cleanItem remove item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
        m_cloudDriveItems->remove(item.localPath, item);
        switch (item.type) {
        case Dropbox:
            deleteItemToDB(Dropbox, item.uid, item.localPath);
            break;
        }
    } else {
        // TODO (Migration) Insert to DB.
//        qDebug() << "CloudDriveModel::cleanItem migrate" << item;
//        if (insertItemToDB(item) > 0) {
//            m_cloudDriveItems.remove(item.localPath, item);
//            qDebug() << "CloudDriveModel::cleanItem migrate m_cloudDriveItems.count()" << m_cloudDriveItems.count();
//        }
    }

    return isInvalid;
}

void CloudDriveModel::syncItems()
{
    // Suspend proceedNextJob().
    suspendNextJob();

    // TODO Refactor to support others cloud storage.
    syncItems(Dropbox);
//    syncItems(GoogleDrive);
    syncItems(SkyDrive);

    // Resume proceedNextJob().
    resumeNextJob();
}

void CloudDriveModel::syncItems(CloudDriveModel::ClientTypes type)
{
//    qDebug() << "CloudDriveModel::syncItems started.";

    // Queue all items for metadata requesting.
    // TODO Queue only topmost items. Suppress if its parent is already queued.
    QScriptEngine engine;
    QScriptValue sc;
    foreach (QString uidJson, getStoredUidList(type)) {
//        qDebug() << "CloudDriveModel::syncItems Dropbox uidJson" << uidJson;

        // Create json object.
        sc = engine.evaluate("(" + uidJson + ")");
        QString uid = sc.property("uid").toString();

        QString lastItemLocalPath = "";
        foreach (CloudDriveItem item, findItems(type, uid)) {
//            qDebug() << "CloudDriveModel::syncItems item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

            // Suppress sync if any items' parent is in queued jobs.
            if (item.localPath.startsWith(lastItemLocalPath) && lastItemLocalPath != "") {
//                qDebug() << "CloudDriveModel::syncItems suppress item localPath" << item.localPath << " as its parent already in queue.";
                continue;
            }

            if (item.uid != "" && item.localPath != "" && item.remotePath != "") {
                qDebug() << "CloudDriveModel::syncItems sync item" << item;
                metadata(type, item.uid, item.localPath, item.remotePath, -1);
                lastItemLocalPath = item.localPath;
            } else {
                qDebug() << "CloudDriveModel::syncItems suppress invalid item" << item;
            }

            // Process events to avoid freezing UI.
            QApplication::processEvents();
        }
    }
}

void CloudDriveModel::syncItem(const QString localFilePath)
{
    // Queue localFilePath's items for metadata requesting.
    foreach (CloudDriveItem item, getItemList(localFilePath)) {
        qDebug() << "CloudDriveModel::syncItem item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

        switch (item.type) {
        case Dropbox:
            metadata(Dropbox, item.uid, item.localPath, item.remotePath, -1);
            break;
        case SkyDrive:
            metadata(SkyDrive, item.uid, item.localPath, item.remotePath, -1);
            break;
        }
    }
}

void CloudDriveModel::syncItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath == localPath) {
        switch (item.type) {
        case Dropbox:
            metadata(Dropbox, item.uid, item.localPath, item.remotePath, -1);
            break;
        case SkyDrive:
            metadata(SkyDrive, item.uid, item.localPath, item.remotePath, -1);
            break;
        }
    }
}

int CloudDriveModel::getCloudDriveItemsCount()
{
    return m_cloudDriveItems->count();
}

void CloudDriveModel::migrateCloudDriveItemsToDB()
{
    if (m_cloudDriveItems->count() <= 0) return;

    qint64 total = m_cloudDriveItems->count();
    qint64 c = 0;

    emit migrateStartedSignal(total);

    foreach (CloudDriveItem item, m_cloudDriveItems->values()) {
        QApplication::processEvents(QEventLoop::AllEvents, 50);

        if (insertItemToDB(item) > 0) {
            m_cloudDriveItems->remove(item.localPath, item);
            qDebug() << "CloudDriveModel::migrateCloudDriveItemsToDB migrate" << item << "m_cloudDriveItems count" << m_cloudDriveItems->count();

            switch (item.type) {
            case Dropbox:
                emit migrateProgressSignal(Dropbox, item.uid, item.localPath, item.remotePath, ++c, total);
                break;
            }
        }
    }

    emit migrateFinishedSignal(c, total);
}

void CloudDriveModel::requestToken(CloudDriveModel::ClientTypes type)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), RequestToken, type, "", "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::authorize(CloudDriveModel::ClientTypes type)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Authorize, type, "", "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    emit proceedNextJobSignal();
}

bool CloudDriveModel::parseAuthorizationCode(CloudDriveModel::ClientTypes type, QString text)
{
//    switch (type) {
//    case GoogleDrive:
//        return gcdClient->parseAuthorizationCode(text);
//    }

    return false;
}

void CloudDriveModel::accessToken(CloudDriveModel::ClientTypes type, QString pin)
{
    // Store access token pin temporarily.
    accessTokenPin = pin;

    // Enqueue job.
    CloudDriveJob job(createNonce(), AccessToken, type, "", "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::refreshToken(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), RefreshToken, type, uid, "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::accountInfo(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), AccountInfo, type, uid, "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::quota(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Quota, type, uid, "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath, int modelIndex)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), FileGet, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isConnectedCache->remove(localFilePath);
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
//    switch (job.type) {
//    case Dropbox:
//        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    case SkyDrive:
//        addItem(SkyDrive, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex)
{
    // TODO Restrict file types.
    if (!canSync(localFilePath)) {
        qDebug() << "CloudDriveModel::filePut localFilePath" << localFilePath << " is restricted, can't upload.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), FilePut, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isConnectedCache->remove(localFilePath);
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
//    switch (job.type) {
//    case Dropbox:
//        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    case SkyDrive:
//        addItem(SkyDrive, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex)
{
    // TODO Restrict file types.
    if (!canSync(localFilePath)) {
        qDebug() << "CloudDriveModel::metadata localFilePath" << localFilePath << " is restricted, can't sync.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), Metadata, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isConnectedCache->remove(localFilePath);
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // ISSUE it created cloud icon on selected item even use cancel sync.
    // TODO handle other clouds.
//    switch (job.type) {
//    case Dropbox:
//        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    case SkyDrive:
//        addItem(SkyDrive, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::browse(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Browse, type, uid, "", remoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->insert(0, job.jobId); // Browse get priority.

    emit proceedNextJobSignal();
}

void CloudDriveModel::syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex, bool forcePut)
{
    /* TODO Handling remotePath.
     *
     * For Dropbox, remotePath is remote absolute path of specified localPath.  Ex. /D/A
     * For SkyDrive, remotePath is unique ID. Ex. folder.xxxxxxxx.yyyy!sss. By default it will not be specified.
    */

    // This method is invoked from dir only as file which is not found will be put right away.
    qDebug() << "----- CloudDriveModel::syncFromLocal -----" << type << uid << localPath << remotePath << modelIndex << "forcePut" << forcePut;

    if (localPath == "" || remotePath == "") return;

    QFileInfo info(localPath);
    if (info.isDir()) {
        // Sync based on local contents.

        // TODO create remote directory if no content or pending refresh metadata.
        CloudDriveItem cloudItem = getItem(localPath, type, uid);
        if (cloudItem.localPath == "" || cloudItem.hash == CloudDriveModel::DirtyHash) {
            qDebug() << "CloudDriveModel::syncFromLocal not found cloudItem. Invoke creatFolder.";
            createFolder(type, uid, localPath, remotePath, modelIndex);
        } else {
            qDebug() << "CloudDriveModel::syncFromLocal found cloudItem" << cloudItem;
        }

        QDir dir(info.absoluteFilePath());
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        foreach (QFileInfo item, dir.entryInfoList()) {
            // Process events to avoid freezing UI.
            QApplication::processEvents();

            QString localFilePath = item.absoluteFilePath();
            CloudDriveItem cloudDriveItem = getItem(localFilePath, type, uid);
//            qDebug() << "CloudDriveModel::syncFromLocal item" << type << uid << localFilePath << cloudDriveItem.hash;

            // If dir/file don't have localHash which means it's not synced, put it right away.
            // If forcePut, put it right away.
            if (forcePut || cloudDriveItem.hash == "" || cloudDriveItem.remotePath == "") {
                QString remoteFilePath;
                remoteFilePath = remotePath + "/" + item.fileName();

                // Sync dir/file then it will decide whether get/put/do nothing by metadataReply.
                qDebug() << "CloudDriveModel::syncFromLocal new local item" << type << uid << localFilePath << remoteFilePath << cloudDriveItem.hash;

                if (forcePut) {
                    if (item.isDir()) {
                        // Drilldown local dir recursively.
                        syncFromLocal(type, uid, localFilePath, remoteFilePath, -1, forcePut);
                    } else {
                        filePut(type, uid, localFilePath, remoteFilePath, -1);
                    }
                } else {
                    metadata(type, uid, localFilePath, remoteFilePath, -1);
                }
            } else {
                // Skip any items that already have CloudDriveItem and 's localHash.
//                qDebug() << "CloudDriveModel::syncFromLocal skip existing local item" << type << uid << localFilePath << remoteFilePath << cloudDriveItem.hash;
            }
        }

        // TODO avoid having below line. It caused infinite loop.
        // Update hash for itself will be requested from QML externally.
    } else {
        qDebug() << "CloudDriveModel::syncFromLocal file is not supported." << type << uid << localPath << remotePath << modelIndex << "forcePut" << forcePut;
    }
}

void CloudDriveModel::syncFromLocal_SkyDrive(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, int modelIndex, bool forcePut)
{
    /* TODO Handling remotePath.
     *
     * For Dropbox, remotePath is remote absolute path of specified localPath.  Ex. /D/A
     * For SkyDrive, remotePath is unique ID. Ex. folder.xxxxxxxx.yyyy!sss. By default it will not be specified.
    */

    // This method is invoked from dir only as file which is not found will be put right away.
    qDebug() << "----- CloudDriveModel::syncFromLocal_SkyDrive -----" << type << uid << localPath << remoteParentPath << modelIndex << "forcePut" << forcePut;

    if (localPath == "") return;

    QApplication::processEvents();

    QFileInfo info(localPath);
    if (info.isDir()) {
        // Sync based on local contents.

        // TODO create remote directory if no content or pending refresh metadata.
        CloudDriveItem parentCloudDriveItem = getItem(localPath, type, uid);
        if (parentCloudDriveItem.localPath == "" || parentCloudDriveItem.hash == CloudDriveModel::DirtyHash) {
            qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive not found parentCloudDriveItem. Invoke creatFolder.";
            // Get remoteParentPath from localParentPath's cloudDriveItem if it's not specified.
            remoteParentPath = (remoteParentPath == "") ? getItemRemotePath(info.absolutePath(), type, uid) : remoteParentPath;
            qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive remoteParentPath" << remoteParentPath;

            // TODO Request SkyDriveClient's createFolder synchronously.
            QNetworkReply * createFolderReply = skdClient->createFolder(createNonce(), uid, info.fileName(), remoteParentPath, true);
            if (createFolderReply->error() == QNetworkReply::NoError) {
                QString createFolderReplyResult(createFolderReply->readAll());
                qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive createFolder success" << createFolderReplyResult;

                QScriptEngine se;
                QScriptValue sc = se.evaluate("(" + createFolderReplyResult + ")");
                QString createdRemotePath = sc.property("id").toString();
                QString createdRemoteHash = sc.property("updated_time").toString();

                // Update parentCloudDriveItem.
                addItem(type, uid, localPath, createdRemotePath, createdRemoteHash);
                parentCloudDriveItem = getItem(localPath, type, uid);
            } else {
                qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive createFolder error" << createFolderReply->error() << createFolderReply->errorString() << createFolderReply->readAll();
                return;
            }
        } else {
            qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive found parentCloudDriveItem" << parentCloudDriveItem;
        }

        QDir dir(info.absoluteFilePath());
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        foreach (QFileInfo item, dir.entryInfoList()) {
            // Process events to avoid freezing UI.
            QApplication::processEvents();

            QString localFilePath = item.absoluteFilePath();
            CloudDriveItem cloudDriveItem = getItem(localFilePath, type, uid);
//            qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive item" << type << uid << localFilePath << cloudDriveItem.hash;

            // If dir/file don't have localHash which means it's not synced, put it right away.
            // If forcePut, put it right away.
            if (forcePut || cloudDriveItem.hash == "" || cloudDriveItem.remotePath == "") {
                // Sync dir/file then it will decide whether get/put/do nothing by metadataReply.
                qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive new local item" << type << uid << localFilePath << cloudDriveItem.hash << "parentCloudDriveItem.remotePath" << parentCloudDriveItem.remotePath;

                if (item.isDir()) {
                    // Drilldown local dir recursively.
                    // TODO Get new remote folder path.
                    syncFromLocal_SkyDrive(type, uid, localFilePath, parentCloudDriveItem.remotePath, -1, forcePut);
                } else {
                    // Put file to remote parent path.
                    filePut(type, uid, localFilePath, parentCloudDriveItem.remotePath, -1);
                }
            } else {
                // Skip any items that already have CloudDriveItem and 's localHash.
                qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive skip existing local item" << type << uid << localFilePath << cloudDriveItem.remotePath << cloudDriveItem.hash;
            }
        }

        // TODO avoid having below line. It caused infinite loop.
        // Update hash for itself will be requested from QML externally.
    } else {
        qDebug() << "CloudDriveModel::syncFromLocal_SkyDrive file is not supported." << type << uid << localPath << remoteParentPath << modelIndex << "forcePut" << forcePut;
    }
}

void CloudDriveModel:: createFolder(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), CreateFolder, type, uid, localPath, remotePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // TODO Does it need?
    m_isSyncingCache->remove(localPath);
    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localPath);

//    // Add item with dirtyHash to avoid duplicate sync job.
//    // TODO handle other clouds.
//    switch (job.type) {
//    case Dropbox:
//        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    }

    emit proceedNextJobSignal();
}

QString CloudDriveModel::createFolder_SkyDrive(CloudDriveModel::ClientTypes type, QString uid, QString newRemoteFolderName, QString remoteParentPath)
{
    // Request SkyDriveClient's createFolder synchronously.
    QNetworkReply * createFolderReply = skdClient->createFolder(createNonce(), uid, newRemoteFolderName, remoteParentPath, true);
    if (createFolderReply->error() == QNetworkReply::NoError) {
        QString createFolderReplyResult(createFolderReply->readAll());
        qDebug() << "CloudDriveModel::createFolder_SkyDrive createFolder success" << createFolderReplyResult;

        QScriptEngine se;
        QScriptValue sc = se.evaluate("(" + createFolderReplyResult + ")");
        QString createdRemotePath = sc.property("id").toString();
        QString createdRemoteHash = sc.property("updated_time").toString();

        return createdRemotePath;
    }

    return "";
}

void CloudDriveModel::moveFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), MoveFile, type, uid, localFilePath, remoteFilePath, newLocalFilePath, newRemoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);

//    // Add item with dirtyHash to avoid duplicate sync job.
//    // TODO handle other clouds.
//    switch (job.type) {
//    case Dropbox:
//        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    case SkyDrive:
//        addItem(SkyDrive, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::copyFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), CopyFile, type, uid, localFilePath, remoteFilePath, newLocalFilePath, newRemoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);

//    // Add item with dirtyHash to avoid duplicate sync job.
//    // TODO handle other clouds.
//    switch (job.type) {
//    case Dropbox:
//        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    case SkyDrive:
//        addItem(SkyDrive, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::deleteFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), DeleteFile, type, uid, localFilePath, remoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);

//    // Add item with dirtyHash to avoid duplicate sync job.
//    // TODO handle other clouds.
//    switch (job.type) {
//    case Dropbox:
//        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    case SkyDrive:
//        addItem(SkyDrive, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
//        break;
//    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::shareFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), ShareFile, type, uid, localFilePath, remoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    emit proceedNextJobSignal();
}

void CloudDriveModel::fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;
    QNetworkReply *propertyReply;

    if (err == 0) {
        // TODO handle other clouds.
        switch (job.type) {
        case Dropbox:
            sc = engine.evaluate("(" + msg + ")");
            hash = sc.property("rev").toString();
            addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, hash);
            break;
        case SkyDrive:
            propertyReply = skdClient->property(nonce, job.uid, job.remoteFilePath, true);
            if (propertyReply->error() == QNetworkReply::NoError) {
                sc = engine.evaluate("(" + propertyReply->readAll() + ")");
                hash = sc.property("updated_time").toString();
                addItem(SkyDrive, job.uid, job.localFilePath, job.remoteFilePath, hash);
            }
            break;
        case Ftp:
            sc = engine.evaluate("(" + msg + ")");
            hash = sc.property("lastModified").toString();
            addItem(Ftp, job.uid, job.localFilePath, job.remoteFilePath, hash);
            break;
        }

        job.isRunning = false;
        m_cloudDriveJobs->insert(nonce, job);
    } else {
        removeItem(getJobType(job.type), job.uid, job.localFilePath);
    }

    // Notify job done.
    jobDone();

    emit fileGetReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::filePutReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;
    QString remoteFilePath;
    QNetworkReply *propertyReply;

    if (err == 0) {
        // TODO handle other clouds.
        switch (job.type) {
        case Dropbox:
            sc = engine.evaluate("(" + msg + ")");
            hash = sc.property("rev").toString();
            addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, hash);
            break;
        case SkyDrive:
            // Parse result and update remote file path to item.
            sc = engine.evaluate("(" + msg + ")");
            remoteFilePath = sc.property("id").toString();
            // Get property synchronously by omitting callback parameter.
            propertyReply = skdClient->property(nonce, job.uid, remoteFilePath, true);
            if (propertyReply->error() == QNetworkReply::NoError) {
                sc = engine.evaluate("(" + propertyReply->readAll() + ")");
                hash = sc.property("updated_time").toString();
                addItem(SkyDrive, job.uid, job.localFilePath, remoteFilePath, hash);
            }
            break;
        case Ftp:
            // Parse result and update remote file path to item.
            sc = engine.evaluate("(" + msg + ")");
            remoteFilePath = sc.property("path").toString();
            hash = sc.property("lastModified").toString();
            addItem(Ftp, job.uid, job.localFilePath, remoteFilePath, hash);
            break;
        }

        job.isRunning = false;
        m_cloudDriveJobs->insert(nonce, job);
    } else {
        removeItem(getJobType(job.type), job.uid, job.localFilePath);
    }

    // Notify job done.
    jobDone();

    emit filePutReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;
    QString remoteFilePath;

    if (err == 0) {
        // TODO generalize to support other clouds.
        switch (job.type) {
        case Dropbox:
            // Don't update hash to item yet. Hash will be updated by fileGet/filePut.
            break;
        case SkyDrive:
            // TODO Parse result and update remote file path to item.
//            sc = engine.evaluate("(" + msg + ")");
//            remoteFilePath = sc.property("property").property("id").toString();
//            hash = sc.property("property").property("updated_time").toString();
//            addItem(SkyDrive, job.uid, job.localFilePath, remoteFilePath, hash);
            break;
        case Ftp:
            // TODO Parse result and update remote file path to item.
//            sc = engine.evaluate("(" + msg + ")");
//            remoteFilePath = sc.property("property").property("path").toString();
//            hash = sc.property("property").property("lastModified").toString();
//            addItem(Ftp, job.uid, job.localFilePath, remoteFilePath, hash);
            break;
        }

        job.isRunning = false;
        m_cloudDriveJobs->insert(nonce, job);
    } else if (err == 202) {
        // Issue: handle 202 Nonce already in used.
        // Solution: let QML handle retry.
    }

    // Notify job done.
    jobDone();

    emit metadataReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::browseReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    if (err == 0) {
        // TODO generalize to support other clouds.
        job.isRunning = false;
        m_cloudDriveJobs->insert(nonce, job);
    }

    // Notify job done.
    jobDone();

    emit browseReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::createFolderReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;
    QString createdRemotePath;

    // TODO Create folder effects only remote side. Does it need?
    if (err == 0) {
        // TODO generalize to support other clouds.
//        switch (job.type) {
//        case Dropbox:
//            sc = engine.evaluate("(" + msg + ")");
//            hash = sc.property("rev").toString();
//            if (job.localFilePath != "") {
//                // Add cloud item if localPath is specified.
//                addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, hash);
//            }
//            break;
//        case SkyDrive:
//            // REMARK For SkyDrive, job.localFilePath stores newRemoteFolderName.
//            sc = engine.evaluate("(" + msg + ")");
//            hash = sc.property("updated_time").toString();
//            createdRemotePath = sc.property("id").toString();
//            if (job.localFilePath != "") {
//                // Add cloud item if localPath is specified.
//                addItem(SkyDrive, job.uid, job.localFilePath, createdRemotePath, hash);
//            }
//            break;
//        case Ftp:
//            sc = engine.evaluate("(" + msg + ")");
//            hash = sc.property("lastModified").toString();
//            if (job.localFilePath != "") {
//                // Add cloud item if localPath is specified.
//                addItem(Ftp, job.uid, job.localFilePath, job.remoteFilePath, hash);
//            }
//            break;
//        }
    } else if (err == 202) {
        // Forbidden {"error": " at path 'The folder '???' already exists.'"}
        // Do nothing.
    } else {
//        if (job.localFilePath != "") {
//            removeItem(getJobType(job.type), job.uid, job.localFilePath);
//        }

//        switch (job.type) {
//        case Dropbox:
//            // Remove failed item if localPath is specified.
//            if (job.localFilePath != "") {
//                removeItem(Dropbox, job.uid, job.localFilePath);
//            }
//            break;
//        case SkyDrive:
//            // Remove failed item if localPath is specified.
//            if (job.localFilePath != "") {
//                removeItem(SkyDrive, job.uid, job.localFilePath);
//            }
//            break;
//        case Ftp:
//            // Remove failed item if localPath is specified.
//            if (job.localFilePath != "") {
//                removeItem(Ftp, job.uid, job.localFilePath);
//            }
//            break;
//        }
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit createFolderReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::moveFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;
    QString rev;
    bool isDir;

    if (err == 0) {
        // TODO generalize to support other clouds.
        switch (job.type) {
        case Dropbox:
            sc = engine.evaluate("(" + msg + ")");
            hash = sc.property("hash").toString();
            rev = sc.property("rev").toString();
            isDir = sc.property("is_dir").toBool();
            updateItemWithChildren(Dropbox, job.uid,
                                   job.localFilePath, job.remoteFilePath,
                                   job.newLocalFilePath, job.newRemoteFilePath, rev, (isDir ? hash : rev) );
            removeItemWithChildren(Dropbox, job.uid, job.localFilePath);
            break;
        case SkyDrive:
            // TODO
            break;
        }
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit moveFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::copyFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;

    if (err == 0) {
        // TODO generalize to support other clouds.
        switch (job.type) {
        case Dropbox:
            sc = engine.evaluate("(" + msg + ")");
            hash = sc.property("rev").toString();
            break;
        case SkyDrive:
            // TODO
            break;
        }
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit copyFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::deleteFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Disconnect deleted local path.
    if (job.localFilePath != "") {
        removeItemWithChildren(getJobType(job.type), job.uid, job.localFilePath);
    }
    if (job.remoteFilePath != "") {
        removeItemByRemotePath(getJobType(job.type), job.uid, job.remoteFilePath);
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit deleteFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::shareFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString url = "";
    QString expires = "";

    if (err == 0) {
        // TODO generalize to support other clouds.
        switch (job.type) {
        case Dropbox:
            sc = engine.evaluate("(" + msg + ")");
            url = sc.property("url").toString();
            expires = sc.property("expires").toString();
            break;
        case SkyDrive:
            sc = engine.evaluate("(" + msg + ")");
            url = sc.property("link").toString();
            break;
        }

        job.isRunning = false;
        m_cloudDriveJobs->insert(nonce, job);
    } else if (err == 202) {
        // Issue: handle 202 Nonce already in used.
        // Solution: let QML handle retry.
    }

    // Notify job done.
    jobDone();

    emit shareFileReplySignal(nonce, err, errMsg, msg, url, expires);
}

void CloudDriveModel::schedulerTimeoutFilter()
{
    qDebug() << "CloudDriveModel::schedulerTimeoutFilter" << QDateTime::currentDateTime().toString("d/M/yyyy hh:mm:ss.zzz");

    QString cronValue = QDateTime::currentDateTime().toString("m h d M ddd");

    loadScheduledItems(cronValue);
    suspendNextJob();
    syncScheduledItems();
    resumeNextJob();

    emit schedulerTimeoutSignal();
}

void CloudDriveModel::initializeDB(QString nonce)
{
    emit initializeDBStarted(nonce);

    // Create cache database path if it's not exist.
    QFile file(ITEM_DB_PATH);
    QFileInfo info(file);
    if (!info.absoluteDir().exists()) {
        qDebug() << "CloudDriveModel::initializeDB dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(info.absolutePath());
        if (!res) {
            qDebug() << "CloudDriveModel::initializeDB can't make dir" << info.absolutePath();
        } else {
            qDebug() << "CloudDriveModel::initializeDB make dir" << info.absolutePath();
        }
    }

    // First initialization.
    m_db = QSqlDatabase::addDatabase("QSQLITE", ITEM_DB_CONNECTION_NAME);
    m_db.setDatabaseName(ITEM_DB_PATH);
    bool ok = m_db.open();
    qDebug() << "CloudDriveModel::initializeDB" << ok << "connectionName" << m_db.connectionName() << "databaseName" << m_db.databaseName() << "driverName" << m_db.driverName();

    QSqlQuery query(m_db);
    bool res = false;

    res = query.exec("CREATE TABLE cloud_drive_item(type INTEGER PRIMARY_KEY, uid TEXT PRIMARY_KEY, local_path TEXT PRIMARY_KEY, remote_path TEXT, hash TEXT, last_modified TEXT)");
    if (res) {
        qDebug() << "CloudDriveModel::initializeDB CREATE TABLE cloud_drive_item is done.";
    } else {
        qDebug() << "CloudDriveModel::initializeDB CREATE TABLE cloud_drive_item is failed. Error" << query.lastError();
    }

    res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS cloud_drive_item_pk ON cloud_drive_item (type, uid, local_path)");
    if (res) {
        qDebug() << "CloudDriveModel::initializeDB CREATE INDEX cloud_drive_item_pk is done.";
    } else {
        qDebug() << "CloudDriveModel::initializeDB CREATE INDEX cloud_drive_item_pk is failed. Error" << query.lastError();
    }

    // TODO Additional initialization.
    res = query.exec("ALTER TABLE cloud_drive_item ADD COLUMN cron_exp TEXT");
    if (res) {
        qDebug() << "CloudDriveModel::initializeDB adding column cloud_drive_item.cron_exp is done.";
    } else {
        qDebug() << "CloudDriveModel::initializeDB adding column cloud_drive_item.cron_exp is failed. Error" << query.lastError();
    }

    // Prepare queries.
    m_selectByPrimaryKeyPS = QSqlQuery(m_db);
    m_selectByPrimaryKeyPS.prepare("SELECT * FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path");

    m_selectByTypePS = QSqlQuery(m_db);
    m_selectByTypePS.prepare("SELECT * FROM cloud_drive_item WHERE type = :type ORDER BY uid, local_path");

    m_selectByTypeAndUidPS = QSqlQuery(m_db);
    m_selectByTypeAndUidPS.prepare("SELECT * FROM cloud_drive_item WHERE type = :type AND uid = :uid ORDER BY local_path");

    m_selectByLocalPathPS = QSqlQuery(m_db);
    m_selectByLocalPathPS.prepare("SELECT * FROM cloud_drive_item WHERE local_path = :local_path ORDER BY type, uid");

    m_selectChildrenByPrimaryKeyPS = QSqlQuery(m_db);
    m_selectChildrenByPrimaryKeyPS.prepare("SELECT * FROM cloud_drive_item c WHERE c.local_path like :local_path||'/%' and c.type = :type and c.uid = :uid ORDER BY c.local_path");

    m_insertPS = QSqlQuery(m_db);
    m_insertPS.prepare("INSERT INTO cloud_drive_item(type, uid, local_path, remote_path, hash, last_modified)"
                       " VALUES (:type, :uid, :local_path, :remote_path, :hash, :last_modified)");

    m_updatePS = QSqlQuery(m_db);
    m_updatePS.prepare("UPDATE cloud_drive_item SET"
                       " remote_path = :remote_path, hash = :hash, last_modified = :last_modified"
                       " WHERE type = :type AND uid = :uid AND local_path = :local_path");

    m_updateHashByLocalPathPS = QSqlQuery(m_db);
    m_updateHashByLocalPathPS.prepare("UPDATE cloud_drive_item SET"
                       " hash = :hash"
                       " WHERE local_path = :local_path");

    m_deletePS = QSqlQuery(m_db);
    m_deletePS.prepare("DELETE FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path");

    m_countPS = QSqlQuery(m_db);
    m_countPS.prepare("SELECT count(*) count FROM cloud_drive_item");

    m_countByLocalPathPS = QSqlQuery(m_db);
    m_countByLocalPathPS.prepare("SELECT count(*) count FROM cloud_drive_item where local_path = :local_path");

    // Notify job done.
    jobDone();

    // RemoveJob
    removeJob(nonce);

    emit initializeDBFinished(nonce);
}

void CloudDriveModel::closeDB()
{
    qDebug() << "CloudDriveModel::closeDB" << countItemDB();
    m_db.close();
}

bool CloudDriveModel::updateDropboxPrefix(bool fullAccess)
{
    qDebug() << "CloudDriveModel::updateDropboxPrefix started. fullAccess" << fullAccess;
    QSqlQuery updQry(m_db);
    bool res = false;
    if (fullAccess) {
        updQry.prepare("UPDATE cloud_drive_item SET remote_path = '/Apps/FilesPlus'||remote_path WHERE type = :type AND remote_path NOT LIKE '/Apps/FilesPlus/%'");
        updQry.bindValue(":type", Dropbox);
        res = updQry.exec();
    } else {
        // Remove prefix '/Apps/FilesPlus' from remote path.
        // TODO After done, all fullAccess items will mix with sandboxAccess items.
        updQry.prepare("UPDATE cloud_drive_item SET remote_path = SUBSTR(remote_path, 16) WHERE type = :type AND remote_path LIKE '/Apps/FilesPlus/%'");
        updQry.bindValue(":type", Dropbox);
        res = updQry.exec();
    }
    qDebug() << "CloudDriveModel::updateDropboxPrefix" << res << updQry.numRowsAffected() << updQry.lastQuery();

    if (res) {
        m_db.commit();

        // Clear item cache.
        m_itemCache->clear();
        qDebug() << "CloudDriveModel::updateDropboxPrefix itemCache is cleared.";
    } else {
        m_db.rollback();
    }

    return res;
}

bool CloudDriveModel::testConnection(CloudDriveModel::ClientTypes type, QString hostname, quint16 port, QString username, QString password)
{
    if (type == Ftp && ftpClient != 0) {
        return ftpClient->testConnection(hostname, port, username, password);
    }
    return false;
}

void CloudDriveModel::saveConnection(CloudDriveModel::ClientTypes type, QString uid, QString hostname, quint16 port, QString username, QString password)
{
    if (type == Ftp && ftpClient != 0) {
        ftpClient->saveConnection(uid, hostname, port, username, password);
    }
}

QString CloudDriveModel::getItemCacheKey(int type, QString uid, QString localPath)
{
    return QString("%1,%2,%3").arg(type).arg(uid).arg(localPath);
}

void CloudDriveModel::cleanDB()
{
    QSqlQuery updQry(m_db);
    bool res = false;
    updQry.prepare("DELETE FROM cloud_drive_item WHERE local_path = ''");
    res = updQry.exec();
    if (res) {
        qDebug() << "CloudDriveModel::cleanDB" << updQry.lastQuery() << "res" << res << "numRowsAffected" << updQry.numRowsAffected();
        m_db.rollback();
    } else {
        qDebug() << "CloudDriveModel::cleanDB" << updQry.lastQuery() << "res" << res << "error" << updQry.lastError();
        m_db.commit();
    }
}

QList<CloudDriveItem> CloudDriveModel::getItemListFromPS(QSqlQuery ps) {
    QList<CloudDriveItem> itemList;
    CloudDriveItem item;
    bool res = ps.exec();
    QSqlRecord rec = ps.record();
    if (res) {
        while (ps.next()) {
            if (ps.isValid()) {
                item.type = ps.value(rec.indexOf("type")).toInt();
                item.uid = ps.value(rec.indexOf("uid")).toString();
                item.localPath = ps.value(rec.indexOf("local_path")).toString();
                item.remotePath = ps.value(rec.indexOf("remote_path")).toString();
                item.hash = ps.value(rec.indexOf("hash")).toString();
                item.lastModified = ps.value(rec.indexOf("last_modified")).toDateTime();

                QString itemCacheKey = getItemCacheKey(item.type, item.uid, item.localPath);
    //            qDebug() << "CloudDriveModel::getItemListFromPS key" << itemCacheKey << "item" << item;

                // Insert item to itemCache.
                m_itemCache->insert(itemCacheKey, item);

                itemList.append(item);
            } else {
                qDebug() << "CloudDriveModel::getItemListFromPS record position is invalid. ps.lastError()" << ps.lastError();
            }
        }
    } else {
        qDebug() << "CloudDriveModel::getItemListFromPS record not found. ps.lastError()" << ps.lastError() << "ps.lastQuery()" << ps.lastQuery();
    }

    return itemList;
}

CloudDriveItem CloudDriveModel::selectItemByPrimaryKeyFromDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item;

    // Find in itemCache, return if found.
    QString itemCacheKey = getItemCacheKey(type, uid, localPath);
    if (m_itemCache->contains(itemCacheKey)) {
        item = m_itemCache->value(itemCacheKey);
//        qDebug() << "CloudDriveModel::selectItemFromDB cached key" << itemCacheKey << "item" << item;
        return item;
    }

    m_selectByPrimaryKeyPS.bindValue(":type", type);
    m_selectByPrimaryKeyPS.bindValue(":uid", uid);
    m_selectByPrimaryKeyPS.bindValue(":local_path", localPath);
    QList<CloudDriveItem> itemList = getItemListFromPS(m_selectByPrimaryKeyPS);
    if (itemList.count() > 0) {
        item = itemList.at(0);
//        qDebug() << "CloudDriveModel::selectItemByPrimaryKeyFromDB found. key" << itemCacheKey << "item" << item;
    } else {
//        qDebug() << "CloudDriveModel::selectItemByPrimaryKeyFromDB record not found. key" << itemCacheKey;
    }

    return item;
}

QList<CloudDriveItem> CloudDriveModel::selectItemsFromDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    QString sql = "SELECT * FROM cloud_drive_item WHERE type = :type";
    if (uid != "") {
        sql += " AND uid = :uid";
    }
    if (localPath != "") {
        sql += " AND local_path = :local_path";
    }
    QSqlQuery query(m_db);
    query.prepare(sql);
    query.bindValue(":type", type);
    if (uid != "") query.bindValue(":uid", uid);
    if (localPath != "") query.bindValue(":local_path", localPath);
    return getItemListFromPS(query);
}

QList<CloudDriveItem> CloudDriveModel::selectItemsByLocalPathFromDB(QString localPath)
{
    m_selectByLocalPathPS.bindValue(":local_path", localPath);
    return getItemListFromPS(m_selectByLocalPathPS);
}

QList<CloudDriveItem> CloudDriveModel::selectItemsByTypeFromDB(CloudDriveModel::ClientTypes type)
{
    m_selectByTypePS.bindValue(":type", type);
    return getItemListFromPS(m_selectByTypePS);
}

QList<CloudDriveItem> CloudDriveModel::selectItemsByTypeAndUidFromDB(CloudDriveModel::ClientTypes type, QString uid)
{
    m_selectByTypeAndUidPS.bindValue(":type", type);
    m_selectByTypeAndUidPS.bindValue(":uid", uid);
    return getItemListFromPS(m_selectByTypeAndUidPS);
}

QList<CloudDriveItem> CloudDriveModel::selectChildrenByPrimaryKeyFromDB(ClientTypes type, QString uid, QString localPath)
{
    m_selectChildrenByPrimaryKeyPS.bindValue(":type", type);
    m_selectChildrenByPrimaryKeyPS.bindValue(":uid", uid);
    m_selectChildrenByPrimaryKeyPS.bindValue(":local_path", localPath);
    return getItemListFromPS(m_selectChildrenByPrimaryKeyPS);
}

QList<CloudDriveItem> CloudDriveModel::selectItemsByTypeAndUidAndRemotePathFromDB(CloudDriveModel::ClientTypes type, QString uid, QString remotePath)
{
    QSqlQuery qry(m_db);
    qry.prepare("SELECT * FROM cloud_drive_item WHERE type = :type AND uid = :uid AND remote_path = :remote_path");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remotePath);
    return getItemListFromPS(qry);
}

int CloudDriveModel::insertItemToDB(const CloudDriveItem item)
{
    bool res;

    m_insertPS.bindValue(":type", item.type);
    m_insertPS.bindValue(":uid", item.uid);
    m_insertPS.bindValue(":local_path", item.localPath);
    m_insertPS.bindValue(":remote_path", item.remotePath);
    m_insertPS.bindValue(":hash", item.hash);
    m_insertPS.bindValue(":last_modified", item.lastModified);
    res = m_insertPS.exec();
    if (res) {
        qDebug() << "CloudDriveModel::insertItemToDB insert done" << item << "res" << res << "numRowsAffected" << m_insertPS.numRowsAffected();
        m_db.commit();

        // Insert item to itemCache.
        m_itemCache->insert(getItemCacheKey(item.type, item.uid, item.localPath), item);
    } else {
        qDebug() << "CloudDriveModel::insertItemToDB insert failed" << item << "res" << res << m_insertPS.lastError();
    }

    return m_insertPS.numRowsAffected();
}

int CloudDriveModel::updateItemToDB(const CloudDriveItem item)
{
    bool res;

    m_updatePS.bindValue(":type", item.type);
    m_updatePS.bindValue(":uid", item.uid);
    m_updatePS.bindValue(":local_path", item.localPath);
    m_updatePS.bindValue(":remote_path", item.remotePath);
    m_updatePS.bindValue(":hash", item.hash);
    m_updatePS.bindValue(":last_modified", item.lastModified);
    res = m_updatePS.exec();
    if (res) {
        qDebug() << "CloudDriveModel::updateItemToDB update done" << item << "res" << res << "numRowsAffected" << m_updatePS.numRowsAffected();
        m_db.commit();

        // Insert item to itemCache.
        m_itemCache->insert(getItemCacheKey(item.type, item.uid, item.localPath), item);
    } else {
        qDebug() << "CloudDriveModel::updateItemToDB update failed" << item << "res" << res << m_updatePS.lastError();
    }

    return m_updatePS.numRowsAffected();
}

int CloudDriveModel::updateItemHashByLocalPathToDB(const QString localPath, const QString hash)
{
    bool res;

    m_updateHashByLocalPathPS.bindValue(":local_path", localPath);
    m_updateHashByLocalPathPS.bindValue(":hash", hash);
    res = m_updateHashByLocalPathPS.exec();
    if (res) {
        qDebug() << "CloudDriveModel::updateItemHashByLocalPathToDB update done numRowsAffected" << m_updateHashByLocalPathPS.numRowsAffected();
        m_db.commit();

        // Refresh items to itemCache.
        selectItemsByLocalPathFromDB(localPath);
    } else {
        qDebug() << "CloudDriveModel::updateItemHashByLocalPathToDB update failed error" << res << m_updateHashByLocalPathPS.lastError();
    }

    return m_updateHashByLocalPathPS.numRowsAffected();
}

int CloudDriveModel::deleteItemToDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    bool res;

    QString itemCacheKey = getItemCacheKey(type, uid, localPath);
    m_deletePS.bindValue(":type", type);
    m_deletePS.bindValue(":uid", uid);
    m_deletePS.bindValue(":local_path", localPath);
    res = m_deletePS.exec();
    if (res) {
        qDebug() << "CloudDriveModel::deleteItemToDB delete done" << itemCacheKey << "res" << res << "numRowsAffected" << m_deletePS.numRowsAffected();
        m_db.commit();

        // Remove item from itemCache.
        m_itemCache->remove(itemCacheKey);
    } else {
        qDebug() << "CloudDriveModel::deleteItemToDB delete failed" << itemCacheKey << "res" << res << m_deletePS.lastError();
    }

    return m_deletePS.numRowsAffected();
}

int CloudDriveModel::countItemDB()
{
    int c = 0;
    bool res = m_countPS.exec();
    if (res && m_countPS.next()) {
        c = m_countPS.value(m_countPS.record().indexOf("count")).toInt();
    }

    return c;
}

int CloudDriveModel::countItemByLocalPathDB(const QString localPath)
{
    // TODO Cache.
    int c = 0;

    m_countByLocalPathPS.bindValue(":local_path", localPath);
    bool res = m_countByLocalPathPS.exec();
    if (res && m_countByLocalPathPS.next()) {
        c = m_countByLocalPathPS.value(m_countByLocalPathPS.record().indexOf("count")).toInt();
    } else {
        qDebug() << "CloudDriveModel::countItemByLocalPathDB count failed" << m_countByLocalPathPS.lastError();
    }

    return c;
}

int CloudDriveModel::countItemByTypeAndUidAndRemotePathFromDB(CloudDriveModel::ClientTypes type, QString uid, QString remotePath)
{
    // TODO Cache.
    int c = 0;

    QSqlQuery qry(m_db);
    qry.prepare("SELECT count(*) count FROM cloud_drive_item where type = :type AND uid = :uid AND remote_path = :remote_path");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remotePath);
    bool res = qry.exec();
    if (res && qry.next()) {
        c = qry.value(qry.record().indexOf("count")).toInt();
    } else {
        qDebug() << "CloudDriveModel::countItemByTypeAndUidAndRemotePathFromDB count failed" << qry.lastError();
    }

    return c;
}

void CloudDriveModel::uploadProgressFilter(QString nonce, qint64 bytesSent, qint64 bytesTotal)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    job.bytes = bytesSent;
    job.bytesTotal = bytesTotal;
    m_cloudDriveJobs->insert(nonce, job);

    emit uploadProgress(nonce, bytesSent, bytesTotal);
}

void CloudDriveModel::downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    job.bytes = bytesReceived;
    job.bytesTotal = bytesTotal;
    m_cloudDriveJobs->insert(nonce, job);

    emit downloadProgress(nonce, bytesReceived, bytesTotal);
}

void CloudDriveModel::jobDone() {
    mutex.lock();
    runningJobCount--;
    mutex.unlock();

    qDebug() << "CloudDriveModel::jobDone runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue->count() << "m_cloudDriveJobs" << m_cloudDriveJobs->count();

    // Refresh if no more running jobs and jobs in queue.
    if (runningJobCount <= 0 && m_jobQueue->count() <= 0) {
        emit refreshRequestSignal();
    }

    emit jobQueueStatusSignal(runningJobCount, m_jobQueue->count(), getItemCount());
    emit proceedNextJobSignal();
}

void CloudDriveModel::proceedNextJob() {
    // TODO delay start by process events.
    QApplication::processEvents();

    if (m_thread.isRunning()) return;

    // Proceed next job in queue. Any jobs which haven't queued will be ignored.
    qDebug() << "CloudDriveModel::proceedNextJob waiting runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue->count() << "m_cloudDriveJobs" << m_cloudDriveJobs->count();
    if (runningJobCount >= MaxRunningJobCount || m_jobQueue->count() <= 0 || m_isSuspended) {
        return;
    }

    QString nonce = m_jobQueue->dequeue();
    qDebug() << "CloudDriveModel::proceedNextJob jobId " << nonce;
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    mutex.lock();
    runningJobCount++;
    mutex.unlock();

    // Dispatch job.
    dispatchJob(job);

    emit proceedNextJobSignal();
}

void CloudDriveModel::dispatchJob(const CloudDriveJob job)
{
    // TODO
    // If job.type==Dropbox, call DropboxClient.run() directly.
    // If job.type==Any, start thread.

    // TODO Generalize cloud client.
    CloudDriveClient *cloudClient;
    switch (job.type) {
    case Dropbox:
        cloudClient = dbClient;
        break;
    case SkyDrive:
        cloudClient = skdClient;
        break;
    case Ftp:
        cloudClient = ftpClient;
        break;
    }

    switch (job.operation) {
    case LoadCloudDriveItems:
        // Execute logic asynchronously.
        // Configure thread.
        m_thread.setHashFilePath(ITEM_DAT_PATH);
        m_thread.setNonce(job.jobId);
        m_thread.setRunMethod(job.operation);
        m_thread.setCloudDriveItems(m_cloudDriveItems);
        m_thread.start();
        break;
    case InitializeDB:
        // Execute initialize locally.
        initializeDB(job.jobId);
        break;
    case FileGet:
        cloudClient->fileGet(job.jobId, job.uid, job.remoteFilePath, job.localFilePath);
        break;
    case FilePut:
        cloudClient->filePut(job.jobId, job.uid, job.localFilePath, job.remoteFilePath);
        break;
    case Metadata:
        cloudClient->metadata(job.jobId, job.uid, job.remoteFilePath);
        break;
    case Browse:
        cloudClient->browse(job.jobId, job.uid, job.remoteFilePath);
        break;
    case RequestToken:
        cloudClient->requestToken(job.jobId);
        break;
    case Authorize:
        cloudClient->authorize(job.jobId);
        break;
    case AccessToken:
        cloudClient->accessToken(job.jobId, accessTokenPin);
        break;
    case RefreshToken:
        cloudClient->refreshToken(job.jobId, job.uid);
        break;
    case AccountInfo:
        cloudClient->accountInfo(job.jobId, job.uid);
        break;
    case Quota:
        cloudClient->quota(job.jobId, job.uid);
        break;
    case CreateFolder:
        cloudClient->createFolder(job.jobId, job.uid, job.localFilePath, job.remoteFilePath);
        break;
    case MoveFile:
        cloudClient->moveFile(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFilePath);
        break;
    case CopyFile:
        cloudClient->copyFile(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFilePath);
        break;
    case DeleteFile:
        cloudClient->deleteFile(job.jobId, job.uid, job.remoteFilePath);
        break;
    case ShareFile:
        cloudClient->shareFile(job.jobId, job.uid, job.remoteFilePath);
        break;
    }
}

void CloudDriveModel::threadFinishedFilter()
{
    // Notify job done.
    jobDone();
}

void CloudDriveModel::loadCloudDriveItemsFilter(QString nonce)
{
    // Copy thread map to member map.
//    m_cloudDriveItems = m_thread.getCloudDriveItems();
//    m_thread.getCloudDriveItems().clear();

    // Notify job done is done by threadFinishedFilter.

    // RemoveJob
    removeJob(nonce);

    emit loadCloudDriveItemsFinished(nonce);
}

void CloudDriveModel::requestTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit requestTokenReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::authorizeRedirectFilter(QString nonce, QString url, QString redirectFrom)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit authorizeRedirectSignal(nonce, url, redirectFrom);
}

void CloudDriveModel::accessTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    // Get accountInfo.
    if (err == QNetworkReply::NoError) {
        QHash<QString, QString> m_paramMap;

        switch (job.type) {
        case Dropbox:
            // Dropbox provides uid but no email yet. Get email from accountInfo.
            foreach (QString s, msg.split('&')) {
                QStringList c = s.split('=');
                m_paramMap[c.at(0)] = c.at(1);
            }
            qDebug() << "CloudDriveModel::accessTokenReplyFilter Dropbox uid" << m_paramMap["uid"];

            accountInfo(Dropbox, m_paramMap["uid"]);
            break;
        case SkyDrive:
            // SkyDrive's accessTokenReply doesn't provide uid yet. Continue to get account.
            accountInfo(SkyDrive, job.uid);
            break;
        }
    }

    emit accessTokenReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::accountInfoReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit accountInfoReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::quotaReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    m_cloudDriveJobs->insert(nonce, job);

    // Notify job done.
    jobDone();

    emit quotaReplySignal(nonce, err, errMsg, msg);
}
