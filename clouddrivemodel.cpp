#include "clouddrivemodel.h"

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString CloudDriveModel::HashFilePath = "/home/user/.filesplus/CloudDriveModel.dat";
const int CloudDriveModel::MaxRunningJobCount = 3;
#else
const QString CloudDriveModel::HashFilePath = "C:/CloudDriveModel.dat";
const int CloudDriveModel::MaxRunningJobCount = 2;
#endif
const QString CloudDriveModel::DirtyHash = "FFFFFFFF";
const QStringList CloudDriveModel::restrictFileTypes = QString("SIS,SISX,DEB").split(",");

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

    // Load cloud drive items.
    loadCloudDriveItems();

    // Initialize DropboxClient
    initializeDropboxClient();
}

CloudDriveModel::~CloudDriveModel()
{
    saveCloudDriveItems();
}

QString CloudDriveModel::dirtyHash() const
{
    return DirtyHash;
}

void CloudDriveModel::loadCloudDriveItems() {
    // Enqueue LoadCloudDriveItems job.
    CloudDriveJob job(createNonce(), LoadCloudDriveItems, -1, "", "", "", -1);
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::saveCloudDriveItems() {
    // Prevent save for testing only.
    if (m_cloudDriveItems.isEmpty()) return;

    // Cleanup before save.
    cleanItems();

    QFile file(HashFilePath);
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
        out << m_cloudDriveItems;
    }

    qDebug() << "CloudDriveModel::saveCloudDriveItems" << m_cloudDriveItems.size();
}

