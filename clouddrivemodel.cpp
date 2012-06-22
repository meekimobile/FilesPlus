#include "clouddrivemodel.h"
#include <QFile>
#include <QScriptEngine>
#include <QScriptValue>
#include <QThread>
#include <QDebug>

const QString CloudDriveModel::HashFilePath = "C:/CloudDriveModel.dat";
const int CloudDriveModel::MaxRunningJobCount = 3;
const QString CloudDriveModel::DirtyHash = "FFFFFFFF";

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
    if (m_cloudDriveItems.isEmpty()) return;

    QFile file(HashFilePath);
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
    connect(dbClient, SIGNAL(requestTokenReplySignal(int,QString,QString)), SIGNAL(requestTokenReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(authorizeRedirectSignal(QString,QString)), SIGNAL(authorizeRedirectSignal(QString,QString)) );
    connect(dbClient, SIGNAL(accessTokenReplySignal(int,QString,QString)), SIGNAL(accessTokenReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(accountInfoReplySignal(int,QString,QString)), SIGNAL(accountInfoReplySignal(int,QString,QString)) );
    connect(dbClient, SIGNAL(fileGetReplySignal(QString,int,QString,QString)), SLOT(fileGetReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(filePutReplySignal(QString,int,QString,QString)), SLOT(filePutReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(metadataReplySignal(QString,int,QString,QString)), SLOT(metadataReplyFilter(QString,int,QString,QString)) );
    connect(dbClient, SIGNAL(createFolderReplySignal(QString,int,QString,QString)), SLOT(createFolderReplyFilter(QString,int,QString,QString)) );
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

    for(int i = 0; i <= 16; ++i)
    {
        nonce += ALPHANUMERIC.at( qrand() % ALPHANUMERIC.length() );
    }

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

QList<CloudDriveItem> CloudDriveModel::findItemList(QString localPath)
{
//    QList<CloudDriveItem> list;

//    m_cloudDriveItems.keys();
//    while (i != m_cloudDriveItems.end()) {
//        qDebug() << "CloudDriveModel::findItemList" << i.key() << i.value();

//        QString key = i.key();
//        if (!key.startsWith(localPath)) {
//            list.append(i.value());
//        } else {
//            break;
//        }
//        ++i;
//    }
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
    int removeCount = m_cloudDriveJobs.remove(nonce);

    qDebug() << "CloudDriveModel::removeJob nonce" << nonce << "removeCount" << removeCount << "m_cloudDriveJobs.count()" << m_cloudDriveJobs.count();
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
            qDebug() << "CloudDriveModel::addItem remove for update" << item;
            removeItem(type, uid, localPath);
            item = CloudDriveItem(type, uid, localPath, remotePath, hash, QDateTime::currentDateTime());
            addItem(localPath, item);
        } else {
            qDebug() << "CloudDriveModel::addItem suppress remove" << item << "addOnly=" << addOnly;
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
    int removeCount = m_cloudDriveItems.remove(localPath, item);
    qDebug() << "CloudDriveModel::remoteItem item" << item << "removeCount" << removeCount;
}

void CloudDriveModel::updateItems(CloudDriveModel::ClientTypes type, QString localPath, QString hash)
{
    QList<CloudDriveItem> items = getItemList(localPath);
    foreach (CloudDriveItem item, items) {
        item.hash = hash;
        m_cloudDriveItems.replace(item.localPath, item);
    }

    qDebug() << "CloudDriveModel::updateItems items" << getItemList(localPath);
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
    QRegExp rx("^(/*)([C-F])(.+)$");
    rx.indexIn(remoteFilePath);
    if (rx.captureCount() == 3) {
        return rx.cap(2).append(":").append(rx.cap(3));
    } else if (rx.captureCount() == 2) {
        return rx.cap(1).append(":").append(rx.cap(2));
    }
    return "";
}

QString CloudDriveModel::getDefaultRemoteFilePath(const QString &localFilePath)
{
    QRegExp rx("^([C-F])(:)(.+)$");
    rx.indexIn(localFilePath);
    if (rx.captureCount() == 3) {
        return "/" + rx.cap(1).append(rx.cap(3));
    }
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
//    } else if (info.absoluteFilePath().startsWith(":")) {
//        // TODO override remove for unwanted item.
//        qDebug() << "CloudDriveModel::cleanItem remove item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
//        m_cloudDriveItems.remove(item.localPath, item);
//        return true;
    }
    return false;
}

void CloudDriveModel::syncItems()
{
    // Queue all items for metadata requesting.
    foreach (CloudDriveItem item, m_cloudDriveItems.values()) {
        qDebug() << "CloudDriveModel::syncItems item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

        // TODO temp cleanup.
        if (cleanItem(item)) continue;

        switch (item.type) {
        case Dropbox:
            metadata(Dropbox, item.uid, item.localPath, item.remotePath, -1);
            break;
        }
    }
}

void CloudDriveModel::syncFolder(const QString localFilePath)
{
    // Queue localFilePath's items for metadata requesting.
    foreach (CloudDriveItem item, m_cloudDriveItems.values(localFilePath)) {
        qDebug() << "CloudDriveModel::syncFolder item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

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
    CloudDriveJob job(createNonce(), AccountInfo, type, "", "", "", -1);
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

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), FilePut, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

    emit proceedNextJobSignal();
}

void CloudDriveModel::metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Metadata, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs[job.jobId] = job;
    m_jobQueue.enqueue(job.jobId);

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

        // TODO create remote directory if no content.
        createFolder(type, uid, localPath, remotePath, modelIndex);

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

    // Add item with dirtyHash to avoid duplicate sync job.
    // TODO handle other clouds.
    if (job.type == Dropbox) {
        addItem(Dropbox, job.uid, job.localFilePath, job.remoteFilePath, CloudDriveModel::DirtyHash, true);
    }

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
    } else if (err == -1) {
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

    emit proceedNextJobSignal();
}

void CloudDriveModel::proceedNextJob() {
    if (m_thread.isRunning()) return;

    // Proceed next job in queue.
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
    case CreateFolder:
        switch (job.type) {
        case Dropbox:
            dbClient->createFolder(job.jobId, job.uid, job.localFilePath, job.remoteFilePath);
            break;
        }
        break;
    case RequestToken:
        switch (job.type) {
        case Dropbox:
            dbClient->requestToken();
            break;
        }
        break;
    case Authorize:
        switch (job.type) {
        case Dropbox:
            dbClient->authorize();
            break;
        }
        break;
    case AccessToken:
        switch (job.type) {
        case Dropbox:
            dbClient->accessToken();
            break;
        }
        break;
    case AccountInfo:
        switch (job.type) {
        case Dropbox:
            dbClient->accountInfo(job.uid);
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
