#include "clouddrivemodel.h"

bool nameLessThan(const QScriptValue &o1, const QScriptValue &o2)
{
//    qDebug() << "nameLessThan" << o1.property("name").toString().toLower() << o2.property("name").toString().toLower();
    return o1.property("name").toString().toLower() < o2.property("name").toString().toLower();
}

bool nameLessThanWithDirectoryFirst(const QScriptValue &o1, const QScriptValue &o2)
{
    if (o1.property("isDir").toBool() && !o2.property("isDir").toBool()) {
        // If dir before file, return true.
        return true;
    } else if (!o1.property("isDir").toBool() && o2.property("isDir").toBool()) {
        // If file before dir, return false;
        return false;
    } else {
        // If both are dir/file, compare name.
        return o1.property("name").toString().toLower() < o2.property("name").toString().toLower();
    }
}

bool timeGreaterThan(const QScriptValue &o1, const QScriptValue &o2)
{
    qDebug() << "timeGreaterThan" << o1.property("lastModified").toString() << o2.property("lastModified").toString();
    return o1.property("lastModified").toString()  > o2.property("lastModified").toString();
}

bool typeLessThan(const QScriptValue &o1, const QScriptValue &o2)
{
    if (o1.property("isDir").toBool() && !o2.property("isDir").toBool()) {
        // If dir before file, return true.
        return true;
    } else if (!o1.property("isDir").toBool() && o2.property("isDir").toBool()) {
        // If file before dir, return false;
        return false;
    } else {
        // If both are dir/file, compare type then name.
        if (o1.property("fileType").toString().toLower() == o2.property("fileType").toString().toLower()) {
            return o1.property("name").toString().toLower() < o2.property("name").toString().toLower();
        } else if (o1.property("fileType").toString().toLower() < o2.property("fileType").toString().toLower()) {
            return true;
        } else {
            return false;
        }
    }
}

bool sizeGreaterThan(const QScriptValue &o1, const QScriptValue &o2)
{
    if (o1.property("isDir").toBool() && !o2.property("isDir").toBool()) {
        // If dir before file, return true.
        return true;
    } else if (!o1.property("isDir").toBool() && o2.property("isDir").toBool()) {
        // If file before dir, return false;
        return false;
    } else {
        // If both the same, compare size.
        return o1.property("size").toInteger() > o2.property("size").toInteger();
    }
}

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString CloudDriveModel::ITEM_DAT_PATH = "/home/user/.filesplus/CloudDriveModel.dat";
const QString CloudDriveModel::ITEM_DB_PATH = "/home/user/.filesplus/CloudDriveModel.db";
const QString CloudDriveModel::ITEM_DB_CONNECTION_NAME = "cloud_drive_model";
const QString CloudDriveModel::TEMP_PATH = "/home/user/MyDocs/temp/.filesplus";
const QString CloudDriveModel::JOB_DAT_PATH = "/home/user/.filesplus/CloudDriveJobs.dat";
const int CloudDriveModel::MaxRunningJobCount = 1;
#else
const QString CloudDriveModel::ITEM_DAT_PATH = "CloudDriveModel.dat";
const QString CloudDriveModel::ITEM_DB_PATH = "CloudDriveModel.db";
const QString CloudDriveModel::ITEM_DB_CONNECTION_NAME = "cloud_drive_model";
const QString CloudDriveModel::TEMP_PATH = "E:/temp/.filesplus";
const QString CloudDriveModel::JOB_DAT_PATH = "CloudDriveJobs.dat"; // It's in private folder.
const int CloudDriveModel::MaxRunningJobCount = 1;
#endif
const QString CloudDriveModel::DirtyHash = "FFFFFFFF";
//const QStringList CloudDriveModel::restrictFileTypes = QString("SIS,SISX,DEB").split(",");
const QStringList CloudDriveModel::restrictFileTypes;

CloudDriveModel::CloudDriveModel(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    // Issue: Qt on Belle doesn't support QtConcurrent and QFuture.
    // Example code: QFuture<void> loadDataFuture = QtConcurrent::run(this, &CloudDriveModel::loadCloudDriveItems);

    // Initialize DAT and jobQueue.
    m_cloudDriveItems = new QMultiMap<QString, CloudDriveItem>();
    m_cloudDriveJobs = new QHash<QString, CloudDriveJob>();
    m_jobQueue = new QQueue<QString>();

    // Create temp path.
    createTempPath();

    // Set thread pool with MaxRunningJobCount+1 to support browse method.
    QThreadPool::globalInstance()->setMaxThreadCount(MaxRunningJobCount + 1);

    // Initialize scheduler queue.
    initScheduler();
    m_scheduledItems = new QQueue<CloudDriveItem>();

    // Enqueue initialization jobs. Queued jobs will proceed after foldePage is loaded.
    CloudDriveJob loadCloudDriveItemsJob(createNonce(), LoadCloudDriveItems, -1, "", "", "", -1);
    m_cloudDriveJobs->insert(loadCloudDriveItemsJob.jobId, loadCloudDriveItemsJob);
    m_jobQueue->enqueue(loadCloudDriveItemsJob.jobId);
    CloudDriveJob loadCloudDriveJobsJob(createNonce(), LoadCloudDriveJobs, -1, "", "", "", -1);
    m_cloudDriveJobs->insert(loadCloudDriveJobsJob.jobId, loadCloudDriveJobsJob);
    m_jobQueue->enqueue(loadCloudDriveJobsJob.jobId);
    CloudDriveJob initializeDBJob(createNonce(), InitializeDB, -1, "", "", "", -1);
    m_cloudDriveJobs->insert(initializeDBJob.jobId, initializeDBJob);
    m_jobQueue->enqueue(initializeDBJob.jobId);

    // Initialize itemCache.
    m_itemCache = new QHash<QString, CloudDriveItem>();
    m_isConnectedCache = new QHash<QString, bool>();
    m_isDirtyCache = new QHash<QString, bool>();
    m_isSyncingCache = new QHash<QString, bool>();

    // Initialize cloud storage clients.
    initializeCloudClients(createNonce());

    // Initialize job queue timer.
    initJobQueueTimer();
}