void CloudDriveModel::initializeDropboxClient() {
    dbClient = new DropboxClient(this);
    connect(dbClient, SIGNAL(uploadProgress(QString,qint64,qint64)), SLOT(uploadProgressFilter(QString,qint64,qint64)) );
    connect(dbClient, SIGNAL(downloadProgress(QString,qint64,qint64)), SLOT(downloadProgressFilter(QString,qint64,qint64)) );
    connect(dbClient, SIGNAL(requestTokenReplySignal(QString,int,QString,QString)), SLOT(requestTokenReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(authorizeRedirectSignal(QString,QString,QString)), SLOT(authorizeRedirectFilter(QString,QString,QString)) );
    connect(dbClient, SIGNAL(accessTokenReplySignal(QString,int,QString,QString)), SLOT(accessTokenReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(accountInfoReplySignal(QString,int,QString,QString)), SLOT(accountInfoReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SLOT(fileGetReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SLOT(filePutReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SLOT(metadataReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SLOT(createFolderReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(moveFileReplySignal(QString,int,QString,QString)), SLOT(moveFileReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(copyFileReplySignal(QString,int,QString,QString)), SLOT(copyFileReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(deleteFileReplySignal(QString,int,QString,QString)), SLOT(deleteFileReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(shareFileReplySignal(QString,int,QString,QString)), SLOT(shareFileReplyFilter(QString,int,QString,QString)) );
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

QList<CloudDriveItem> CloudDriveModel::findItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    QList<CloudDriveItem> list;

    QMultiMap<QString, CloudDriveItem>::iterator i;
    i = m_cloudDriveItems.find(localPath);
    while (i != m_cloudDriveItems.end()) {
        QString key = i.key();
        CloudDriveItem item = i.value();
//        qDebug() << "CloudDriveModel::findItemWithChildren" << key << item.type << item.uid;
        if (key.startsWith(localPath) && item.type == type && item.uid == uid) {
//            qDebug() << "CloudDriveModel::findItemWithChildren add" << key << item;
            list.append(item);
        } else {
            break;
        }
        ++i;
    }

    return list;
}

QList<CloudDriveItem> CloudDriveModel::findItems(CloudDriveModel::ClientTypes type, QString uid)
{
//    qDebug() << "CloudDriveModel::findItems type" << type << "uid" << uid;
    QList<CloudDriveItem> list;

    foreach (CloudDriveItem item, m_cloudDriveItems.values()) {
        QString key = item.localPath;
//        qDebug() << "CloudDriveModel::findItems" << key << item.type << item.uid;
        if (item.type == type && item.uid == uid) {
//            qDebug() << "CloudDriveModel::findItems add" << key << item;
            list.append(item);
        }
    }

    return list;
}

QList<CloudDriveItem> CloudDriveModel::getItemList(QString localPath) {
    return m_cloudDriveItems.values(localPath);
}

bool CloudDriveModel::isConnected(QString localPath)
{
    return m_cloudDriveItems.contains(localPath);
}

bool CloudDriveModel::isDirty(QString localPath, QDateTime lastModified)
{
//    qDebug() << "CloudDriveModel::isDirty localPath" << localPath;
    if (isConnected(localPath)) {
        foreach (CloudDriveItem item, m_cloudDriveItems.values(localPath)) {
            if (item.hash == DirtyHash) {
//                qDebug() << "CloudDriveModel::isDirty item" << item.localPath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
                return true;
            } else if (lastModified > item.lastModified) {
                // It's also dirty if file/folder 's lastModified is newer. (It's changed.)
                // Signal will be emitted onyl once, next time it will be catched by item.hash==DirtyHash block above.
                qDebug() << "CloudDriveModel::isDirty item" << item.localPath << "type" << item.type << "uid" << item.uid << "lastModified" << item.lastModified << "file.lastModified" << lastModified;

                // TODO Emit signal to update item's hash.
                emit localChangedSignal(localPath);

                return true;
            }

        }
    }

    return false;
}

bool CloudDriveModel::isSyncing(QString localPath)
{
    foreach (CloudDriveJob job, m_cloudDriveJobs.values()) {
        if (job.localFilePath == localPath) {
            return true;
        }
    }

    return false;
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
    qDebug() << "CloudDriveModel::canSync localPath " << localPath << " remotePath " << remotePath;

    return remotePath != "";
}

QString CloudDriveModel::getFirstJobJson(QString localPath)
{
    foreach (CloudDriveJob job, m_cloudDriveJobs.values()) {
        if (job.localFilePath == localPath) {
            QString jsonText = getJobJson(job.jobId);

            return jsonText;
        }
    }

    return "";
}

QString CloudDriveModel::getJobJson(QString jobId)
{
    CloudDriveJob job = m_cloudDriveJobs[jobId];

    return job.toJsonText();
}

void CloudDriveModel::removeJob(QString nonce)
{
    mutex.lock();
    int removeCount = m_cloudDriveJobs.remove(nonce);
    mutex.unlock();

    qDebug() << "CloudDriveModel::removeJob nonce" << nonce << "removeCount" << removeCount << "m_cloudDriveJobs.count()" << m_cloudDriveJobs.count();
}

int CloudDriveModel::getQueuedJobCount() const
{
    return m_jobQueue.count();
}

void CloudDriveModel::cancelQueuedJobs()
{
    m_jobQueue.clear();
    // TODO Should it also clear jobs? Some QNAM requests may need it after this method call.
    m_cloudDriveJobs.clear();

    qDebug() << "CloudDriveModel::cancelQueuedJobs done m_jobQueue.count()" << m_jobQueue.count() << "m_cloudDriveJobs.count()" << m_cloudDriveJobs.count();
}

void CloudDriveModel::addItem(QString localPath, CloudDriveItem item)
{
    m_cloudDriveItems.insert(localPath, item);

    qDebug() << "CloudDriveModel::addItem" << item;
}

void CloudDriveModel::addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash, bool addOnly)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath != "") {
        // Found existing item. Remove then add.
        if (!addOnly) {
//            qDebug() << "CloudDriveModel::addItem remove for update" << item;
            removeItem(type, uid, localPath);
            item = CloudDriveItem(type, uid, localPath, remotePath, hash, QDateTime::currentDateTime());
            addItem(localPath, item);
        } else {
//            qDebug() << "CloudDriveModel::addItem suppress remove" << item << "addOnly=" << addOnly;
        }
    } else {
        // Not found, add it right away.
        item = CloudDriveItem(type, uid, localPath, remotePath, hash, QDateTime::currentDateTime());
        addItem(localPath, item);
    }
}

void CloudDriveModel::removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    CloudDriveItem item =  getItem(localPath, type, uid);
    int removeCount = m_cloudDriveItems.remove(item.localPath, item);
    qDebug() << "CloudDriveModel::removeItem item" << item << "removeCount" << removeCount;
}

void CloudDriveModel::removeItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    int removeCount = 0;

    foreach (CloudDriveItem item, findItemWithChildren(type, uid, localPath)) {
//        qDebug() << "CloudDriveModel::removeItemWithChildren item" << item;

        // TODO Remove only localPath's children which have specified type and uid.
        removeCount += m_cloudDriveItems.remove(item.localPath, item);
    }

    qDebug() << "CloudDriveModel::removeItemWithChildren removeCount" << removeCount;
}

void CloudDriveModel::removeItems(QString localPath)
{
    QList<CloudDriveItem> items = getItemList(localPath);
    foreach (CloudDriveItem item, items) {
        m_cloudDriveItems.remove(item.localPath, item);
    }

    if (getItemList(localPath).isEmpty()) {
        qDebug() << "CloudDriveModel::removeItems items" << getItemList(localPath);
    }
}

void CloudDriveModel::updateItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString hash)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    item.hash = hash;
    m_cloudDriveItems.replace(item.localPath, item);
}

void CloudDriveModel::updateItems(QString localPath, QString hash)
{
    QList<CloudDriveItem> items = getItemList(localPath);
    foreach (CloudDriveItem item, items) {
        item.hash = hash;
        m_cloudDriveItems.replace(item.localPath, item);
    }

    if (!items.isEmpty()) {
        qDebug() << "CloudDriveModel::updateItems items" << getItemList(localPath);
    }
}

int CloudDriveModel::getItemCount() const
{
    return m_cloudDriveItems.count();
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

//CloudDriveModelThread::ClientTypes CloudDriveModel::mapToThreadClientTypes(CloudDriveModel::ClientTypes type) {
//    switch (type) {
//    case Dropbox:
//        return CloudDriveModelThread::Dropbox;
//    case GoogleDrive:
//        return CloudDriveModelThread::GoogleDrive;
//    }
//}

//CloudDriveModelThread::ClientTypes CloudDriveModel::mapToThreadClientTypes(int type)
//{
//    switch (type) {
//    case Dropbox:
//        return CloudDriveModelThread::Dropbox;
//    case GoogleDrive:
//        return CloudDriveModelThread::GoogleDrive;
//    }
//}

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
//    case GoogleDrive:
//        return gcdClient->isAuthorized();
    }

    return false;
}

QStringList CloudDriveModel::getStoredUidList(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        return dbClient->getStoredUidList();
//    case GoogleDrive:
//        return gcdClient->getStoredUidList();
    }

    return QStringList();
}

int CloudDriveModel::removeUid(CloudDriveModel::ClientTypes type, QString uid)
{
    switch (type) {
    case Dropbox:
        return dbClient->removeUid(uid);
    }

    return -1;
}

void CloudDriveModel::requestJobQueueStatus()
{
    emit jobQueueStatusSignal(runningJobCount, m_jobQueue.count(), m_cloudDriveItems.count());
}

void CloudDriveModel::cleanItems()
{
    foreach (CloudDriveItem item, m_cloudDriveItems.values()) {
        cleanItem(item);
    }
}

bool CloudDriveModel::cleanItem(const CloudDriveItem &item)
{
//    qDebug() << "CloudDriveModel::cleanItem item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

    QFileInfo info(item.localPath);
    if (!info.exists()) {
        qDebug() << "CloudDriveModel::cleanItem remove item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
        m_cloudDriveItems.remove(item.localPath, item);
        return true;
    } else if (item.localPath.startsWith(":") || item.localPath == "" || item.remotePath == "") {
        // TODO override remove for unwanted item.
        qDebug() << "CloudDriveModel::cleanItem remove item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
        m_cloudDriveItems.remove(item.localPath, item);
        return true;
    }
    return false;
}

void CloudDriveModel::syncItems()
{
//    qDebug() << "CloudDriveModel::syncItems started.";

    // Queue all items for metadata requesting.
    // TODO Queue only topmost items. Suppress if its parent is already queued.
    QScriptEngine engine;
    QScriptValue sc;
    foreach (QString uidJson, getStoredUidList(Dropbox)) {
//        qDebug() << "CloudDriveModel::syncItems Dropbox uidJson" << uidJson;

        // Create json object.
        sc = engine.evaluate("(" + uidJson + ")");
        QString uid = sc.property("uid").toString();

        QString lastItemLocalPath = "";
        foreach (CloudDriveItem item, findItems(Dropbox, uid)) {
//            qDebug() << "CloudDriveModel::syncItems item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

            // TODO temp cleanup.
            if (cleanItem(item)) continue;

            // TODO suppress sync if any items' parent is in queued jobs.
            if (item.localPath.startsWith(lastItemLocalPath) && lastItemLocalPath != "") {
                qDebug() << "CloudDriveModel::syncItems suppress item localPath" << item.localPath << " as its parent already in queue.";
                continue;
            }

            switch (item.type) {
            case Dropbox:
                qDebug() << "CloudDriveModel::syncItems sync item localPath" << item.localPath;
                metadata(Dropbox, item.uid, item.localPath, item.remotePath, -1);
                break;
            }

            lastItemLocalPath = item.localPath;

            // Process events to avoid freezing UI.
            QApplication::processEvents();
        }
    }
}

void CloudDriveModel::syncItem(const QString localFilePath)
{
    // Queue localFilePath's items for metadata requesting.
    foreach (CloudDriveItem item, m_cloudDriveItems.values(localFilePath)) {
        qDebug() << "CloudDriveModel::syncItem item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

        switch (item.type) {
        case Dropbox:
            metadata(Dropbox, item.uid, item.localPath, item.remotePath, -1);
            break;
        }
    }
}

void CloudDriveModel::requestToken(CloudDriveModel::ClientTypes type)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), RequestToken, type, "", "", "", -1);
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::authorize(CloudDriveModel::ClientTypes type)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Authorize, type, "", "", "", -1);
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

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

