#include "clouddrivemodel.h"
#include <QFile>
#include <QScriptEngine>
#include <QScriptValue>
#include <QThread>
#include <QDebug>

const QString CloudDriveModel::HashFilePath = "C:/CloudDriveModel.dat";
const int CloudDriveModel::MaxRunningJobCount = 3;

CloudDriveModel::CloudDriveModel(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    // Issue: Qt on Belle doesn't support QtConcurrent and QFuture.
    // Example code: QFuture<void> loadDataFuture = QtConcurrent::run(this, &CloudDriveModel::loadCloudDriveItems);

    // Load cloud drive items.
    loadCloudDriveItems();

    // Initialize cloud drive clients.
    initializeDropboxClient();
}

CloudDriveModel::~CloudDriveModel()
{
    saveCloudDriveItems();
}

void CloudDriveModel::loadCloudDriveItems() {
    // Start loading thread.
    connect(&m_thread, SIGNAL(dataLoadedSignal()), this, SLOT(dataLoadedFilter()) );
    m_thread.setHashFilePath(HashFilePath);
    m_thread.start();
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
}

void CloudDriveModel::initializeGCDClient() {
    gcdClient = new GCDClient(this);
//    connect(gcdClient, SIGNAL(uploadProgress(qint64,qint64)), SIGNAL(uploadProgress(qint64,qint64)) );
//    connect(gcdClient, SIGNAL(downloadProgress(qint64,qint64)), SIGNAL(downloadProgress(qint64,qint64)) );
    connect(gcdClient, SIGNAL(authorizeRedirectSignal(QString,QString)), SIGNAL(authorizeRedirectSignal(QString,QString)) );
    connect(gcdClient, SIGNAL(accessTokenReplySignal(int,QString,QString)), SIGNAL(accessTokenReplySignal(int,QString,QString)) );
    connect(gcdClient, SIGNAL(accountInfoReplySignal(int,QString,QString)), SIGNAL(accountInfoReplySignal(int,QString,QString)) );
    connect(gcdClient, SIGNAL(fileGetReplySignal(int,QString,QString)), SIGNAL(fileGetReplySignal(int,QString,QString)) );
    connect(gcdClient, SIGNAL(filePutReplySignal(int,QString,QString)), SIGNAL(filePutReplySignal(int,QString,QString)) );
    connect(gcdClient, SIGNAL(metadataReplySignal(int,QString,QString)), SIGNAL(metadataReplySignal(int,QString,QString)) );
}

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

QList<CloudDriveItem> CloudDriveModel::getItemList(QString localPath) {
    return m_cloudDriveItems.values(localPath);
}

bool CloudDriveModel::isConnected(QString localPath)
{
    return m_cloudDriveItems.contains(localPath);
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
    m_cloudDriveJobs.remove(nonce);
}

void CloudDriveModel::addItem(QString localPath, CloudDriveItem item)
{
    m_cloudDriveItems.insert(localPath, item);

    qDebug() << "CloudDriveModel::addItem " << item;
}

void CloudDriveModel::addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath != "") {
        qDebug() << "CloudDriveModel::addItem remove for update " << item;
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

void CloudDriveModel::updateItems(CloudDriveModel::ClientTypes type, QString localPath, QString hash)
{
    QList<CloudDriveItem> items = getItemList(localPath);
    foreach (CloudDriveItem item, items) {
        item.hash = hash;
        m_cloudDriveItems.replace(item.localPath, item);
    }

//    qDebug() << "CloudDriveModel::updateItems items" << getItemList(localPath);
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
    case GoogleDrive:
        return gcdClient->isAuthorized();
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
        break;
    case GoogleDrive:
        gcdClient->authorize();
        break;
    }
}

bool CloudDriveModel::parseAuthorizationCode(CloudDriveModel::ClientTypes type, QString text)
{
    switch (type) {
    case GoogleDrive:
        return gcdClient->parseAuthorizationCode(text);
    }

    return false;
}

void CloudDriveModel::accessToken(CloudDriveModel::ClientTypes type)
{
    switch (type) {
    case Dropbox:
        dbClient->accessToken();
        break;
    case GoogleDrive:
        gcdClient->accessToken();
        break;
    }
}

void CloudDriveModel::accountInfo(CloudDriveModel::ClientTypes type, QString uid)
{
    switch (type) {
    case Dropbox:
        dbClient->accountInfo(uid);
    }
}

void CloudDriveModel::fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath, int modelIndex)
{
    QString nonce = createNonce();

    CloudDriveJob job(nonce, FileGet, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs[nonce] = job;
    m_jobQueue.enqueue(nonce);

    proceedNextJob();
}

void CloudDriveModel::filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex)
{
    QString nonce = createNonce();

    CloudDriveJob job(nonce, FilePut, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs[nonce] = job;
    m_jobQueue.enqueue(nonce);

    proceedNextJob();
}

void CloudDriveModel::metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex)
{
    QString nonce = createNonce();

    CloudDriveJob job(nonce, Metadata, type, uid, localFilePath, remoteFilePath, modelIndex);
    job.isRunning = true;
    m_cloudDriveJobs[nonce] = job;
    m_jobQueue.enqueue(nonce);

    proceedNextJob();
}

void CloudDriveModel::fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    if (err == 0) {
        CloudDriveJob job = m_cloudDriveJobs[nonce];

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
    if (err == 0) {
        CloudDriveJob job = m_cloudDriveJobs[nonce];

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

    emit filePutReplySignal(nonce, err, errMsg, msg);
}

void CloudDriveModel::metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    if (err == 0) {
        CloudDriveJob job = m_cloudDriveJobs[nonce];

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
    runningJobCount--;
    proceedNextJob();
}

void CloudDriveModel::proceedNextJob() {
    // TODO Proceed next job in queue.
    if (runningJobCount >= MaxRunningJobCount || m_jobQueue.isEmpty()) {
        qDebug() << "CloudDriveModel::proceedNextJob waiting runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue.count() << "m_cloudDriveJobs" << m_cloudDriveJobs.count();
        return;
    }

    QString nonce = m_jobQueue.dequeue();
    qDebug() << "CloudDriveModel::proceedNextJob jobId " << nonce;
    CloudDriveJob job = m_cloudDriveJobs[nonce];

    switch (job.type) {
    case Dropbox:
        switch (job.operation) {
        case FileGet:
            dbClient->fileGet(job.jobId, job.uid, job.remoteFilePath, job.localFilePath);
            break;
        case FilePut:
            dbClient->filePut(job.jobId, job.uid, job.localFilePath, job.remoteFilePath);
            break;
        case Metadata:
            dbClient->metadata(job.jobId, job.uid, job.remoteFilePath);
        }
        break;
    }

    runningJobCount++;

    proceedNextJob();
}


void CloudDriveModel::dataLoadedFilter()
{
    // Copy thread map to member map.
    m_cloudDriveItems = m_thread.getCloudDriveItems();

    m_thread.getCloudDriveItems().clear();

    emit dataLoadedSignal();
}