CloudDriveModel::~CloudDriveModel()
{
    // TODO Migrate DAT to DB.
    saveCloudDriveItems();

    // Save current jobs.
    saveCloudDriveJobs();

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

void CloudDriveModel::loadCloudDriveItems(QString nonce) {
    QFile file(ITEM_DAT_PATH);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> *m_cloudDriveItems;
    }

    qDebug() << QTime::currentTime() << "CloudDriveModel::loadCloudDriveItems " << m_cloudDriveItems->size();

    // Notify job done.
    jobDone();

    // RemoveJob
    removeJob("CloudDriveModel::loadCloudDriveItems", nonce);

    emit loadCloudDriveItemsFinished(nonce);
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

void CloudDriveModel::loadCloudDriveJobs(QString nonce)
{
    QScriptEngine engine;
    QScriptValue sc;
    int i = 0;

    QFile file(JOB_DAT_PATH);
    if (file.open(QIODevice::ReadOnly)) {
        sc = engine.evaluate(QString::fromUtf8(file.readAll()));
        if (sc.isArray()) {
            int len = sc.toVariant().toList().length();
            for (i = 0; i < len; i++) {
                QScriptValue item = sc.property(i);
                CloudDriveJob job(item.property("job_id").toString(),
                                  item.property("operation").toInt32(),
                                  item.property("type").toInt32(),
                                  item.property("uid").toString(),
                                  item.property("local_file_path").toString(),
                                  item.property("remote_file_path").toString(), -1);
                job.newLocalFilePath = item.property("new_local_file_path").toString();
                job.newRemoteFilePath = item.property("new_remote_file_path").toString();
                job.newRemoteFileName = item.property("new_remote_file_name").toString();
                job.targetUid = item.property("target_uid").toString();
                job.targetType = item.property("target_type").toInt32();
                job.bytes = item.property("bytes").toInt32();
                job.bytesTotal = item.property("bytes_total").toInt32();
                job.forcePut = item.property("force_put").toBool();
                job.downloadOffset = item.property("download_offset").toInt32();
                job.uploadId = item.property("upload_id").toString();
                job.uploadOffset = item.property("upload_offset").toInt32();
                job.createdTime = item.property("created_time").toDateTime();
                job.lastStartedTime = item.property("last_started_time").toDateTime();
                job.lastStoppedTime = item.property("last_stopped_time").toDateTime();
                job.err = item.property("err").toInt32();
                job.errString = item.property("err_string").toString();
                job.errMessage = item.property("err_message").toString();
                job.nextJobId = item.property("next_job_id").toString();
                m_cloudDriveJobs->insert(job.jobId, job);

                // Add job to jobModel for displaying on job page.
                emit jobEnqueuedSignal(job.jobId, job.localFilePath);
            }
        }
    }

    qDebug() << "CloudDriveModel::loadCloudDriveJobs " << i;

    // Notify job done.
    jobDone();

    // RemoveJob
    removeJob("CloudDriveModel::loadCloudDriveJobs", nonce);
}

void CloudDriveModel::saveCloudDriveJobs()
{
    QFile file(JOB_DAT_PATH);
    QFileInfo info(file);
    if (!info.absoluteDir().exists()) {
        qDebug() << "CloudDriveModel::saveCloudDriveJobs dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(info.absolutePath());
        if (!res) {
            qDebug() << "CloudDriveModel::saveCloudDriveJobs can't make dir" << info.absolutePath();
        } else {
            qDebug() << "CloudDriveModel::saveCloudDriveJobs make dir" << info.absolutePath();
        }
    }

    if (file.open(QIODevice::WriteOnly)) {
        file.write("[ ");
        int c = 0;
        foreach (CloudDriveJob job, m_cloudDriveJobs->values()) {
            // Clean up job.
            if (job.jobId == "") continue;

            // Include only file transferring jobs.
            switch (job.operation) {
            case FileGet:
            case FilePut:
            case MigrateFile:
            case MigrateFilePut:
            case SyncFromLocal:
            case FilePutResume:
            case FilePutCommit:
            case FileGetResume:
            case FileGetCommit:
                if (c > 0) file.write(", ");
                file.write(job.toJsonText().toUtf8());
                c++;
                break;
            default:
                qDebug() << "CloudDriveModel::saveCloudDriveJobs skip jobId" << job.jobId << getCloudName(job.type) << getOperationName(job.operation);
            }
        }
        file.write(" ]");
    }
    file.close();

    qDebug() << "CloudDriveModel::saveCloudDriveJobs" << m_cloudDriveJobs->size() << "fileSize" << file.size();
}

void CloudDriveModel::initializeCloudClients(QString nonce)
{
    // TODO Generalize to support plugable client.
    initializeDropboxClient();
    initializeSkyDriveClient();
    initializeGoogleDriveClient();
    initializeFtpClient();
    initializeWebDAVClient();
}

void CloudDriveModel::connectCloudClientsSignal(CloudDriveClient *client)
{
    connect(client, SIGNAL(uploadProgress(QString,qint64,qint64)), SLOT(uploadProgressFilter(QString,qint64,qint64)) );
    connect(client, SIGNAL(downloadProgress(QString,qint64,qint64)), SLOT(downloadProgressFilter(QString,qint64,qint64)) );
    connect(client, SIGNAL(requestTokenReplySignal(QString,int,QString,QString)), SLOT(requestTokenReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(authorizeRedirectSignal(QString,QString,QString)), SLOT(authorizeRedirectFilter(QString,QString,QString)) );
    connect(client, SIGNAL(accessTokenReplySignal(QString,int,QString,QString)), SLOT(accessTokenReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(accountInfoReplySignal(QString,int,QString,QString)), SLOT(accountInfoReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(quotaReplySignal(QString,int,QString,QString,qint64,qint64,qint64)), SLOT(quotaReplyFilter(QString,int,QString,QString,qint64,qint64,qint64)) );
    connect(client, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SLOT(fileGetReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SLOT(filePutReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SLOT(metadataReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(browseReplySignal(QString,int,QString,QString)), SLOT(browseReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SLOT(createFolderReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(moveFileReplySignal(QString,int,QString,QString)), SLOT(moveFileReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(copyFileReplySignal(QString,int,QString,QString)), SLOT(copyFileReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(deleteFileReplySignal(QString,int,QString,QString)), SLOT(deleteFileReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(shareFileReplySignal(QString,int,QString,QString,QString,int)), SLOT(shareFileReplyFilter(QString,int,QString,QString,QString,int)) );
    connect(client, SIGNAL(migrateFilePutReplySignal(QString,int,QString,QString)), SLOT(migrateFilePutFilter(QString,int,QString,QString)) ); // Added because FtpClient doesn't provide QNetworkReply.
    connect(client, SIGNAL(fileGetResumeReplySignal(QString,int,QString,QString)), SLOT(fileGetResumeReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(filePutResumeReplySignal(QString,int,QString,QString)), SLOT(filePutResumeReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(deltaReplySignal(QString,int,QString,QString)), SLOT(deltaReplyFilter(QString,int,QString,QString)) );
}

void CloudDriveModel::initializeDropboxClient() {
    qDebug() << "CloudDriveModel::initializeDropboxClient" << m_dropboxFullAccess;

    if (dbClient != 0) {
        dbClient->deleteLater();
    }

    dbClient = new DropboxClient(this, m_dropboxFullAccess);
    connectCloudClientsSignal(dbClient);
}

void CloudDriveModel::initializeSkyDriveClient()
{
    qDebug() << "CloudDriveModel::initializeSkyDriveClient";

    if (skdClient != 0) {
        skdClient->deleteLater();
    }

    skdClient = new SkyDriveClient(this);
    connectCloudClientsSignal(skdClient);
}

void CloudDriveModel::initializeGoogleDriveClient()
{
    qDebug() << "CloudDriveModel::initializeGoogleDriveClient";

    if (gcdClient != 0) {
        gcdClient->deleteLater();
    }

    gcdClient = new GCDClient(this);
    connectCloudClientsSignal(gcdClient);
}

void CloudDriveModel::initializeFtpClient()
{
    qDebug() << "CloudDriveModel::initializeFtpClient";

    if (ftpClient != 0) {
        ftpClient->deleteLater();
    }

    ftpClient = new FtpClient(this);
    connectCloudClientsSignal(ftpClient);
}

void CloudDriveModel::initializeWebDAVClient()
{
    // TODO Generalize initialization.
    qDebug() << "CloudDriveModel::initializeWebDAVClient";

    if (webDavClient != 0) {
        webDavClient->deleteLater();
    }

    webDavClient = new WebDavClient(this);
    connectCloudClientsSignal(webDavClient);
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
//        qDebug() << "CloudDriveModel::findItemWithChildren" << item.localPath << item.type << item.uid;
        if (item.type == type && item.uid == uid) {
            if (item.localPath == localPath || item.localPath.startsWith(localPath + "/")) {
                qDebug() << "CloudDriveModel::findItemWithChildren add" << item;
                list.append(item);
                isFound = true;
            } else if (isFound) {
                // End of sub tree in same type, uid.
                break;
            }
        }

        // TODO (Realtime Migration) Insert to DB.
        if (insertItemToDB(item, true) > 0) {
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

QList<CloudDriveItem> CloudDriveModel::findItemsByRemotePath(ClientTypes type, QString uid, QString remotePath, bool caseInsensitive)
{
    QSqlQuery qry(m_db);
    if (caseInsensitive) {
        qry.prepare("SELECT * FROM cloud_drive_item where type = :type AND uid = :uid AND lower(remote_path) = lower(:remote_path)");
    } else {
        qry.prepare("SELECT * FROM cloud_drive_item where type = :type AND uid = :uid AND remote_path = :remote_path");
    }
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remotePath);

    return getItemListFromPS(qry);
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
//        qDebug() << "CloudDriveModel::isConnected cached localPath" << localPath << m_isConnectedCache->value(localPath);
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
        bool isDirty = m_isDirtyCache->value(localPath);
//        qDebug() << "CloudDriveModel::isDirty cached localPath" << localPath << "file.lastModified" << lastModified << "isDirty" << isDirty;
        return isDirty;
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

    if (res) {
        qDebug() << "CloudDriveModel::isDirty localPath" << localPath << "file.lastModified" << lastModified << "isDirty" << res;
    }
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

bool CloudDriveModel::isSyncing(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    bool res = false;
    foreach (CloudDriveJob job, m_cloudDriveJobs->values()) {
        if (job.localFilePath == localPath && getClientType(job.type) == type && job.uid == uid) {
            res = true;
        }
    }

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

void CloudDriveModel::clearConnectedRemoteDirtyCache(QString localPath)
{
    m_isConnectedCache->remove(localPath);
    m_isDirtyCache->remove(localPath);
    m_isSyncingCache->remove(localPath);
}

bool CloudDriveModel::isRemoteRoot(CloudDriveModel::ClientTypes type, QString uid, QString remotePath)
{
    CloudDriveClient *client = getCloudClient(type);
    if (client == 0) {
        return false;
    } else {
        return (client->getRemoteRoot(uid) == remotePath || remotePath == "" || remotePath == "/");
    }
}

QString CloudDriveModel::getRemoteRoot(CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveClient *client = getCloudClient(type);
    if (client == 0) {
        return "";
    } else {
        return client->getRemoteRoot(uid);
    }
}

QString CloudDriveModel::getParentRemotePath(CloudDriveModel::ClientTypes type, QString remotePath)
{
    if (isRemoteAbsolutePath(type)) {
        QString remoteParentPath = "";
        if (remotePath != "" && remotePath != "/") {
            // Remove trailing slash from WebDavClient remote folder path.
            remotePath = remotePath.endsWith("/") ? remotePath.mid(0, remotePath.lastIndexOf("/")) : remotePath;
            // Remove item name to get parent path.
            remoteParentPath = remotePath.mid(0, remotePath.lastIndexOf("/"));
            remoteParentPath = (remoteParentPath == "") ? "/" : remoteParentPath;
        }
        return remoteParentPath;
    } else {
        return "";
    }
}

QString CloudDriveModel::getRemoteName(CloudDriveModel::ClientTypes type, QString remotePath) {
    if (isRemoteAbsolutePath(type) && remotePath != "") {
        return remotePath.mid(remotePath.lastIndexOf("/") + 1);
    } else {
        return remotePath;
    }
}

QString CloudDriveModel::getRemotePath(CloudDriveModel::ClientTypes type, QString remoteParentPath, QString remotePathName) {
    if (isRemoteAbsolutePath(type)) {
        return (remoteParentPath == "/" ? "" : remoteParentPath) + "/" + remotePathName;
    } else {
        return remoteParentPath;
    }
}

QString CloudDriveModel::getParentLocalPath(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.absolutePath();
}

bool CloudDriveModel::isDir(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.isDir();
}

bool CloudDriveModel::isFile(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.isFile();
}

QString CloudDriveModel::getAbsolutePath(const QString dirPath, const QString fileName)
{
    QDir dir(dirPath);

    return dir.absoluteFilePath(fileName);
}

bool CloudDriveModel::createDirPath(const QString absPath)
{
    qDebug() << "CloudDriveModel::createDirPath" << absPath;

    if (absPath.trimmed().isEmpty()) return false;

    QDir dir(getParentLocalPath(absPath));
    bool res = dir.mkdir(getFileName(absPath));

    return res;
}

QString CloudDriveModel::getFileName(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.fileName();
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

qint64 CloudDriveModel::getFileSize(QString localPath)
{
    QFileInfo fileInfo(localPath);
    return fileInfo.size();
}

QString CloudDriveModel::getFileLastModified(QString localPath)
{
    QFileInfo fileInfo(localPath);
    QString datetimeString = fileInfo.lastModified().toString();
    qDebug() << "CloudDriveModel::getFileLastModified localPath" << localPath << datetimeString;
    return datetimeString;
}

CloudDriveModel::ClientTypes CloudDriveModel::getClientType(int typeInt)
{
//    qDebug() << "CloudDriveModel::getClientType" << typeInt;

    switch (typeInt) {
    case Dropbox:
        return Dropbox;
    case SkyDrive:
        return SkyDrive;
    case GoogleDrive:
        return GoogleDrive;
    case Ftp:
        return Ftp;
    case WebDAV:
        return WebDAV;
    }
}

CloudDriveModel::ClientTypes CloudDriveModel::getClientType(QString typeText)
{
//    qDebug() << "CloudDriveModel::getClientType" << typeText;

    if (typeText.indexOf(QRegExp("dropboxclient|dropbox", Qt::CaseInsensitive)) != -1) {
        return Dropbox;
    } else if (typeText.indexOf(QRegExp("skydriveclient|skydrive", Qt::CaseInsensitive)) != -1) {
        return SkyDrive;
    } else if (typeText.indexOf(QRegExp("gcdclient|googledriveclient|googledrive", Qt::CaseInsensitive)) != -1) {
        return GoogleDrive;
    } else if (typeText.indexOf(QRegExp("ftpclient|ftp", Qt::CaseInsensitive)) != -1) {
        return Ftp;
    } else if (typeText.indexOf(QRegExp("webdavclient|webdav", Qt::CaseInsensitive)) != -1) {
        return WebDAV;
    }
}

QString CloudDriveModel::getCloudName(int type) {
    switch (type) {
    case Dropbox:
        return "Dropbox";
    case GoogleDrive:
        return "GoogleDrive";
    case SkyDrive:
        return "SkyDrive";
    case Ftp:
        return "FTP";
    case WebDAV:
        return "WebDAV";
    default:
        return QString("Invalid type(%1)").arg(type);
    }
}

QString CloudDriveModel::getOperationName(int operation) {
    switch (operation) {
    case FileGet:
        return tr("Download");
    case FilePut:
        return tr("Upload");
    case Metadata:
        return tr("Sync");
    case CreateFolder:
        return tr("Create folder");
    case RequestToken:
        return tr("Request token");
    case Authorize:
        return tr("Authorize");
    case AccessToken:
        return tr("Access token");
    case RefreshToken:
        return tr("Refresh token");
    case AccountInfo:
        return tr("Account Info");
    case Quota:
        return tr("Quota");
    case DeleteFile:
        return tr("Delete");
    case MoveFile:
        return tr("Move");
    case CopyFile:
        return tr("Copy");
    case ShareFile:
        return tr("Share link");
    case Delta:
        return tr("Delta");
    case Browse:
        return tr("Browse");
    case LoadCloudDriveItems:
        return tr("LoadCloudDriveItems");
    case LoadCloudDriveJobs:
        return tr("LoadCloudDriveJobs");
    case InitializeDB:
        return tr("InitializeDB");
    case InitializeCloudClients:
        return tr("InitializeCloudClients");
    case Disconnect:
        return tr("Disconnect");
    case ScheduleSync:
        return tr("ScheduleSync");
    case MigrateFile:
        return tr("Migrate");
    case MigrateFilePut:
        return tr("Migrate file");
    case SyncFromLocal:
        return tr("Sync from local");
    case FilePutResume:
        return tr("Upload");
    case FilePutCommit:
        return tr("Commit upload");
    case FileGetResume:
        return tr("Download");
    default:
        return "invalid";
    }
}

QDateTime CloudDriveModel::parseReplyDateString(CloudDriveModel::ClientTypes type, QString dateString)
{
    return getCloudClient(type)->parseReplyDateString(dateString); //.toLocalTime();
}

QString CloudDriveModel::formatJSONDateString(QDateTime datetime)
{
    // Format to JSON which match with javascript, QML format.
    return datetime.toString("yyyy-MM-ddThh:mm:ss.zzzZ");
}

bool CloudDriveModel::isRemoteAbsolutePath(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isRemoteAbsolutePath();
}

bool CloudDriveModel::isRemotePathCaseInsensitive(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isRemotePathCaseInsensitive();
}

bool CloudDriveModel::isConfigurable(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isConfigurable();
}

bool CloudDriveModel::isViewable(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isViewable();
}

bool CloudDriveModel::isImageUrlCachable(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isImageUrlCachable();
}

bool CloudDriveModel::isUnicodeSupported(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isUnicodeSupported();
}

void CloudDriveModel::initScheduler()
{
    connect(&m_schedulerTimer, SIGNAL(timeout()), this, SLOT(schedulerTimeoutFilter()) );
    m_schedulerTimer.setInterval(60000);
    m_schedulerTimer.start();
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

void CloudDriveModel::updateJob(CloudDriveJob job, bool emitJobUpdatedSignal)
{
    mutex.lock();
    // Remove cache for furthur refresh.
    clearConnectedRemoteDirtyCache(job.localFilePath);

    if (job.isRunning) {
        job.lastStartedTime = QDateTime::currentDateTime();
    } else {
        job.lastStoppedTime = QDateTime::currentDateTime();
    }
    m_cloudDriveJobs->insert(job.jobId, job);
    mutex.unlock();

    if (emitJobUpdatedSignal) emit jobUpdatedSignal(job.jobId);

//    qDebug() << "CloudDriveModel::updateJob job" << job.toJsonText();
}

void CloudDriveModel::removeJob(QString caller, QString nonce)
{
    if (nonce == "") {
        qDebug() << "CloudDriveModel::removeJob caller" << caller << "nonce" << nonce << "jobId is empty. Operation is aborted";
        return;
    }

    // Abort job.
    suspendJob(nonce);

    mutex.lock();
    QString localPath = m_cloudDriveJobs->value(nonce).localFilePath;
    clearConnectedRemoteDirtyCache(localPath);
    int removeCount = m_cloudDriveJobs->remove(nonce);
    mutex.unlock();

    // Remove temp file if it exists.
    QString tempFilePath = m_settings.value("temp.path", TEMP_PATH).toString() + "/" + nonce;
    if (QFileInfo(tempFilePath).exists()) {
        QFile(tempFilePath).remove();
        qDebug() << "CloudDriveModel::removeJob caller" << caller << "temp file" << tempFilePath << "is removed";
    }

    if (removeCount > 0) {
        emit jobRemovedSignal(nonce);
    }

    qDebug() << "CloudDriveModel::removeJob caller" << caller << "nonce" << nonce << "removeCount" << removeCount << "m_cloudDriveJobs->count()" << m_cloudDriveJobs->count();
}

int CloudDriveModel::getQueuedJobCount() const
{
    return m_jobQueue->count();
}

int CloudDriveModel::getRunningJobCount() const
{
    return runningJobCount;
}

int CloudDriveModel::getJobCount() const
{
    return m_cloudDriveJobs->count();
}

void CloudDriveModel::cancelQueuedJobs()
{
    while (!m_jobQueue->isEmpty()) {
        CloudDriveJob job = m_cloudDriveJobs->value(m_jobQueue->dequeue());
        if (!job.isRunning) {
            removeJob("CloudDriveModel::cancelQueuedJobs", job.jobId);
        }
    }

    qDebug() << "CloudDriveModel::cancelQueuedJobs done m_jobQueue->count()" << m_jobQueue->count() << "m_cloudDriveJobs->count()" << m_cloudDriveJobs->count();
}

void CloudDriveModel::removeJobs(bool removeAll)
{
    // Remove unqueued jobs.
    foreach (CloudDriveJob job, m_cloudDriveJobs->values()) {
        if (removeAll || !job.isRunning) {
            removeJob("CloudDriveModel::removeJobs", job.jobId);
        }
    }
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
    clearConnectedRemoteDirtyCache(localPath);

    qDebug() << "CloudDriveModel::removeItem item" << item << "removeCount" << removeCount << "deleteCount" << deleteCount;
}

void CloudDriveModel::removeItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    if (localPath == "") return;

    int removeCount = 0;
    int deleteCount = 0;

    foreach (CloudDriveItem item, findItemWithChildren(type, uid, localPath)) {
//        qDebug() << "CloudDriveModel::removeItemWithChildren item" << item;

        // Process events to avoid freezing UI.
        QApplication::processEvents(QEventLoop::AllEvents, 50);

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
    m_cloudDriveItems->remove(item.localPath, item); // Remove found item.
    item.hash = hash;
    m_cloudDriveItems->insert(item.localPath, item); // Insert updates item.
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

void CloudDriveModel::updateItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString newLocalPath, QString newRemotePath, QString newParentHash, QString newChildrenHash)
{
    qDebug() << "CloudDriveModel::updateItemWithChildren localPath" << localPath << "remotePath" << remotePath << "newLocalPath" << newLocalPath << "newRemotePath" << newRemotePath << "newParentHash" << newParentHash << "newChildrenHash" << newChildrenHash;

    int updateCount = 0;

    foreach (CloudDriveItem item, findItemWithChildren(type, uid, localPath)) {
        qDebug() << "CloudDriveModel::updateItemWithChildren before item" << item;
        if (item.localPath == localPath) {
            // Set hash for parent.
            // Set hash = DirtyHash to hint syncFromLocal to put files.
            // Set hash = empty to hint metadata to get files, otherwise syncFromLocal to put local files which are remained with empty hash.
            if (newParentHash != "") {
                item.hash = newParentHash;
            }
        } else if (newChildrenHash != "") {
            // Set hash for children.
            item.hash = newChildrenHash;
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

void CloudDriveModel::updateItemWithChildrenByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath, QString newRemotePath, QString newParentHash, QString newChildrenHash)
{
    foreach (CloudDriveItem item, findItemsByRemotePath(type, uid, remotePath)) {
        qDebug() << "CloudDriveModel::updateItemWithChildrenByRemotePath item" << item.localPath << "remotePath" << item.remotePath << "newRemotePath" << newRemotePath << "newParentHash" << newParentHash << "newChildrenHash" << newChildrenHash;
        updateItemWithChildren(type, uid, item.localPath, item.remotePath, item.localPath, newRemotePath, newParentHash, newChildrenHash);
    }
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
        qDebug() << "CloudDriveModel::syncScheduledItems dequeue and sync item" << item;
        metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1);
    }
}

bool CloudDriveModel::isDeltaSupported(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isDeltaSupported();
}

bool CloudDriveModel::isDeltaEnabled(CloudDriveModel::ClientTypes type, QString uid)
{
    return getCloudClient(type)->isDeltaEnabled(uid);
}

void CloudDriveModel::scheduleDeltaJobs(QString cronValue)
{
    QScriptEngine engine;
    foreach (QString uidJson, getStoredUidList()) {
        QScriptValue sc = engine.evaluate("(" + uidJson + ")");
        QString typeText = sc.property("type").toString();
        QString uid = sc.property("uid").toString();
        if (isDeltaSupported(getClientType(typeText)) && isDeltaEnabled(getClientType(typeText), uid)) {
            QString deltaCronExp = getDeltaCronExp(getClientType(typeText), uid);
            if (matchCronExp(deltaCronExp, cronValue)) {
                delta(getClientType(typeText), uid);
            }
        }
    }
}

void CloudDriveModel::setDeltaCronExp(CloudDriveModel::ClientTypes type, QString uid, QString cronExp)
{
    QString clientObjectName = getCloudClient(type)->objectName();
    m_settings.setValue(QString("%1.%2.deltaCronExp").arg(clientObjectName).arg(uid), QVariant(cronExp));
    m_settings.setValue(QString("%1.%2.delta.enabled").arg(clientObjectName).arg(uid), QVariant(cronExp != ""));
}

QString CloudDriveModel::getDeltaCronExp(CloudDriveModel::ClientTypes type, QString uid)
{
    return m_settings.value(QString("%1.%2.deltaCronExp").arg(getCloudClient(type)->objectName()).arg(uid), QVariant("")).toString();
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
//    qDebug() << "CloudDriveModel::getItemCount db" << dbCount << "dat" << datCount << "total" << (dbCount + datCount);
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

QString CloudDriveModel::getItemListJsonByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath)
{
    QString jsonText;
    foreach (CloudDriveItem item, findItemsByRemotePath(type, uid, remotePath)) {
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
    return dbClient->isAuthorized() || skdClient->isAuthorized() || gcdClient->isAuthorized() || ftpClient->isAuthorized();
}

bool CloudDriveModel::isAuthorized(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isAuthorized();
}

QStringList CloudDriveModel::getStoredUidList()
{
    QStringList uidList;

    uidList.append(dbClient->getStoredUidList());
    uidList.append(gcdClient->getStoredUidList());
    uidList.append(skdClient->getStoredUidList());
    uidList.append(ftpClient->getStoredUidList());
    uidList.append(webDavClient->getStoredUidList());

    return uidList;
}

QStringList CloudDriveModel::getStoredUidList(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->getStoredUidList();
}

QString CloudDriveModel::getStoredUid(CloudDriveModel::ClientTypes type, QString uid)
{
    return getCloudClient(type)->getStoredUid(uid);
}

int CloudDriveModel::removeUid(CloudDriveModel::ClientTypes type, QString uid)
{
    return getCloudClient(type)->removeUid(uid);
}

void CloudDriveModel::requestJobQueueStatus()
{
    emit jobQueueStatusSignal(runningJobCount, m_jobQueue->count(), m_cloudDriveJobs->count(), getItemCount());
}

void CloudDriveModel::suspendNextJob(bool abort)
{
    qDebug() << "CloudDriveModel::suspendNextJob abort" << abort;

    m_isSuspended = true;
    if (abort) {
        m_isAborted = true;
    }
}

void CloudDriveModel::resumeNextJob(bool resetAbort)
{
    qDebug() << "CloudDriveModel::resumeNextJob resetAbort" << resetAbort;

    m_isSuspended = false;
    if (resetAbort) {
        m_isAborted = false;
    }

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
    syncItems(Ftp);

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
        metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1);
    }
}

void CloudDriveModel::syncItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath == localPath) {
        metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1);
    }
}

bool CloudDriveModel::syncItemByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath, QString newHash, bool forcePut, bool forceGet)
{
    QSqlQuery qry(m_db);
    qry.prepare("SELECT * FROM cloud_drive_item where type = :type AND uid = :uid AND remote_path = :remote_path");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remotePath);

    bool res = false;
    foreach (CloudDriveItem item, getItemListFromPS(qry)) {
        if (newHash != "") {
            qDebug() << "CloudDriveModel::syncItemByRemotePath updating hash" << newHash << "to item" << item;
            updateItem(type, uid, item.localPath, newHash);
        }
        metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1, forcePut, forceGet);
        res = true;
    }

    return res;
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
}

void CloudDriveModel::authorize(CloudDriveModel::ClientTypes type)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Authorize, type, "", "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);
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
    // TODO Validate type.

    // Store access token pin temporarily.
    accessTokenPin = pin;

    // Enqueue job.
    CloudDriveJob job(createNonce(), AccessToken, type, "", "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);
}

void CloudDriveModel::refreshToken(CloudDriveModel::ClientTypes type, QString uid, QString nextNonce)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), RefreshToken, type, uid, "", "", -1);
    job.nextJobId = nextNonce;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->insert(0, job.jobId);
}

void CloudDriveModel::accountInfo(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), AccountInfo, type, uid, "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);
}

void CloudDriveModel::quota(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Quota, type, uid, "", "", -1);
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);
}

void CloudDriveModel::fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, QString localFilePath, int modelIndex)
{
    if (localFilePath == "") {
        qDebug() << "CloudDriveModel::fileGet localFilePath" << localFilePath << " is empty, can't sync.";
        return;
    }

    if (remoteFilePath == "") {
        qDebug() << "CloudDriveModel::fileGet remoteFilePath" << remoteFilePath << " is empty, can't sync.";
        return;
    }

    // Redirect to filePutResume for large file.
    qDebug() << "CloudDriveModel::fileGet" << remoteFilePath << "size" << remoteFileSize;
    if (getCloudClient(type)->isFileGetResumable(remoteFileSize)) {
        qDebug() << "CloudDriveModel::fileGet" << remoteFilePath << "size" << remoteFileSize << ", request is redirected to fileGetResume.";
        fileGetResume(type, uid, remoteFilePath, remoteFileSize, localFilePath);
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), FileGet, type, uid, localFilePath, remoteFilePath, modelIndex);
//    job.isRunning = true;
    job.downloadOffset = 0;
    job.bytesTotal = remoteFileSize;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isConnectedCache->remove(localFilePath);
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, int modelIndex)
{
    if (localFilePath == "") {
        qDebug() << "CloudDriveModel::filePut localFilePath" << localFilePath << " is empty, can't sync.";
        return;
    }

    if (remoteParentPath == "") {
        qDebug() << "CloudDriveModel::filePut remoteParentPath" << remoteParentPath << " is empty, can't sync.";
        return;
    }

    if (remoteFileName == "") {
        qDebug() << "CloudDriveModel::filePut remoteFileName" << remoteFileName << " is empty, can't sync.";
        return;
    }

    // Restrict file types.
    if (!canSync(localFilePath)) {
        qDebug() << "CloudDriveModel::filePut localFilePath" << localFilePath << " is restricted, can't upload.";
        return;
    }

    // Redirect to filePutResume for large file.
    qint64 localFileSize = QFileInfo(localFilePath).size();
    if (getCloudClient(type)->isFilePutResumable(localFileSize)) {
        qDebug() << "CloudDriveModel::filePut" << localFilePath << "size" << localFileSize << ", request is redirected to filePutResume.";
        filePutResume(type, uid, localFilePath, remoteParentPath, remoteFileName);
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), FilePut, type, uid, localFilePath, remoteParentPath, modelIndex);
//    job.isRunning = true;
    job.newRemoteFileName = remoteFileName;
    job.uploadOffset = 0;
    job.bytesTotal = QFileInfo(localFilePath).size();
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isConnectedCache->remove(localFilePath);
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex, bool forcePut, bool forceGet)
{
    if (localFilePath == "") {
        qDebug() << "CloudDriveModel::metadata localFilePath" << localFilePath << " is empty, can't sync.";
        return;
    }

    if (remoteFilePath == "") {
        qDebug() << "CloudDriveModel::metadata remoteFilePath" << remoteFilePath << " is empty, can't sync.";
        return;
    }

    if (isRemoteRoot(type, uid, remoteFilePath) || remoteFilePath == "/") {
        qDebug() << "CloudDriveModel::metadata remoteFilePath" << remoteFilePath << " is root, can't sync.";
        return;
    }

    // TODO Restrict file types.
    if (!canSync(localFilePath)) {
        qDebug() << "CloudDriveModel::metadata localFilePath" << localFilePath << " is restricted, can't sync.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), Metadata, type, uid, localFilePath, remoteFilePath, modelIndex);
//    job.isRunning = true;
    job.forcePut = forcePut;
    job.forceGet = forceGet;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    clearConnectedRemoteDirtyCache(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::browse(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Browse, type, uid, "", remoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);

    // Force start thread.
    CloudDriveModelThread *t = new CloudDriveModelThread(this);
    t->setNonce(job.jobId); // Set job ID to thread. It will invoke parent's dispatchJob later.
    t->setDirectInvokation(false);
    QThreadPool::globalInstance()->start(t);
}

QStringList CloudDriveModel::getLocalPathList(QString localParentPath)
{
    QStringList localPathList;
    QDir dir(localParentPath);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    foreach (QFileInfo item, dir.entryInfoList()) {
        localPathList.append(item.absoluteFilePath());
    }

    return localPathList;
}

void CloudDriveModel::syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, int modelIndex, bool forcePut, QString data)
{
    qDebug() << "----- CloudDriveModel::syncFromLocal -----" << type << uid << localPath << "remoteParentPath" << remoteParentPath << "forcePut" << forcePut << "data" << data;

    if (localPath == "") {
        qDebug() << "CloudDriveModel::syncFromLocal localPath" << localPath << "is empty. Operation is aborted.";
        return;
    }

    if (remoteParentPath == "") {
        qDebug() << "CloudDriveModel::syncFromLocal remoteParentPath" << remoteParentPath << "is empty. Operation is aborted.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), SyncFromLocal, type, uid, localPath, remoteParentPath, modelIndex);
    job.forcePut = forcePut;
    job.data = data;
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    clearConnectedRemoteDirtyCache(localPath);
    emit jobEnqueuedSignal(job.jobId, localPath);
}

void CloudDriveModel::syncFromLocal_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, int modelIndex, bool forcePut, bool isRootLocalPath, QString data)
{
    // This method is invoked from dir only as file which is not found will be put right away.
    qDebug() << "----- CloudDriveModel::syncFromLocal_Block -----" << nonce << type << uid << localPath << remoteParentPath << modelIndex << "forcePut" << forcePut << "data" << data;

    if (localPath == "") {
        qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "localPath" << localPath << "is empty. Operation is aborted.";
        return;
    }

    if (remoteParentPath == "") {
        qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "remoteParentPath" << remoteParentPath << "is empty. Operation is aborted.";
        return;
    }

    // TODO Must not be invoked if it's running in main thread.
    QApplication::processEvents();

    CloudDriveClient *cloudClient = getCloudClient(type);
    if (cloudClient == 0) {
        return;
    }

    QStringList remotePathList = data.split(",");
//    qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "remotePathList" << remotePathList;

    QFileInfo info(localPath);
    if (info.isDir()) {
        // Sync based on local contents.

        // TODO create remote directory if no content or pending refresh metadata.
        CloudDriveItem parentCloudDriveItem = getItem(localPath, type, uid);
        if (parentCloudDriveItem.localPath == "" || parentCloudDriveItem.hash == CloudDriveModel::DirtyHash) {
            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "not found parentCloudDriveItem. Invoke creatFolder.";
            // Get remoteParentPath from localParentPath's cloudDriveItem if it's not specified.
            remoteParentPath = (remoteParentPath == "") ? getItemRemotePath(info.absolutePath(), type, uid) : remoteParentPath;
            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "remoteParentPath" << remoteParentPath;

            // Request SkyDriveClient's createFolder synchronously.
            // NOTE Insert dummy job to support QML CloudDriveModel.onCreateFolderReplySignal. It will be removed there.
            CloudDriveJob job(createNonce(), CreateFolder, type, uid, localPath, remoteParentPath, modelIndex);
            job.newRemoteFileName = info.fileName();
            m_cloudDriveJobs->insert(job.jobId, job);

            QString createFolderReplyResult = cloudClient->createFolder(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFileName, true);
//            qDebug() << "CloudDriveModel::syncFromLocal_Block createFolderReplyResult" << createFolderReplyResult;
            if (createFolderReplyResult != "") {
                QScriptEngine engine;
                QScriptValue sc = engine.evaluate("(" + createFolderReplyResult + ")");
                QString hash = "";
                QString createdRemotePath = "";

                /* Handle error result.
                 *Dropbox {"error": " at path 'The folder 'C' already exists.'"}
                 *SkyDrive {   "error": {      "code": "resource_already_exists",       "message": "The resource couldn't be created because a resource named 'C' already exists."   }}
                 *GoogleDrive supports duplicated folder's name.
                 *Ftp { "error": "Creating directory failed: /Users/test/D: File exists.", "path": "/Users/test/D" }
                 */
                if (sc.property("error").isValid()) {
                    qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "createFolder error localPath" << localPath << "remoteParentPath" << remoteParentPath << "createFolderReplyResult" << createFolderReplyResult;
                    return;
                }

                hash = sc.property("hash").toString();
                createdRemotePath = sc.property("absolutePath").toString();
                addItem(type, uid, localPath, createdRemotePath, hash);

                // Update parentCloudDriveItem.
                parentCloudDriveItem = getItem(localPath, type, uid);
                qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "createFolder success parentCloudDriveItem" << parentCloudDriveItem;
            } else {
                // Insert dummy job and invoke slot to emit signal to front-end.
                // TODO Suppress signal if newRemoteFolder is not requested path.
                qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "createFolder failed localPath" << localPath << "remoteParentPath" << remoteParentPath << "createFolderReplyResult" << createFolderReplyResult;
                return;
            }
        } else {
            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "found parentCloudDriveItem" << parentCloudDriveItem;
        }

        // TODO Abort operation if remotePath is not specified.
        if (parentCloudDriveItem.remotePath == "") {
            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "remotePath is not specified. Operation is aborted. parentCloudDriveItem" << parentCloudDriveItem;
        }

        QDir dir(info.absoluteFilePath());
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        foreach (QFileInfo item, dir.entryInfoList()) {
            // Process events to avoid freezing UI.
            QApplication::processEvents();

            QString localFilePath = item.absoluteFilePath();
            CloudDriveItem cloudDriveItem = getItem(localFilePath, type, uid);
            // TODO Cache isSyncing()
            bool isNewItem = !isSyncing(type, uid, localFilePath) && (cloudDriveItem.hash == "" || cloudDriveItem.remotePath == "");
            bool isDeletedItem = cloudDriveItem.remotePath != "" && remotePathList.indexOf(cloudDriveItem.remotePath) == -1 && remotePathList.indexOf("*") == -1;
            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "cloudDriveItem.remotePath" << cloudDriveItem.remotePath << "isNewItem" << isNewItem << "isDeletedItem" << isDeletedItem << "remotePathList" << remotePathList;

            // If dir/file don't have localHash which means it's not synced, put it right away.
            // If forcePut, put it right away.
            if (forcePut || isNewItem) {
                // Sync dir/file then it will decide whether get/put/do nothing by metadataReply.
                qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "new local item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "parentCloudDriveItem.remotePath" << parentCloudDriveItem.remotePath << "isNewItem" << isNewItem;

                if (item.isDir()) {
                    // Drilldown local dir recursively.
                    // TODO Don't pass remotePathList will cause removing items.
                    syncFromLocal_Block(nonce, type, uid, localFilePath, parentCloudDriveItem.remotePath, -1, forcePut, false);
                } else {
                    // Put file to remote parent path.
                    // TODO Add more comparing logic whether upload or just update hash.
                    filePut(type, uid, localFilePath, parentCloudDriveItem.remotePath, getFileName(localFilePath), -1);
                }
            } else if (isDeletedItem) {
                // Remove link if it's deleted item (there is remotePathList and cloudDriveItem.remotePath not in list).
                // TODO Configurable file removing.
                qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "remove link to existing local item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "cloudDriveItem.remotePath" << cloudDriveItem.remotePath << "isDeletedItem" << isDeletedItem;
                removeItemWithChildren(type, uid, localFilePath);
            } else {
                // Skip any items that already have CloudDriveItem and has localHash.
                qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "skip existing local item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "cloudDriveItem.remotePath" << cloudDriveItem.remotePath << "isNewItem" << isNewItem;
            }
        }

        // TODO avoid having below line. It caused infinite loop.
        // Update hash for itself will be requested from QML externally.
    } else {
        qDebug() << "CloudDriveModel::syncFromLocal_Block file is not supported." << type << uid << localPath << remoteParentPath << modelIndex << "forcePut" << forcePut;
    }
}

void CloudDriveModel::createFolder(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, QString newRemoteFolderName)
{
    if (newRemoteFolderName == "") {
        qDebug() << "CloudDriveModel::createFolder newRemoteFolderName" << newRemoteFolderName << "is empty. Operation is aborted.";
        return;
    }

    if (remoteParentPath == "") {
        qDebug() << "CloudDriveModel::createFolder remoteParentPath" << remoteParentPath << "is empty. Operation is aborted.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), CreateFolder, type, uid, localPath, remoteParentPath, -1);
    job.newRemoteFileName = newRemoteFolderName;
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // TODO Does it need?
    m_isSyncingCache->remove(localPath);
    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localPath);
}

QString CloudDriveModel::createFolder_Block(CloudDriveModel::ClientTypes type, QString uid, QString remoteParentPath, QString newRemoteFolderName)
{
    if (newRemoteFolderName == "") {
        qDebug() << "CloudDriveModel::createFolder_Block newRemoteFolderName" << newRemoteFolderName << "is empty. Operation is aborted.";
        return "";
    }

    if (remoteParentPath == "") {
        qDebug() << "CloudDriveModel::createFolder_Block remoteParentPath" << remoteParentPath << "is empty. Operation is aborted.";
        return "";
    }

    CloudDriveClient *cloudClient = getCloudClient(type);
    if (cloudClient == 0) {
        return "";
    }

    // Request cloudClient's createFolder synchronously.
    QString createFolderReplyResult = cloudClient->createFolder(createNonce(), uid, remoteParentPath, newRemoteFolderName, true);
    qDebug() << "CloudDriveModel::createFolder_Block createFolder createFolderReplyResult" << createFolderReplyResult;

    if (createFolderReplyResult != "") {
        QScriptEngine se;
        QScriptValue sc = se.evaluate("(" + createFolderReplyResult + ")");
        QString createdRemotePath = sc.property("absolutePath").toString();

        return createdRemotePath;
    }

    return "";
}

void CloudDriveModel::migrateFile_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName)
{
    qDebug() << "CloudDriveModel::migrateFile_Block" << nonce << type << uid << remoteFilePath << remoteFileSize << targetType << targetUid << targetRemoteParentPath << targetRemoteFileName;

    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    CloudDriveClient *sourceClient = getCloudClient(type);
    CloudDriveClient *targetClient = getCloudClient(targetType);
    QString tempFilePath = m_settings.value("temp.path", TEMP_PATH).toString() + "/" + nonce;

    // Migrates using synchronous methods.
    // TODO Limit migration size to fit in RAM. Larger file needs to be downloaded to local before proceed uploading.
    if (sourceClient->isFileGetResumable(remoteFileSize)) {
        // Get each chunk to file.
        while (job.downloadOffset < remoteFileSize && !m_isAborted) {
            QIODevice *source = sourceClient->fileGet(nonce, uid, remoteFilePath, job.downloadOffset, true);
            if (source == 0) {
                migrateFilePutFilter(nonce, -1, "Service is not implemented.", "{ }");
                return;
            }
            // Handle error.
            // TODO Check type before cast.
            QNetworkReply *sourceReply = dynamic_cast<QNetworkReply *>(source);
            if (sourceReply->error() != QNetworkReply::NoError) {
                migrateFilePutFilter(nonce, sourceReply->error(), sourceReply->errorString(), sourceReply->readAll());
                return;
            }
            qDebug() << "CloudDriveModel::migrateFile_Block chunk is downloaded. size" << source->size() << "bytesAvailable" << source->bytesAvailable() << "job" << job.toJsonText();

            // Stream source to a file on localPath.
            qDebug() << "CloudDriveModel::migrateFile_Block tempFilePath" << tempFilePath;
            qint64 totalBytes = sourceClient->writeToFile(source, tempFilePath, job.downloadOffset);
            if (totalBytes >= 0) {
                qDebug() << "CloudDriveModel::migrateFile_Block chunk is written at offset" << job.downloadOffset << "totalBytes"  << totalBytes << "to" << tempFilePath;
            } else {
                qDebug() << "CloudDriveModel::migrateFile_Block can't open tempFilePath" << tempFilePath;
                migrateFilePutFilter(nonce, -1, QString("Can't open temp file %1").arg(tempFilePath), "{ }");
                return;
            }

            // Close source.
            source->close();
            source->deleteLater();

            job.downloadOffset += totalBytes;
            updateJob(job);
        }
    } else {
        // Get whole data to file with synchronous method.
        sourceClient->fileGet(nonce, uid, remoteFilePath, tempFilePath, true);
        job.downloadOffset = QFileInfo(tempFilePath).size();
        updateJob(job);
    }
    qDebug() << "CloudDriveModel::migrateFile_Block source is downloaded to temp file. tempFilePath" << tempFilePath << "size" << job.downloadOffset << "remoteFileSize" << remoteFileSize;

    // Check isAborted.
    if (m_isAborted) {
        migrateFilePutFilter(nonce, -1, "job is aborted.","");
        return;
    }

    // Put from file.
    QFile *localSourceFile = new QFile(tempFilePath);
    if (localSourceFile->open(QFile::ReadOnly)) {
        QNetworkReply *targetReply = targetClient->filePut(nonce, targetUid, localSourceFile, remoteFileSize, targetRemoteParentPath, targetRemoteFileName, true);
        if (targetReply == 0) {
            // FtpClient doens't provide QNetworkReply. It emits migrateFilePutReplySignal internally.
            job.uploadOffset = remoteFileSize;
            updateJob(job);
        } else {
            if (targetReply->error() == QNetworkReply::NoError){
                job.uploadOffset = remoteFileSize;
                // Invoke slot to reset running and emit signal.
                migrateFilePutFilter(nonce, targetReply->error(), targetReply->errorString(),targetReply->readAll());
            } else {
                // Invoke slot to reset running and emit signal.
                // TODO How to shows if error occurs from target?
                migrateFilePutFilter(nonce, targetReply->error(), getCloudName(job.targetType) + " " + targetReply->errorString(), getCloudName(job.targetType).append(" ").append(targetReply->readAll()) );
            }

            // Scheduled to delete later.
            targetReply->deleteLater();
            targetReply->manager()->deleteLater();
        }
    }
    localSourceFile->close();
    localSourceFile->deleteLater();

    // Delete temp file.
    // TODO Temp file should also be removed if job is removed manually.
    if (job.uploadOffset == job.downloadOffset) {
        QFile(tempFilePath).remove();
        qDebug() << "CloudDriveModel::migrateFile_Block job" << nonce << "done." << tempFilePath << "is removed.";
    }
}

void CloudDriveModel::migrateFileResume_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName)
{
    qDebug() << "CloudDriveModel::migrateFileResume_Block" << nonce << type << uid << remoteFilePath << remoteFileSize << targetType << targetUid << targetRemoteParentPath << targetRemoteFileName;

    // Migrates using resumable methods.
    // TODO Source and target chunk size may not be the same.
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    CloudDriveClient *sourceClient = getCloudClient(type);
    CloudDriveClient *targetClient = getCloudClient(targetType);
    QScriptEngine engine;
    QScriptValue sc;
    qDebug() << "CloudDriveModel::migrateFileResume_Block job" << job.toJsonText();

    // Start uploading session and get upload_id.
    if (job.uploadId == "") {
        QString startResult = targetClient->filePutResumeStart(nonce, targetUid, targetRemoteFileName, remoteFileSize, targetRemoteParentPath, true);
        if (startResult != "") {
            qDebug() << "CloudDriveModel::migrateFileResume_Block startResult" << startResult;
            sc = engine.evaluate("(" + startResult + ")");

            if (sc.property("upload_id").isValid()) {
                job.uploadId = sc.property("upload_id").toString();
                m_cloudDriveJobs->insert(job.jobId, job);
            }
            if (sc.property("error").isValid()) {
                int err = sc.property("error").toInt32();
                QString errString = sc.property("error_string").toString();
                migrateFilePutFilter(job.jobId, err, errString, "");
                return;
            }
        }
    } else if (job.uploadOffset < 0) {
        // Check uploading status.
        QString statusResult = targetClient->filePutResumeStatus(nonce, targetUid, targetRemoteFileName, remoteFileSize, job.uploadId, job.uploadOffset, true);
        if (statusResult != "") {
            // TODO Get range and check if resume upload is required.
            qDebug() << "CloudDriveModel::migrateFileResume_Block statusResult" << statusResult;
            sc = engine.evaluate("(" + statusResult + ")");

            if (sc.property("offset").isValid()) {
                job.uploadOffset = sc.property("offset").toUInt32();
                m_cloudDriveJobs->insert(job.jobId, job);
            }
            if (sc.property("error").isValid()) {
                int err = sc.property("error").toInt32();
                QString errString = sc.property("error_string").toString();
                migrateFilePutFilter(job.jobId, err, errString, "");
                return;
            }
        }
    }

    // Adjust downloadOffset to follow uploadOffset.
    job.uploadOffset = (job.uploadOffset < 0) ? 0 : job.uploadOffset;
    job.downloadOffset = job.uploadOffset;
    m_cloudDriveJobs->insert(job.jobId, job);

    // Check isAborted.
    if (m_isAborted) {
        migrateFilePutFilter(nonce, -1, "job is aborted.","");
        return;
    }

    // Download chunk.
    QIODevice *source = sourceClient->fileGet(nonce, uid, remoteFilePath, job.downloadOffset, true);
    if (source == 0) {
        // Error occurs in fileGet method.
        migrateFilePutFilter(nonce, -1, tr("Service is not implemented."), "{ }");
        return;
    } else {
        qDebug() << "CloudDriveModel::migrateFileResume_Block chunk is downloaded. source->metaObject()->className()" << source->metaObject()->className() << "size" << source->size() << "bytesAvailable" << source->bytesAvailable() << "job" << job.toJsonText();
        // TODO Handle source error.
        if (QNetworkReply *sourceReply = dynamic_cast<QNetworkReply*>(source)) {
            if (sourceReply->error() != QNetworkReply::NoError) {
                migrateFilePutFilter(job.jobId, sourceReply->error(), sourceReply->errorString(), QString::fromUtf8(sourceReply->readAll()));
                return;
            }
        }
        job.downloadOffset += source->size();
        m_cloudDriveJobs->insert(job.jobId, job);
    }

    // Check isAborted.
    // Reset downloadOffset as migrateFileResume_Block never store downloaded chunk.
    if (m_isAborted) {
        job.downloadOffset = 0;
        m_cloudDriveJobs->insert(job.jobId, job);
        migrateFilePutFilter(nonce, -1, "job is aborted.","");
        return;
    }

    // Upload chunk and get offset (and upload_id if exists).
    QString uploadResult = targetClient->filePutResumeUpload(nonce, targetUid, source, targetRemoteFileName, remoteFileSize, job.uploadId, job.uploadOffset, true);
    // Default target remote path for client with remote absolute path. (Ex. Dropbox, FTP)
    QString targetRemoteFilePath = (targetClient->isRemoteAbsolutePath()) ? (targetRemoteParentPath + "/" + targetRemoteFileName) : "";
    if (uploadResult != "") {
        // TODO Get range and check if resume upload is required.
        qDebug() << "CloudDriveModel::migrateFileResume_Block uploadResult" << uploadResult;
        sc = engine.evaluate("(" + uploadResult + ")");

        if (sc.property("upload_id").isValid()) {
            job.uploadId = sc.property("upload_id").toString();
        }
        if (sc.property("offset").isValid()) {
            job.uploadOffset = sc.property("offset").toUInt32();
        }
        if (sc.property("id").isValid()) { // Find targetRemoteFilePath from GoogleDrive reply.
            targetRemoteFilePath = sc.property("id").toString();
            job.uploadOffset = sc.property("fileSize").toUInt32(); // Get fileSize to uploadOffset.
            qDebug() << "CloudDriveModel::migrateFileResume_Block uploaded file with targetRemoteFilePath" << targetRemoteFilePath;
        }
        // Update changed job.
        m_cloudDriveJobs->insert(job.jobId, job);

        if (sc.property("error").isValid()) {
            int err = sc.property("error").toInt32();
            QString errString = sc.property("error_string").toString();
            job.uploadOffset = -1; // Failed upload needs to get status on resume.
            migrateFilePutFilter(job.jobId, err, errString, "");
            return;
        }
    } else {
        migrateFilePutFilter(job.jobId, -1, "Unexpected error.", "Unexpected upload result is empty.");
        return;
    }
    // Check whether resume or commit.
    if (job.uploadOffset < job.bytesTotal) {
        // Enqueue and resume job.
        qDebug() << "CloudDriveModel::migrateFileResume_Block resume uploading job" << job.toJsonText();
        m_jobQueue->enqueue(job.jobId);
        updateJob(job);
        jobDone();
        return;
    } else {
        // Invoke to handle successful uploading.
        qDebug() << "CloudDriveModel::migrateFileResume_Block commit uploading job" << job.toJsonText();
        QString commitResult = targetClient->filePutCommit(nonce, targetUid, targetRemoteFilePath, job.uploadId, true);
        if (commitResult != "") {
            qDebug() << "GCDClient::migrateFileResume_Block commitResult" << commitResult;
            sc = engine.evaluate("(" + commitResult + ")");

            if (sc.property("error").isValid()) {
                int err = sc.property("error").toInt32();
                QString errString = sc.property("error_string").toString();
                migrateFilePutFilter(job.jobId, err, errString, "");
                return;
            }
            // Proceed filter with commit result and job done.
            migrateFilePutFilter(job.jobId, QNetworkReply::NoError, "", commitResult);
        } else {
            // Proceed filter with last upload result and job done.
            migrateFilePutFilter(job.jobId, QNetworkReply::NoError, "", uploadResult);
        }
    }
}

void CloudDriveModel::disconnect(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Disconnect, type, uid, localPath, "", -1);
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show in job page.
    emit jobEnqueuedSignal(job.jobId, job.localFilePath);
}

QString CloudDriveModel::thumbnail(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString format, QString size)
{
    CloudDriveClient *client = getCloudClient(getClientType(type));
    if (client == 0) return "";

    return client->thumbnail(createNonce(), uid, remoteFilePath, format, size);
}

void CloudDriveModel::cacheImage(QString remoteFilePath, QString url, int w, int h, QString caller)
{
    CacheImageWorker *worker = new CacheImageWorker(remoteFilePath, url, QSize(w,h), TEMP_PATH, caller);
    connect(worker, SIGNAL(cacheImageFinished(QString,int,QString,QString)), SIGNAL(cacheImageFinished(QString,int,QString,QString)));
    connect(worker, SIGNAL(refreshFolderCacheSignal(QString)), SIGNAL(refreshFolderCacheSignal(QString)));
    QThreadPool::globalInstance()->start(worker);
}

QString CloudDriveModel::media(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath)
{
    CloudDriveClient *client = getCloudClient(getClientType(type));
    if (client == 0) return "";

    return client->media(createNonce(), uid, remoteFilePath);
}

void CloudDriveModel::moveFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    if (remoteFilePath == "") {
        qDebug() << "CloudDriveModel::moveFile remoteFilePath" << remoteFilePath << "is empty. Operation is aborted.";
        return;
    }

    // NOTE moveFile without newRemoteParentPath means rename.
    if (newRemoteFileName == "") {
        qDebug() << "CloudDriveModel::moveFile newRemoteFileName" << newRemoteFileName << "is empty. Operation is aborted.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), MoveFile, type, uid, localFilePath, remoteFilePath, -1);
    job.newLocalFilePath = newLocalFilePath;
    job.newRemoteFilePath = newRemoteParentPath;
    job.newRemoteFileName = newRemoteFileName;
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::copyFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteParentPath, QString newRemoteFileName)
{
    if (remoteFilePath == "") {
        qDebug() << "CloudDriveModel::copyFile remoteFilePath" << remoteFilePath << "is empty. Operation is aborted.";
        return;
    }

    if (newRemoteParentPath == "" && newRemoteFileName == "") {
        qDebug() << "CloudDriveModel::copyFile newRemoteParentPath" << newRemoteParentPath << "or newRemoteFileName" << newRemoteFileName << "is empty. Operation is aborted.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), CopyFile, type, uid, localFilePath, remoteFilePath, -1);
    job.newLocalFilePath = newLocalFilePath;
    job.newRemoteFilePath = newRemoteParentPath;
    job.newRemoteFileName = newRemoteFileName;
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::deleteFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath)
{
    if (remoteFilePath == "") {
        qDebug() << "CloudDriveModel::deleteFile remoteFilePath" << remoteFilePath << "is empty. Operation is aborted.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), DeleteFile, type, uid, localFilePath, remoteFilePath, -1);
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::shareFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), ShareFile, type, uid, localFilePath, remoteFilePath, -1);
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    clearConnectedRemoteDirtyCache(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::delta(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Delta, type, uid, "", "", -1);
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show in job page.
    emit jobEnqueuedSignal(job.jobId, "");
}

void CloudDriveModel::filePutResume(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), FilePutResume, type, uid, localFilePath, remoteParentPath, -1);
    job.newRemoteFileName = remoteFileName;
    job.uploadId = uploadId;
    job.uploadOffset = offset;
    job.bytesTotal = QFileInfo(localFilePath).size();
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isConnectedCache->remove(localFilePath);
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::fileGetResume(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFIleSize, QString localFilePath, qint64 offset)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), FileGetResume, type, uid, localFilePath, remoteFilePath, -1);
    job.downloadOffset = offset;
    job.bytesTotal = remoteFIleSize;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    m_isConnectedCache->remove(localFilePath);
    m_isSyncingCache->remove(localFilePath);
    emit jobEnqueuedSignal(job.jobId, localFilePath);
}