void CloudDriveModel::accessToken(CloudDriveModel::ClientTypes type)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), AccessToken, type, "", "", "", -1);
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::accountInfo(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), AccountInfo, type, uid, "", "", -1);
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    emit proceedNextJobSignal();
}

void CloudDriveModel::fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath, int modelIndex)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), FileGet, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

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
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

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
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex, bool forcePut)
{
    // This method is invoked from dir only as file which is not found will be put right away.
    qDebug() << "CloudDriveModel::syncFromLocal" << type << uid << localPath << remotePath << modelIndex << "forcePut" << forcePut;

    QFileInfo info(localPath);
    if (info.isDir()) {
        // Sync based on local contents.

        // TODO create remote directory if no content or pending refresh metadata.
        CloudDriveItem cloudItem = getItem(localPath, type, uid);
        qDebug() << "CloudDriveModel::syncFromLocal cloudItem" << cloudItem;
        if (cloudItem.localPath == "" || cloudItem.hash == CloudDriveModel::DirtyHash) {
            createFolder(type, uid, localPath, remotePath, modelIndex);
        }

        QDir dir(info.absoluteFilePath());
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        foreach (QFileInfo item, dir.entryInfoList()) {
            QString localFilePath = item.absoluteFilePath();
            QString localHash = getItemHash(localFilePath, type, uid);
//            qDebug() << "CloudDriveModel::syncFromLocal item" << type << uid << localFilePath << localHash;

            // If dir/file don't have localHash which means it's not synced, put it right away.
            // If forcePut, put it right away.
            if (forcePut || localHash == "") {
                QString remoteFilePath = getDefaultRemoteFilePath(localFilePath);
                // Sync dir/file then it will decide whether get/put/do nothing by metadataReply.
                qDebug() << "CloudDriveModel::syncFromLocal new local item" << type << uid << localFilePath << remoteFilePath << localHash;

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
            }
        }

        // TODO avoid having below line. It caused infinite loop.
        // Update hash for itself will be requested from QML externally.
    } else {
        qDebug() << "CloudDriveModel::syncFromLocal file is not supported." << type << uid << localPath << remotePath << modelIndex << "forcePut" << forcePut;
    }
}

void CloudDriveModel::createFolder(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), CreateFolder, type, uid, localPath, remotePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localPath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::moveFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), MoveFile, type, uid, localFilePath, remoteFilePath, newLocalFilePath, newRemoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::copyFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), CopyFile, type, uid, localFilePath, remoteFilePath, newLocalFilePath, newRemoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::deleteFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), DeleteFile, type, uid, localFilePath, remoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::shareFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), ShareFile, type, uid, localFilePath, remoteFilePath, -1);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Emit signal to show cloud_wait.
    emit jobEnqueuedSignal(job.jobId, localFilePath);

    emit proceedNextJobSignal();
}

void CloudDriveModel::fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    if (err == 0) {
        // TODO generalize to support other clouds.
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + msg + ")");
        QString hash = sc.property("rev").toString();

        // TODO handle other clouds.
        if (job.type == Dropbox) {
            addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, hash);
        }

        job.isRunning = false;
        m_cloudDriveJobs[nonce] = job;
    }

    // Notify job done.
    jobDone();

    emit fileGetReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::filePutReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    if (err == 0) {
        // TODO generalize to support other clouds.
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + msg + ")");
        QString hash = sc.property("rev").toString();

        // TODO handle other clouds.
        if (job.type == Dropbox) {
            addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, hash);
        }

        job.isRunning = false;
        m_cloudDriveJobs[nonce] = job;
    } else if (err == -1) {
        // Remove failed item.
        if (job.type == Dropbox) {
            removeItem(Dropbox, job.uid, job.localFilePath);
        }
    }

    // Notify job done.
    jobDone();

    emit filePutReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    if (err == 0) {
        // TODO generalize to support other clouds.
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + msg + ")");
        QString hash = sc.property("rev").toString();

        // Don't update hash to item yet. Hash will be updated by fileGet/filePut.

        job.isRunning = false;
        m_cloudDriveJobs[nonce] = job;
    } else if (err == 202) {
        // Issue: handle 202 Nonce already in used.
        // Solution: let QML handle retry.
    }

    // Notify job done.
    jobDone();

    emit metadataReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::createFolderReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    if (err == 0) {
        // TODO generalize to support other clouds.
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + msg + ")");
        QString hash = sc.property("rev").toString();

        // TODO handle other clouds.
        if (job.type == Dropbox) {
            addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, hash);
        }
    } else if (err == 202) {
        // Forbidden {"error": " at path 'The folder '???' already exists.'"}
        // Do nothing.
    } else {
        // Remove failed item.
        if (job.type == Dropbox) {
            removeItem(Dropbox, job.uid, job.localFilePath);
        }
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    emit createFolderReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::moveFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    if (err == 0) {
        // TODO generalize to support other clouds.
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + msg + ")");
        QString hash = sc.property("rev").toString();


        // TODO Disconnect original item from clouds.
        // TODO Connect moved item to clouds.

//        // TODO handle other clouds.
//        // Connect moved local path with new remote path.
//        if (job.type == Dropbox) {
//            addItem(Dropbox, job.uid, job.localFilePath, job.newRemoteFilePath, hash);
//        }

        // Disconnect original local path.
        if (job.type == Dropbox) {
            removeItem(Dropbox, job.uid, job.localFilePath);
        }
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    emit moveFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::copyFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    if (err == 0) {
        // TODO generalize to support other clouds.
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + msg + ")");
        QString hash = sc.property("rev").toString();

//        // TODO handle other clouds.
//        // Connect copied local path with new remote path.
//        if (job.type == Dropbox) {
//            addItem(Dropbox, job.uid, job.localFilePath, job.newRemoteFilePath, hash);
//        }
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    emit copyFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::deleteFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    if (err == 0) {
        // TODO handle other clouds.
        // Disconnect deleted local path.
        if (job.type == Dropbox) {
            // TODO Remove local cloudDriveItems with it children.
            removeItemWithChildren(Dropbox, job.uid, job.localFilePath);
        }
    } else if (err == 203) {
        // Not Found {"error": "Path '???' not found"}

        // TODO handle other clouds.
        // Disconnect deleted local path.
        if (job.type == Dropbox) {
            // TODO Remove local cloudDriveItems with it children.
            removeItemWithChildren(Dropbox, job.uid, job.localFilePath);
        }
    }

    // Stop running.
    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    emit deleteFileReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::shareFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    QString url = "";
    QString expires = "";
    if (err == 0) {
        // TODO generalize to support other clouds.
        QScriptEngine engine;
        QScriptValue sc;
        sc = engine.evaluate("(" + msg + ")");
        url = sc.property("url").toString();
        expires = sc.property("expires").toString();

        job.isRunning = false;
        m_cloudDriveJobs[nonce] = job;
    } else if (err == 202) {
        // Issue: handle 202 Nonce already in used.
        // Solution: let QML handle retry.
    }

    // Notify job done.
    jobDone();

    emit shareFileReplySignal(nonce, err, errMsg, msg, url, expires);
}