void CloudDriveModel::migrateFile(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), MigrateFile, type, uid, "", remoteFilePath, -1);
    job.targetType = targetType;
    job.targetUid = targetUid;
    job.newRemoteFilePath = targetRemoteParentPath;
    job.newRemoteFileName = targetRemoteFileName;
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, "");
}

void CloudDriveModel::migrateFilePut(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 bytesTotal, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName)
{
    qDebug() << "CloudDriveModel::migrateFilePut" << type << uid << remoteFilePath << bytesTotal << targetType << targetUid << targetRemoteParentPath << targetRemoteFileName;

    // Enqueue job.
    CloudDriveJob job(createNonce(), MigrateFilePut, type, uid, "", remoteFilePath, -1);
    job.bytesTotal = bytesTotal;
    job.targetType = targetType;
    job.targetUid = targetUid;
    job.newRemoteFilePath = targetRemoteParentPath;
    job.newRemoteFileName = targetRemoteFileName;
//    job.isRunning = true;
    m_cloudDriveJobs->insert(job.jobId, job);
    m_jobQueue->enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, "");
}

void CloudDriveModel::fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;

    if (err == 0) {
        // TODO handle other clouds.
        if (job.localFilePath != "") {
            sc = engine.evaluate("(" + msg + ")");
            hash = sc.property("hash").toString();
            addItem(getClientType(job.type), job.uid, job.localFilePath, job.remoteFilePath, hash);

            // Emit signal to refresh its parent folder cache.
            emit refreshFolderCacheSignal(job.localFilePath);
        }
    } else {
        if (job.localFilePath != "") {
            removeItem(getClientType(job.type), job.uid, job.localFilePath);
        }
    }

    // Update job running flag.
    job.err = err;
    job.errString = errMsg;
//    job.errMessage = msg.replace(QRegExp("\""), "\\\""); // TODO encode to ?
    job.isRunning = false;
    updateJob(job);

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

    if (err == 0) {
        // TODO handle other clouds.
        if (job.localFilePath != "") {
            sc = engine.evaluate("(" + msg + ")");
            remoteFilePath = sc.property("absolutePath").toString();
            hash = sc.property("hash").toString();
            job.newRemoteFilePath = remoteFilePath;
            addItem(getClientType(job.type), job.uid, job.localFilePath, remoteFilePath, hash);
        }
    } else {
        if (job.localFilePath != "") {
            removeItem(getClientType(job.type), job.uid, job.localFilePath);
        }
    }

    // Update job running flag.
    job.err = err;
    job.errString = errMsg;
//    job.errMessage = msg.replace(QRegExp("\""), "\\\""); // TODO encode to ?
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit filePutReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Update job running flag.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit metadataReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::browseReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Update job running flag.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    // Route signal to caller.
    if (job.operation == MigrateFile) {
        emit migrateFileReplySignal(nonce, err, errMsg, msg);
    } else {
        if (err == 0) {
            m_cachedJsonText = sortJsonText(msg, m_sortFlag);
            emit browseReplySignal(nonce, err, errMsg, m_cachedJsonText);
        } else {
            emit browseReplySignal(nonce, err, errMsg, msg);
        }
    }
}