void CloudDriveModel::uploadProgressFilter(QString nonce, qint64 bytesSent, qint64 bytesTotal)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];
    job.bytes = bytesSent;
    job.bytesTotal = bytesTotal;
    m_cloudDriveJobs[nonce] = job;

    emit uploadProgress(nonce, bytesSent, bytesTotal);
}

void CloudDriveModel::downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];
    job.bytes = bytesReceived;
    job.bytesTotal = bytesTotal;
    m_cloudDriveJobs[nonce] = job;

    emit downloadProgress(nonce, bytesReceived, bytesTotal);
}

void CloudDriveModel::jobDone() {
    mutex.lock();
    runningJobCount--;
    mutex.unlock();

    qDebug() << "CloudDriveModel::jobDone runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue.count() << "m_cloudDriveJobs" << m_cloudDriveJobs.count();

    emit jobQueueStatusSignal(runningJobCount, m_jobQueue.count(), m_cloudDriveItems.count());
    emit proceedNextJobSignal();
}

void CloudDriveModel::proceedNextJob() {
    // TODO delay start by process events.
    QApplication::processEvents();

    if (m_thread.isRunning()) return;

    // Proceed next job in queue. Any jobs which haven't queued will be ignored.
    qDebug() << "CloudDriveModel::proceedNextJob waiting runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue.count() << "m_cloudDriveJobs" << m_cloudDriveJobs.count();
    if (runningJobCount >= MaxRunningJobCount || m_jobQueue.count() <= 0) {
        return;
    }

    QString nonce = m_jobQueue.dequeue();
    qDebug() << "CloudDriveModel::proceedNextJob jobId " << nonce;
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    // TODO
    // If job.type==Dropbox, call DropboxClient.run() directly.
    // If job.type==Any, start thread.

    switch (job.operation) {
    case LoadCloudDriveItems:
        // Execute logic asynchronously.
        // Configure thread.
        m_thread.setHashFilePath(HashFilePath);
        m_thread.setNonce(job.jobId);
        m_thread.setRunMethod(job.operation);
        m_thread.start();
        break;
    case FileGet:
        switch (job.type) {
        case Dropbox:
            dbClient->fileGet(job.jobId, job.uid, job.remoteFilePath, job.localFilePath);
            break;
        }
        break;
    case FilePut:
        switch (job.type) {
        case Dropbox:
            dbClient->filePut(job.jobId, job.uid, job.localFilePath, job.remoteFilePath);
            break;
        }
        break;
    case Metadata:
        switch (job.type) {
        case Dropbox:
            dbClient->metadata(job.jobId, job.uid, job.remoteFilePath);
            break;
        }
        break;
    case RequestToken:
        switch (job.type) {
        case Dropbox:
            dbClient->requestToken(job.jobId);
            break;
        }
        break;
    case Authorize:
        switch (job.type) {
        case Dropbox:
            dbClient->authorize(job.jobId);
            break;
        }
        break;
    case AccessToken:
        switch (job.type) {
        case Dropbox:
            dbClient->accessToken(job.jobId);
            break;
        }
        break;
    case AccountInfo:
        switch (job.type) {
        case Dropbox:
            dbClient->accountInfo(job.jobId, job.uid);
            break;
        }
        break;
    case CreateFolder:
        switch (job.type) {
        case Dropbox:
            dbClient->createFolder(job.jobId, job.uid, job.localFilePath, job.remoteFilePath);
            break;
        }
        break;
    case MoveFile:
        switch (job.type) {
        case Dropbox:
            dbClient->moveFile(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFilePath);
            break;
        }
        break;
    case CopyFile:
        switch (job.type) {
        case Dropbox:
            dbClient->copyFile(job.jobId, job.uid, job.remoteFilePath, job.newRemoteFilePath);
            break;
        }
        break;
    case DeleteFile:
        switch (job.type) {
        case Dropbox:
            dbClient->deleteFile(job.jobId, job.uid, job.remoteFilePath);
            break;
        }
        break;
    case ShareFile:
        switch (job.type) {
        case Dropbox:
            dbClient->shareFile(job.jobId, job.uid, job.remoteFilePath);
            break;
        }
        break;
    }

    mutex.lock();
    runningJobCount++;
    mutex.unlock();

    emit proceedNextJobSignal();
}