void CloudDriveModel::createFolderReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;
    QString hash;
    QString newRemotePath;
    QString newRemoteParentPath;
    bool isDir;

    if (err == 0) {
        // Add connection if localFilePath is specified because createFolder was invoked in syncFromLocal.
        // NOTE It's not required to create cloud item for created folder. Because further sync operation will do.
        sc = engine.evaluate("(" + msg + ")");
        isDir = sc.property("isDir").toBool();
        hash = sc.property("hash").toString();
        newRemotePath = sc.property("absolutePath").toString();
        newRemoteParentPath = sc.property("parentPath").toString();

        qDebug() << "CloudDriveModel::createFolderReplyFilter" << getCloudName(job.type) << "hash" << hash << "newRemotePath" << newRemotePath << "newRemoteParentPath" << newRemoteParentPath;

        // Sync newRemoteParentPath (with DirtyHash) to get newRemotePath sync'd.
        // Local path isn't specified, it's requested from CloudFolderPage.
        // If (newRemotePath's parent is connected but newRemotePath is not connected), sync its parent to connected local path.
        if (isRemotePathConnected(getClientType(job.type), job.uid, newRemoteParentPath) && !isRemotePathConnected(getClientType(job.type), job.uid, newRemotePath) && job.localFilePath == "") {
            qDebug() << "CloudDriveModel::createFolderReplyFilter newRemotePath" << newRemotePath << "is under connected parent remote path. Sync its parent" << newRemoteParentPath;
            syncItemByRemotePath(getClientType(job.type), job.uid, newRemoteParentPath, DirtyHash);
        }
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

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
    QString newRemotePath;
    QString newRemoteParentPath;
    QString oldRemoteParentPath;
    bool isDir;

    if (err == 0) {
        // Update connection for client which uses absolute remote path.
        if (isRemoteAbsolutePath(getClientType(job.type))) {
            sc = engine.evaluate("(" + msg + ")");
            isDir = sc.property("isDir").toBool();
            hash = sc.property("hash").toString();
            newRemotePath = sc.property("absolutePath").toString();
            newRemoteParentPath = sc.property("parentPath").toString();
            oldRemoteParentPath = getParentRemotePath(getClientType(job.type), job.remoteFilePath);

            if (job.localFilePath != "" && job.newLocalFilePath != "") {
                // Local path is specified, it's requested from FolderPage.
                // Update both new local and remote path for specified local path.
                updateItemWithChildren(getClientType(job.type), job.uid,
                                       job.localFilePath, job.remoteFilePath,
                                       job.newLocalFilePath, newRemotePath,
                                       hash);
                // Remove cloud item and its children with original local path.
                removeItemWithChildren(getClientType(job.type), job.uid, job.localFilePath);
            } else {
                // Local path isn't specified, it's requested from CloudFolderPage.
                // Update new remote path for all connected local path.
                foreach (CloudDriveItem item, findItemsByRemotePath(getClientType(job.type), job.uid, job.remoteFilePath)) {
                    qDebug() << "CloudDriveModel::moveFileReplyFilter updateItemWithChildren item" << item.localPath << "remotePath" << item.remotePath << "newRemotePath" << newRemotePath << "new hash" << hash;
                    updateItemWithChildren(getClientType(job.type), job.uid,
                                           item.localPath, item.remotePath,
                                           item.localPath, newRemotePath,
                                           hash);
                }

                // Sync both oldRemoteParentPath and newRemoteParentPath (with DirtyHash).
                // If newRemotePath's parent is connected, sync its parent to connected local path.
                if (isRemotePathConnected(getClientType(job.type), job.uid, newRemoteParentPath)) {
                    qDebug() << "CloudDriveModel::moveFileReplyFilter newRemotePath" << newRemotePath << "is under connected parent remote path. Sync its parent" << newRemoteParentPath;
                    syncItemByRemotePath(getClientType(job.type), job.uid, newRemoteParentPath, DirtyHash);
                }
                if (oldRemoteParentPath != newRemoteParentPath && isRemotePathConnected(getClientType(job.type), job.uid, oldRemoteParentPath)) {
                    qDebug() << "CloudDriveModel::moveFileReplyFilter remoteFilePath" << job.remoteFilePath << "is under connected parent remote path. Sync its parent" << oldRemoteParentPath;
                    syncItemByRemotePath(getClientType(job.type), job.uid, oldRemoteParentPath, DirtyHash);
                }
            }
        }
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

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
    QString newRemotePath;
    QString newRemoteParentPath;
    bool isDir;

    if (err == 0) {
        // TODO If it doesn't do here, it will be sync'd with parent folder's metadata request.
        sc = engine.evaluate("(" + msg + ")");
        isDir = sc.property("isDir").toBool();
        hash = sc.property("hash").toString();
        newRemotePath = sc.property("absolutePath").toString();
        newRemoteParentPath = sc.property("parentPath").toString();
        qDebug() << "CloudDriveModel::copyFileReplyFilter" << getCloudName(job.type) << "hash" << hash << "newRemotePath" << newRemotePath << "newRemoteParentPath" << newRemoteParentPath;

        // Sync newRemoteParentPath (with DirtyHash) to get newRemotePath sync'd.
        // Local path isn't specified, it's requested from CloudFolderPage.
        // If newRemotePath's parent is connected, sync its parent to connected local path.
        if (isRemotePathConnected(getClientType(job.type), job.uid, newRemoteParentPath)) {
            qDebug() << "CloudDriveModel::copyFileReplyFilter newRemotePath" << newRemotePath << "is under connected parent remote path. Sync its parent" << newRemoteParentPath;
            syncItemByRemotePath(getClientType(job.type), job.uid, newRemoteParentPath, DirtyHash);
        }
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit copyFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::deleteFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Disconnect deleted local path.
    if (job.localFilePath != "") {
        removeItemWithChildren(getClientType(job.type), job.uid, job.localFilePath);
    }
    if (job.remoteFilePath != "") {
        removeItemByRemotePath(getClientType(job.type), job.uid, job.remoteFilePath);
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit deleteFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::shareFileReplyFilter(QString nonce, int err, QString errMsg, QString msg, QString url, int expires)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit shareFileReplySignal(nonce, err, errMsg, msg, url, expires);
}

void CloudDriveModel::deltaReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    if (err == QNetworkReply::NoError) {
        QHash<QString, bool> remotePathHash;
        QScriptEngine engine;
        QScriptValue parsedObj = engine.evaluate("(" + msg + ")");

        // TODO Process delta. Move to QML to connect to FolderSizeItemListModel's delete recursive method.
        // TODO Suppress duplicated items to avoid overload job queues.
        if (parsedObj.property("children").isValid()) {
            for (int i = 0; i < parsedObj.property("children").toVariant().toList().length(); i++) {
                QScriptValue childObj = parsedObj.property("children").property(i);
                bool isDeleted = childObj.property("isDeleted").toBool();
                QString remoteFilePath = childObj.property("absolutePath").toString();
                QString remoteParentPath = childObj.property("parentPath").toString();
                //        qDebug() << "CloudDriveModel::deltaReplyFilter remoteFilePath" << remoteFilePath << "remoteParentPath" << remoteParentPath << "isDeleted" << isDeleted;

                if (isDeleted) {
                    if (remotePathHash.value(remoteFilePath)) {
                        qDebug() << "CloudDriveModel::deltaReplyFilter suppress remove remoteFilePath" << remoteFilePath;
                    } else {
                        // TODO delete local file path and cloud item.
                        // NOTE Metadata currently only remove link once file/folder was removed remotely.
                        foreach (CloudDriveItem item, findItemsByRemotePath(getClientType(job.type), job.uid, remoteFilePath, isRemotePathCaseInsensitive(getClientType(job.type)))) {
                            qDebug() << "CloudDriveModel::deltaReplyFilter remove connection to deleted remoteFilePath" << remoteFilePath << "cloudItem" << item;
                            removeItem(getClientType(item.type), item.uid, item.localPath);
                        }

                        // Insert remoteFilePath with isDeleted to hash.
                        remotePathHash.insert(remoteFilePath, isDeleted);
                    }
                } else {
                    if (remotePathHash.contains(remoteFilePath) || remotePathHash.contains(remoteParentPath)) {
                        qDebug() << "CloudDriveModel::deltaReplyFilter suppress sync remoteFilePath" << remoteFilePath << "remoteParentPath" << remoteParentPath;
                    } else {
                        QList<CloudDriveItem> itemList = findItemsByRemotePath(getClientType(job.type), job.uid, remoteFilePath, isRemotePathCaseInsensitive(getClientType(job.type)));
                        if (itemList.isEmpty()) {
                            // TODO Sync its connected parents (with DirtyHash) to force sync all its children.
                            qDebug() << "CloudDriveModel::deltaReplyFilter download new item by syncing remoteParentPath" << remoteParentPath << "isRemotePathCaseInsensitive" << isRemotePathCaseInsensitive(getClientType(job.type));
                            foreach (CloudDriveItem item, findItemsByRemotePath(getClientType(job.type), job.uid, remoteParentPath, isRemotePathCaseInsensitive(getClientType(job.type)))) {
                                qDebug() << "CloudDriveModel::deltaReplyFilter sync remoteParentPath" << remoteParentPath << "parent cloudItem" << item;
                                updateItem(getClientType(item.type), item.uid, item.localPath, DirtyHash);
                                metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1);
                            }
                        } else {
                            // TODO Sync existing item.
                            foreach (CloudDriveItem item, itemList) {
                                qDebug() << "CloudDriveModel::deltaReplyFilter sync remoteFilePath" << remoteFilePath << "cloudItem" << item;
                                metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1);
                            }
                        }

                        // Insert remoteFilePath with isDeleted to hash.
                        remotePathHash.insert(remoteFilePath, isDeleted);
                        remotePathHash.insert(remoteParentPath, isDeleted);
                    }
                }
            }
        }

        // Check if there is more delta.
        if (parsedObj.property("hasMore").toBool()) {
            qDebug() << "CloudDriveModel::deltaReplyFilter has more delta. Proceed next delta request.";
            delta(getClientType(job.type), job.uid);
        }

        // Clear remotePathHash.
        remotePathHash.clear();
    }

    // Update job running flag.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit deltaReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::migrateFilePutFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Update job running flag.
    job.err = err;
    job.errString = errMsg;
//    job.errMessage = msg.replace(QRegExp("\""), "\\\""); // TODO encode to ?
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit migrateFilePutReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::fileGetResumeReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    qDebug() << "CloudDriveModel::fileGetResumeReplyFilter" << nonce << err << errMsg << msg;

    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    if (err == 0) {
        if (job.localFilePath != "") {
            // TODO Get next offset from metadata. But it will require type switch because each client has its response format.
            job.downloadOffset = job.bytes;

            // Enqueue and resume job.
            if (job.downloadOffset < job.bytesTotal) {
                qDebug() << "CloudDriveModel::fileGetResumeReplyFilter resume downloading job" << job.toJsonText();
                m_jobQueue->insert(0, job.jobId); // Resume download with priority.
            } else {
                qDebug() << "CloudDriveModel::fileGetResumeReplyFilter commit downloading job" << job.toJsonText();
                fileGetReplyFilter(job.jobId, err, errMsg, msg);
                return;
            }
        }
    } else if (err == QNetworkReply::AuthenticationRequiredError) {
        // Emit signal to refresh token and resume.
        m_cloudDriveJobs->insert(job.jobId, job);
        fileGetReplyFilter(job.jobId, err, errMsg, msg);
        return;
    } else if (err == 503) { // 503 Service Unavailable.
        // Emit signal to refresh token and resume.
        m_cloudDriveJobs->insert(job.jobId, job);
        fileGetReplyFilter(job.jobId, err, errMsg, msg);
        return;
    } else {
        qDebug() << "CloudDriveModel::fileGetResumeReplyFilter failed. Operation is aborted. jobId" << nonce << "error" << err << errMsg << msg;
        fileGetReplyFilter(job.jobId, err, errMsg, msg);
        return;
    }

    // Update job.
    updateJob(job);

    // Notify job done.
    jobDone();
}

void CloudDriveModel::filePutResumeReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    qDebug() << "CloudDriveModel::filePutResumeReplyFilter" << nonce << err << errMsg << msg;

    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    QScriptEngine engine;
    QScriptValue sc;

    if (err == 0) {
        if (job.localFilePath != "") {
            switch (job.type) {
            case Dropbox:
                sc = engine.evaluate("(" + msg + ")");
                if (sc.property("upload_id").isValid()) {
                    job.uploadId = sc.property("upload_id").toString();
                }
                if (sc.property("offset").isValid()) {
                    job.uploadOffset = sc.property("offset").toUInt32(); // NOTE It's actually the offset for resume uploading.
                }

                // Enqueue and resume job.
                if (job.uploadOffset < job.bytesTotal) {
                    qDebug() << "CloudDriveModel::filePutResumeReplyFilter resume uploading job" << job.toJsonText();
                    m_jobQueue->insert(0, job.jobId); // Resume upload with priority.
                } else {
                    qDebug() << "CloudDriveModel::filePutResumeReplyFilter commit uploading job" << job.toJsonText();
                    // Get uploaded path.
                    job.newRemoteFilePath = getRemotePath(getClientType(job.type), job.remoteFilePath, job.newRemoteFileName);
                    job.operation = FilePutCommit;
                    m_jobQueue->insert(0, job.jobId); // Resume upload with priority.
                }
                break;
            case GoogleDrive:
                sc = engine.evaluate("(" + msg + ")");
                if (sc.property("upload_id").isValid()) {
                    job.uploadId = sc.property("upload_id").toString();
                    job.uploadOffset = -1; // TODO Force request status. Is it required?
                }
                if (sc.property("offset").isValid()) {
                    job.uploadOffset = sc.property("offset").toUInt32(); // NOTE offset got from (maxRange+1) returned from status request. It's actually the offset for resume uploading.
                }
                if (sc.property("absolutePath").isValid()) { // Check if msg contains common file json with absolutePath, it means uploading is done.
                    job.uploadOffset = sc.property("size").toUInt32(); // NOTE file size is actually the offset for resume uploading.
                }

                // Check whether resume or commit.
                if (job.uploadOffset < job.bytesTotal) {
                    // Enqueue and resume job.
                    qDebug() << "CloudDriveModel::filePutResumeReplyFilter resume uploading job" << job.toJsonText();
                    m_jobQueue->insert(0, job.jobId); // Resume upload with priority.
                } else {
                    // Invoke to handle successful uploading.
                    qDebug() << "CloudDriveModel::filePutResumeReplyFilter commit uploading job" << job.toJsonText();
                    // Get uploaded path.
                    if (sc.property("absolutePath").isValid()) {
                        job.newRemoteFilePath = sc.property("absolutePath").toString();
                    }
                    filePutReplyFilter(job.jobId, err, errMsg, msg);
                    return;
                }
                break;
            }
        }
    } else if (err == QNetworkReply::AuthenticationRequiredError && job.type == GoogleDrive) {
        // TODO For GoogleDrive only.
        // Emit signal with offset=-1 to request status.
        job.uploadOffset = -1; // Force filePutResumeStatus.
        m_cloudDriveJobs->insert(job.jobId, job);
        filePutReplyFilter(job.jobId, err, errMsg, msg);
        return;
    } else if (err == 503 && job.type == GoogleDrive) { // 503 Service Unavailable.
        // TODO For GoogleDrive only.
        // Emit signal with offset=-1 to request status.
        job.uploadOffset = -1; // Force filePutResumeStatus.
        m_cloudDriveJobs->insert(job.jobId, job);
        filePutReplyFilter(job.jobId, err, errMsg, msg);
        return;
    } else {
        qDebug() << "CloudDriveModel::filePutResumeReplyFilter failed. Operation is aborted. jobId" << nonce << "error" << err << errMsg << msg;
        filePutReplyFilter(job.jobId, err, errMsg, msg);
        return;
    }

    // Update job.
    updateJob(job);

    // Notify job done.
    jobDone();
}