void CloudDriveModel::threadFinishedFilter()
{
    // Notify job done.
    jobDone();
}

void CloudDriveModel::loadCloudDriveItemsFilter(QString nonce)
{
    // Copy thread map to member map.
    m_cloudDriveItems = m_thread.getCloudDriveItems();

    m_thread.getCloudDriveItems().clear();

    // RemoveJob
    removeJob(nonce);

    emit loadCloudDriveItemsFinished(nonce);
}

void CloudDriveModel::requestTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    emit requestTokenReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::authorizeRedirectFilter(QString nonce, QString url, QString redirectFrom)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    emit authorizeRedirectSignal(nonce, url, redirectFrom);
}

void CloudDriveModel::accessTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    // Get accountInfo.
    if (err == QNetworkReply::NoError) {
        QHash<QString, QString> m_paramMap;
        foreach (QString s, msg.split('&')) {
            QStringList c = s.split('=');
            m_paramMap[c.at(0)] = c.at(1);
        }

        QString uid = m_paramMap["uid"];

        qDebug() << "CloudDriveModel::accessTokenReplyFilter uid" << uid;

        // Get email from accountInfo
        switch (job.type) {
        case Dropbox:
            accountInfo(Dropbox, uid);
            break;
        }
    }

    emit accessTokenReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::accountInfoReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    job.isRunning = false;
    m_cloudDriveJobs[nonce] = job;

    // Notify job done.
    jobDone();

    emit accountInfoReplySignal(nonce, err, errMsg, msg);
}