void CloudDriveModel::refreshRequestFilter(QString nonce)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Update job running flag.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit refreshRequestSignal(nonce);
}

void CloudDriveModel::schedulerTimeoutFilter()
{
//    qDebug() << "CloudDriveModel::schedulerTimeoutFilter" << QDateTime::currentDateTime().toString("d/M/yyyy hh:mm:ss.zzz");

    QString cronValue = QDateTime::currentDateTime().toString("m h d M ddd");

    // Sync scheduled items.
    loadScheduledItems(cronValue);
    if (!m_scheduledItems->isEmpty()) {
        suspendNextJob();
        syncScheduledItems();
        resumeNextJob();
    }

    // Request delta if there is no running job.
    if (runningJobCount <= 0) {
        scheduleDeltaJobs(cronValue);
    } else {
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter there is running jobs, skip delta request. runningJobCount" << runningJobCount;
    }

    emit schedulerTimeoutSignal();
}

void CloudDriveModel::initJobQueueTimer()
{
    connect(&m_jobQueueTimer, SIGNAL(timeout()), this, SLOT(proceedNextJob()) );
    m_jobQueueTimer.setInterval(1000);
    m_jobQueueTimer.start();
}

void CloudDriveModel::fixDamagedDB()
{
    QSqlQuery query(m_db);
    bool res;

    // Drop index to fix QSqlError 11 database disk image is malformed.
    res = query.exec("DROP INDEX cloud_drive_item_pk;");
    if (res) {
        qDebug() << "CloudDriveModel::fixDamagedDB DROP INDEX is done.";
    } else {
        qDebug() << "CloudDriveModel::fixDamagedDB DROP INDEX is failed. Error" << query.lastError();
    }

    res = query.exec("DELETE FROM cloud_drive_item WHERE local_path = '';");
    if (res) {
        qDebug() << "CloudDriveModel::fixDamagedDB DELETE item with empty local_path is done." << query.numRowsAffected();
    } else {
        qDebug() << "CloudDriveModel::fixDamagedDB DELETE is failed. Error" << query.lastError() << query.lastQuery();
    }

    // TODO Workaround. Delete duplicated unique item to fix QSqlError 19 indexed columns are not unique.
    QSqlQuery deleteDuplicatedPS(m_db);
    deleteDuplicatedPS.prepare("DELETE FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path AND rowid < :rowid;");

    res = query.exec("SELECT type, uid, local_path, count(*) c, max(rowid) max_rowid FROM cloud_drive_item GROUP BY type, uid, local_path HAVING count(*) > 1;");
    if (res) {
        qDebug() << "CloudDriveModel::fixDamagedDB find duplicated unique key. numRowsAffected" << query.numRowsAffected();
        QSqlRecord rec = query.record();
        while (query.next()) {
            if (query.isValid()) {
                int type = query.value(rec.indexOf("type")).toInt();
                QString uid = query.value(rec.indexOf("uid")).toString();
                QString localPath = query.value(rec.indexOf("local_path")).toString();
                int c = query.value(rec.indexOf("c")).toInt();
                int maxRowId = query.value(rec.indexOf("max_rowid")).toInt();
                qDebug() << "CloudDriveModel::fixDamagedDB find duplicated unique key record" << type << uid << localPath << c << maxRowId;

                // Delete duplicated unique key record.
                deleteDuplicatedPS.bindValue(":type", type);
                deleteDuplicatedPS.bindValue(":uid", uid);
                deleteDuplicatedPS.bindValue(":local_path", localPath);
                deleteDuplicatedPS.bindValue(":rowid", maxRowId);
                res = deleteDuplicatedPS.exec();
                if (res) {
                    qDebug() << "CloudDriveModel::fixDamagedDB DELETE duplicated unique key record is done." << deleteDuplicatedPS.numRowsAffected();
                } else {
                    qDebug() << "CloudDriveModel::fixDamagedDB DELETE duplicated unique key record is failed. Error" << deleteDuplicatedPS.lastError() << deleteDuplicatedPS.lastQuery();
                }
            } else {
                qDebug() << "CloudDriveModel::fixDamagedDB find duplicated unique key record position is invalid. ps.lastError()" << query.lastError();
            }
        }
    } else {
        qDebug() << "CloudDriveModel::fixDamagedDB find duplicated unique key failed. Error" << query.lastError() << query.lastQuery();
    }
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

    // Fix damaged DB.
    fixDamagedDB();

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
    removeJob("CloudDriveModel::initializeDB", nonce);

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

bool CloudDriveModel::testConnection(CloudDriveModel::ClientTypes type, QString uid, QString hostname, QString username, QString password, QString token, QString authHostname)
{
    return getCloudClient(type)->testConnection(uid, hostname, username, password, token, authHostname);
}

void CloudDriveModel::saveConnection(CloudDriveModel::ClientTypes type, QString uid, QString hostname, QString username, QString password, QString token)
{
    getCloudClient(type)->saveConnection(uid, hostname, username, password, token);
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

int CloudDriveModel::insertItemToDB(const CloudDriveItem item, bool suppressMessages)
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
        if (!suppressMessages) {
            qDebug() << "CloudDriveModel::insertItemToDB insert done" << item << "res" << res << "numRowsAffected" << m_insertPS.numRowsAffected();
        }
        m_db.commit();

        // Insert item to itemCache.
        m_itemCache->insert(getItemCacheKey(item.type, item.uid, item.localPath), item);
    } else {
        if (!suppressMessages) {
            qDebug() << "CloudDriveModel::insertItemToDB insert failed" << item << "res" << res << m_insertPS.lastError();
        }
    }

    return m_insertPS.numRowsAffected();
}

int CloudDriveModel::updateItemToDB(const CloudDriveItem item, bool suppressMessages)
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
        if (!suppressMessages) {
            qDebug() << "CloudDriveModel::updateItemToDB update done" << item << "res" << res << "numRowsAffected" << m_updatePS.numRowsAffected();
        }
        m_db.commit();

        // Insert item to itemCache.
        m_itemCache->insert(getItemCacheKey(item.type, item.uid, item.localPath), item);
    } else {
        if (!suppressMessages) {
            qDebug() << "CloudDriveModel::updateItemToDB update failed" << item << "res" << res << m_updatePS.lastError();
        }
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
//    qDebug() << "CloudDriveModel::uploadProgressFilter" << nonce << getOperationName(job.operation) << bytesSent << bytesTotal << job.uploadOffset;
    job.bytes = job.uploadOffset + bytesSent; // Add job.uploadOffset to support filePutResume.
    updateJob(job, false); // Omit signal as it's too noisy and freeze UI on job page.

    emit uploadProgress(nonce, bytesSent, bytesTotal);
}

void CloudDriveModel::downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
//    qDebug() << "CloudDriveModel::downloadProgressFilter" << nonce << getOperationName(job.operation) << bytesReceived << bytesTotal << job.downloadOffset;
    job.bytes = job.downloadOffset + bytesReceived; // Add job.downloadOffset to support fileGetResume.
    updateJob(job, false); // Omit signal as it's too noisy and freeze UI on job page.

    emit downloadProgress(nonce, bytesReceived, bytesTotal);
}

void CloudDriveModel::jobDone() {
    mutex.lock();
    runningJobCount--;
    runningJobCount = (runningJobCount < 0) ? 0 : runningJobCount;
    mutex.unlock();

    qDebug() << "CloudDriveModel::jobDone runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue->count() << "m_cloudDriveJobs" << m_cloudDriveJobs->count();
}

void CloudDriveModel::proceedNextJob() {
    // Proceed next job in queue. Any jobs which haven't queued will be ignored.
    if (runningJobCount > 0) {
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "CloudDriveModel::proceedNextJob waiting runningJobCount" << runningJobCount
                 << "thread pool" << QThreadPool::globalInstance()->activeThreadCount() << "/" << QThreadPool::globalInstance()->maxThreadCount()
                 << "m_jobQueue" << m_jobQueue->count() << "m_cloudDriveJobs" << m_cloudDriveJobs->count() << "m_isSuspended" << m_isSuspended;
    }

    // Emit status signal.
    emit jobQueueStatusSignal(runningJobCount, m_jobQueue->count(), m_cloudDriveJobs->count(), getItemCount());

    if (runningJobCount >= MaxRunningJobCount || m_jobQueue->count() <= 0 || m_isSuspended || m_isAborted) {
        // Check if there is no running job and is aborted.
        if (runningJobCount <= 0 && m_isAborted) {
            QApplication::quit();
        }

        return;
    }

    // TODO Check retryCount before proceed.
    QString nonce = m_jobQueue->dequeue();
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Check if it's invalid operation, then discard and return.
    if (getOperationName(job.operation) == "invalid") {
        qDebug() << "CloudDriveModel::proceedNextJob jobId" << nonce << "operation" << getOperationName(job.operation) << " Operation is discarded.";
        return;
    }

//    job.isRunning = true; // It will be set by dispatchJob() invoked by thread.
    if (job.err != 0) job.retryCount++;
    updateJob(job);

    // Check if retry hits maximum, then discard and return.
    int maxRetryCount = m_settings.value("CloudDriveModel.maxRetryCount", QVariant(3)).toInt();
    if (job.retryCount > maxRetryCount) {
        qDebug() << "CloudDriveModel::proceedNextJob jobId" << nonce << "operation" << getOperationName(job.operation) << "retry" << job.retryCount << "maxRetryCount" << maxRetryCount << " Operation is aborted.";
        return;
    }

    // Increase runningJobCount.
    mutex.lock();
    runningJobCount++;
    mutex.unlock();
    qDebug() << "CloudDriveModel::proceedNextJob jobId" << nonce << "operation" << getOperationName(job.operation) << "runningJobCount" << runningJobCount;

    // Dispatch job.
    CloudDriveModelThread *t = new CloudDriveModelThread(this);
    t->setNonce(nonce); // Set job ID to thread. It will invoke parent's dispatchJob later.
    t->setDirectInvokation(false);

    // These are blocking operations.
    switch (job.operation) {
    case LoadCloudDriveItems:
    case LoadCloudDriveJobs:
    case InitializeDB:
    case InitializeCloudClients:
    case Disconnect:
    case SyncFromLocal:
    case MigrateFilePut:
        t->setDirectInvokation(true);
        break;
    }

    // TODO NOTE Try to avoid thread panic by invoking dispatchJob() directly for any QNAM implementation methods.
    if (t->isDirectInvokation()) {
        QThreadPool::globalInstance()->start(t);
        qDebug() << "CloudDriveModel::proceedNextJob jobId" << nonce << "is started. runningJobCount" << runningJobCount << "thread pool" << QThreadPool::globalInstance()->activeThreadCount() << "/" << QThreadPool::globalInstance()->maxThreadCount();
    } else {
        qDebug() << "CloudDriveModel::proceedNextJob jobId" << nonce << "is dispatching. runningJobCount" << runningJobCount << "thread pool" << QThreadPool::globalInstance()->activeThreadCount() << "/" << QThreadPool::globalInstance()->maxThreadCount();
        dispatchJob(nonce);
    }
}

CloudDriveClient * CloudDriveModel::getCloudClient(ClientTypes type)
{
    switch (type) {
    case Dropbox:
        return dbClient;
    case SkyDrive:
        return skdClient;
    case GoogleDrive:
        return gcdClient;
    case Ftp:
        return ftpClient;
    case WebDAV:
        return webDavClient;
    default:
        qDebug() << "CloudDriveModel::getCloudClient type" << type << "is not implemented yet.";
    }

    return 0;
}

CloudDriveClient * CloudDriveModel::getCloudClient(const int type)
{
    switch (type) {
    case Dropbox:
        return dbClient;
    case SkyDrive:
        return skdClient;
    case GoogleDrive:
        return gcdClient;
    case Ftp:
        return ftpClient;
    case WebDAV:
        return webDavClient;
    default:
        qDebug() << "CloudDriveModel::getCloudClient type" << type << "is not implemented yet.";
    }

    return 0;
}

void CloudDriveModel::dispatchJob(const QString jobId)
{
    dispatchJob(m_cloudDriveJobs->value(jobId));
}

void CloudDriveModel::dispatchJob(CloudDriveJob job)
{
    // NOTE This method will be queued and invoked by main eventloop because CloudDriveModelThread.run() invoked as queue connection to its parent(CloudDriveModel).
    // Generalize cloud client.
    CloudDriveClient *cloudClient = getCloudClient(job.type);

    qDebug() << "CloudDriveModel::dispatchJob thread" << QThread::currentThread() << "job" << job.jobId << job.operation << getOperationName(job.operation) << job.type << (cloudClient == 0 ? "no client" : cloudClient->objectName());

    // Update job status and emit signal to update UI.
    job.isRunning = true;
    updateJob(job);

    switch (job.operation) {
    case LoadCloudDriveItems:
        loadCloudDriveItems(job.jobId);
        break;
    case LoadCloudDriveJobs:
        loadCloudDriveJobs(job.jobId);
        break;
    case InitializeDB:
        initializeDB(job.jobId);
        break;
    case InitializeCloudClients:
        initializeCloudClients(job.jobId);
        break;
    case FileGet:
        cloudClient->fileGet(job.jobId, job.uid, job.remoteFilePath, job.localFilePath);
        break;
    case FilePut:
        cloudClient->filePut(job.jobId, job.uid, job.localFilePath, job.remoteFilePath, job.newRemoteFileName);
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
        cloudClient->authorize(job.jobId, "");
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
        cloudClient->createFolder(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFileName);
        break;
    case MoveFile:
        cloudClient->moveFile(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFilePath, job.newRemoteFileName);
        break;
    case CopyFile:
        cloudClient->copyFile(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFilePath, job.newRemoteFileName);
        break;
    case DeleteFile:
        cloudClient->deleteFile(job.jobId, job.uid, job.remoteFilePath);
        break;
    case ShareFile:
        cloudClient->shareFile(job.jobId, job.uid, job.remoteFilePath);
        break;
    case Delta:
        cloudClient->delta(job.jobId, job.uid, false);
        break;
    case MigrateFile:
        cloudClient->browse(job.jobId, job.uid, job.remoteFilePath);
        break;
    case MigrateFilePut:
        // TODO Notify user before start migration.
        // Check if it's resumable for any file size.
        if (cloudClient->isFileGetResumable() && getCloudClient(job.targetType)->isFilePutResumable()) {
            migrateFileResume_Block(job.jobId, getClientType(job.type), job.uid, job.remoteFilePath, job.bytesTotal, getClientType(job.targetType), job.targetUid, job.newRemoteFilePath, job.newRemoteFileName);
        } else {
            migrateFile_Block(job.jobId, getClientType(job.type), job.uid, job.remoteFilePath, job.bytesTotal, getClientType(job.targetType), job.targetUid, job.newRemoteFilePath, job.newRemoteFileName);
        }
        break;
    case Disconnect:
        removeItemWithChildren(getClientType(job.type), job.uid, job.localFilePath);
        refreshRequestFilter(job.jobId);
        break;
    case SyncFromLocal:
        syncFromLocal_Block(job.jobId, getClientType(job.type), job.uid, job.localFilePath, job.remoteFilePath, job.modelIndex, job.forcePut, true, job.data);
        refreshRequestFilter(job.jobId);
        break;
    case FileGetResume:
        cloudClient->fileGetResume(job.jobId, job.uid, job.remoteFilePath, job.localFilePath, job.downloadOffset);
        break;
    case FileGetCommit:
        // TODO
        break;
    case FilePutResume:
        cloudClient->filePutResume(job.jobId, job.uid, job.localFilePath, job.remoteFilePath, job.newRemoteFileName, job.uploadId, job.uploadOffset);
        break;
    case FilePutCommit:
        cloudClient->filePutCommit(job.jobId, job.uid, job.newRemoteFilePath, job.uploadId);
        break;
    }
}

void CloudDriveModel::suspendJob(const QString jobId)
{
    // Suspend job.
    CloudDriveJob job = m_cloudDriveJobs->value(jobId);
//    job.isRunning = false;
//    updateJob(job);
//    jobDone();
//    requestJobQueueStatus();

    // Actually abort job.
    CloudDriveClient *client = getCloudClient(getClientType(job.type));
    if (client != 0) {
        client->abort(jobId);
    }
}

void CloudDriveModel::resumeJob(const QString jobId)
{
    // Enqueue job.
    CloudDriveJob job = m_cloudDriveJobs->value(jobId);
    if (!job.isRunning) {
        job.retryCount = 0; // Reset retry count.
        updateJob(job, false);
        m_jobQueue->enqueue(jobId);
    }
}

/*
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
*/

void CloudDriveModel::requestTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit requestTokenReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::authorizeRedirectFilter(QString nonce, QString url, QString redirectFrom)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit authorizeRedirectSignal(nonce, url, redirectFrom);
}

void CloudDriveModel::accessTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    updateJob(job);

    if (err == QNetworkReply::NoError) {
        // Check if job.nextJobId is specified. (From quota request)
        if (job.nextJobId != "") {
            // Insert job.nextJobId to first in queue.
            qDebug() << "CloudDriveModel::accessTokenReplyFilter insert job.nextJobId" << job.nextJobId;
            m_jobQueue->insert(0, job.nextJobId);
        } else {
            QHash<QString, QString> m_paramMap;

            // Proceed to accountInfo for authorization's accessToken.
            switch (job.type) {
            case Dropbox:
                // Dropbox provides uid but no email yet. Get email from accountInfo.
                foreach (QString s, msg.split('&')) {
                    QStringList c = s.split('=');
                    m_paramMap[c.at(0)] = c.at(1);
                }
//                qDebug() << "CloudDriveModel::accessTokenReplyFilter Dropbox uid" << m_paramMap["uid"];

                accountInfo(Dropbox, m_paramMap["uid"]);
                break;
            default:
                accountInfo(getClientType(job.type), job.uid);
            }
        }
    }

    // Notify job done.
    jobDone();

    emit accessTokenReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::accountInfoReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit accountInfoReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::quotaReplyFilter(QString nonce, int err, QString errMsg, QString msg, qint64 normalBytes, qint64 sharedBytes, qint64 quotaBytes)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    qDebug() << "CloudDriveModel::quotaReplyFilter thread" << QThread::currentThread() << "job" << job.jobId;

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit quotaReplySignal(nonce, err, errMsg, msg, normalBytes, sharedBytes, quotaBytes);
}

void CloudDriveModel::createTempPath()
{
    QFileInfo tempPathInfo(m_settings.value("temp.path", TEMP_PATH).toString());
    if (!tempPathInfo.isDir()) {
        if (QDir::home().mkpath(tempPathInfo.absoluteFilePath())) {
            qDebug() << "CloudDriveModel::createTempPath" << tempPathInfo.absoluteFilePath() << "done";
        } else {
            qDebug() << "CloudDriveModel::createTempPath" << tempPathInfo.absoluteFilePath() << "failed";
        }
    }
}

int CloudDriveModel::getSortFlag()
{
    return m_sortFlag;
}

void CloudDriveModel::setSortFlag(int sortFlag)
{
    m_sortFlag = sortFlag;

    // Sorting.
    QString msg = sortJsonText(m_cachedJsonText, m_sortFlag);

    emit browseReplySignal(createNonce(), 0, "Set sort flag", msg);
}

QString CloudDriveModel::sortJsonText(QString &jsonText, int sortFlag)
{
    QScriptEngine engine;
    QScriptValue parsedJsonObj = engine.evaluate("(" + jsonText + ")");

    qDebug() << "CloudDriveModel::sortJsonText parsedJsonObj.children.length" << parsedJsonObj.property("children").property("length").toInteger();

    QList<QScriptValue> itemList;
    for (int i = 0; i < parsedJsonObj.property("children").property("length").toInteger(); i++) {
        itemList.append(parsedJsonObj.property("children").property(i));
    }

    sortItemList(itemList, sortFlag);

    QScriptValue sortedChildren = engine.newArray();
    for (int i = 0; i < itemList.count(); i++) {
        sortedChildren.setProperty(i, itemList.at(i));
    }
    parsedJsonObj.setProperty("children", sortedChildren);
    qDebug() << "CloudDriveModel::sortJsonText sorted parsedJsonObj.children.length" << parsedJsonObj.property("children").property("length").toInteger();

    return stringifyScriptValue(engine, parsedJsonObj);
}

void CloudDriveModel::sortItemList(QList<QScriptValue> &itemList, int sortFlag)
{
    QDateTime start = QDateTime::currentDateTime();
    qDebug() << start.toString(Qt::ISODate) << "CloudDriveModel::sortItemList sortFlag" << sortFlag << "itemList.length()" << itemList.length();

    switch (sortFlag) {
    case SortByName:
        qSort(itemList.begin(), itemList.end(), nameLessThan);
        break;
    case SortByNameWithDirectoryFirst:
        qSort(itemList.begin(), itemList.end(), nameLessThanWithDirectoryFirst);
        break;
    case SortByTime:
        qSort(itemList.begin(), itemList.end(), timeGreaterThan);
        break;
    case SortByType:
        qSort(itemList.begin(), itemList.end(), typeLessThan);
        break;
    case SortBySize:
        qSort(itemList.begin(), itemList.end(), sizeGreaterThan);
        break;
    }

    QDateTime end = QDateTime::currentDateTime();
    qDebug() << end.toString(Qt::ISODate) << "CloudDriveModel::sortItemList done elapse" << start.msecsTo(end) << "ms. itemList.length()" << itemList.length();
}

QString CloudDriveModel::stringifyScriptValue(QScriptEngine &engine, QScriptValue &jsonObj)
{
    return engine.evaluate("JSON.stringify").call(QScriptValue(), QScriptValueList() << jsonObj).toString();
}
