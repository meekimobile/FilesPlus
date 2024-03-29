#include "clouddrivemodel.h"
#include <QScriptValueIterator>
#include <QDesktopServices>
#include "cacheimageworker.h"
#include "databasemanager.h"

bool jsonNameLessThan(const QScriptValue &o1, const QScriptValue &o2)
{
//    qDebug() << "nameLessThan" << o1.property("name").toString().toLower() << o2.property("name").toString().toLower();
    return o1.property("name").toString().toLower() < o2.property("name").toString().toLower();
}

bool jsonNameLessThanWithDirectoryFirst(const QScriptValue &o1, const QScriptValue &o2)
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

bool jsonTimeGreaterThan(const QScriptValue &o1, const QScriptValue &o2)
{
    // NOTE date string is in UTC date string format. Needs to use custom format parsing.
//    qDebug() << "CloudDriveModel timeGreaterThan" << o1.property("lastModified").toString() << o2.property("lastModified").toString();
    QDateTime dt1 = QDateTime::fromString(o1.property("lastModified").toString(), "ddd, dd MMM yyyy hh:mm:ss +0000");
    if (!dt1.isValid()) {
        dt1 = QDateTime::fromString(o1.property("lastModified").toString(), Qt::ISODate);
    }
    QDateTime dt2 = QDateTime::fromString(o2.property("lastModified").toString(), "ddd, dd MMM yyyy hh:mm:ss +0000");
    if (!dt2.isValid()) {
        dt2 = QDateTime::fromString(o2.property("lastModified").toString(), Qt::ISODate);
    }
//    qDebug() << "CloudDriveModel timeGreaterThan" << dt1 << dt2;

    return dt1 > dt2;
}

bool jsonTypeLessThan(const QScriptValue &o1, const QScriptValue &o2)
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

bool jsonSizeGreaterThan(const QScriptValue &o1, const QScriptValue &o2)
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

bool nameLessThan(const CloudDriveModelItem &o1, const CloudDriveModelItem &o2)
{
//    qDebug() << "nameLessThan" << o1.name.toLower() << o2.name.toLower();
    return o1.name.toLower() < o2.name.toLower();
}

bool nameLessThanWithDirectoryFirst(const CloudDriveModelItem &o1, const CloudDriveModelItem &o2)
{
    if (o1.isDir && !o2.isDir) {
        // If dir before file, return true.
        return true;
    } else if (!o1.isDir && o2.isDir) {
        // If file before dir, return false;
        return false;
    } else {
        // If both are dir/file, compare name.
        return o1.name.toLower() < o2.name.toLower();
    }
}

bool timeGreaterThan(const CloudDriveModelItem &o1, const CloudDriveModelItem &o2)
{
//    qDebug() << "CloudDriveModel timeGreaterThan" << o1.lastModified << o2.lastModified;
    return o1.lastModified > o2.lastModified;
}

bool typeLessThan(const CloudDriveModelItem &o1, const CloudDriveModelItem &o2)
{
    if (o1.isDir && !o2.isDir) {
        // If dir before file, return true.
        return true;
    } else if (!o1.isDir && o2.isDir) {
        // If file before dir, return false;
        return false;
    } else {
        // If both are dir/file, compare type then name.
        if (o1.fileType.toLower() == o2.fileType.toLower()) {
            return o1.name.toLower() < o2.name.toLower();
        } else if (o1.fileType.toLower() < o2.fileType.toLower()) {
            return true;
        } else {
            return false;
        }
    }
}

bool sizeGreaterThan(const CloudDriveModelItem &o1, const CloudDriveModelItem &o2)
{
    if (o1.isDir && !o2.isDir) {
        // If dir before file, return true.
        return true;
    } else if (!o1.isDir && o2.isDir) {
        // If file before dir, return false;
        return false;
    } else {
        // If both the same, compare size.
        return o1.size > o2.size;
    }
}

// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString CloudDriveModel::TEMP_PATH = "/home/user/MyDocs/temp/.filesplus";
const QString CloudDriveModel::JOB_DAT_PATH = "/home/user/.filesplus/CloudDriveJobs.dat";
const int CloudDriveModel::MaxRunningJobCount = 1;
#else
const QString CloudDriveModel::TEMP_PATH = "E:/temp/.filesplus";
const QString CloudDriveModel::JOB_DAT_PATH = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + "/CloudDriveJobs.dat"; // It's in private folder.
const int CloudDriveModel::MaxRunningJobCount = 1;
#endif
const QString CloudDriveModel::DirtyHash = "FFFFFFFF";
//const QStringList CloudDriveModel::restrictFileTypes = QString("SIS,SISX,DEB").split(",");
const QStringList CloudDriveModel::restrictFileTypes;

CloudDriveModel::CloudDriveModel(QObject *parent) :
    QAbstractListModel(parent)
{
    // Issue: Qt on Belle doesn't support QtConcurrent and QFuture.
    // Example code: QFuture<void> loadDataFuture = QtConcurrent::run(this, &CloudDriveModel::loadCloudDriveItems);

    // Initialize DAT and jobQueue.
    m_cloudDriveItems = new QMultiMap<QString, CloudDriveItem>();
    m_cloudDriveJobs = new QHash<QString, CloudDriveJob>();
    m_jobQueue = new QQueue<QString>();

    // Create temp path.
    createTempPath();

    // Initialize thread pools and hash
    // TODO Refactor to avoid using QThreadPool as it causes panic in Symbian.
    m_browseThreadPool.setMaxThreadCount(1);
    m_threadHash = new QHash<QString, QThread*>();

    // Initialize DB.
    initializeDB();

    // Initialize scheduler queue.
    initScheduler();

    // Enqueue initialization jobs. Queued jobs will proceed after foldePage is loaded.
    CloudDriveJob loadCloudDriveJobsJob(createNonce(), LoadCloudDriveJobs, -1, "", "", "", -1);
    addJob(loadCloudDriveJobsJob);

    // Initialize itemCache.
    m_itemCache = new QMap<QString, CloudDriveItem>();
    m_isConnectedCache = new QMap<QString, bool>();
    m_isDirtyCache = new QMap<QString, bool>();
    m_isSyncingCache = new QMap<QString, bool>();
    m_runningJobCache = new QMap<QString, QString>();

    // Initialize cloud drive model item list.
    QHash<int, QByteArray> roles;
    int i = Qt::UserRole; // Start from user role.
    roles[++i] = "name";
    roles[++i] = "absolutePath";
    roles[++i] = "parentPath";
    roles[++i] = "size";
    roles[++i] = "lastModified";
    roles[++i] = "isDir";
    roles[++i] = "hash";
    roles[++i] = "subDirCount";
    roles[++i] = "subFileCount";
    roles[++i] = "fileType";
    roles[++i] = "isDeleted";
    roles[++i] = "isHidden";
    roles[++i] = "isReadOnly";
    roles[++i] = "source";
    roles[++i] = "alternative";
    roles[++i] = "thumbnail";
    roles[++i] = "thumbnail128";
    roles[++i] = "preview";
    roles[++i] = "downloadUrl";
    roles[++i] = "webContentLink";
    roles[++i] = "embedLink";
    roles[++i] = "isRunning";
    roles[++i] = "runningValue";
    roles[++i] = "runningMaxValue";
    roles[++i] = "runningOperation";
    roles[++i] = "isChecked";
    roles[++i] = "isDirty";
    roles[++i] = "isConnected";
    roles[++i] = "timestamp";
    setRoleNames(roles);
    m_modelItemList = new QList<CloudDriveModelItem>();

    // Initialize cloud storage clients.
    initializeCloudClients(createNonce());

    // Initialize job queue timer.
    initJobQueueTimer();
}

CloudDriveModel::~CloudDriveModel()
{
    // Save current jobs.
    saveCloudDriveJobs();

    // Clean DB.
    cleanDB();
}

int CloudDriveModel::rowCount(const QModelIndex &parent) const
{
    return m_modelItemList->count();
}

QVariant CloudDriveModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= rowCount()) {
        return QString("");
    }

    CloudDriveModelItem modelItem = m_modelItemList->at(index.row());
    QString roleName = roleNames().value(role);
//    qDebug() << "CloudDriveModel::data role" << role << roleName;
    if (roleName == "name") return modelItem.name;
    else if (roleName == "absolutePath") return modelItem.absolutePath;
    else if (roleName == "parentPath") return modelItem.parentPath;
    else if (roleName == "size") return modelItem.size;
    else if (roleName == "lastModified") return modelItem.lastModified;
    else if (roleName == "isDir") return modelItem.isDir;
    else if (roleName == "hash") return modelItem.hash;
    else if (roleName == "subDirCount") return modelItem.subDirCount;
    else if (roleName == "subFileCount") return modelItem.subFileCount;
    else if (roleName == "fileType") return modelItem.fileType;
    else if (roleName == "isDeleted") return modelItem.isDeleted;
    else if (roleName == "isHidden") return modelItem.isHidden;
    else if (roleName == "isReadOnly") return modelItem.isReadOnly;
    else if (roleName == "source") return modelItem.source;
    else if (roleName == "alternative") return modelItem.alternative;
    else if (roleName == "thumbnail") return modelItem.thumbnail;
    else if (roleName == "thumbnail128") return modelItem.thumbnail128;
    else if (roleName == "preview") return modelItem.preview;
    else if (roleName == "downloadUrl") return modelItem.downloadUrl;
    else if (roleName == "webContentLink") return modelItem.webContentLink;
    else if (roleName == "embedLink") return modelItem.embedLink;
    else if (roleName == "isRunning") return modelItem.isRunning;
    else if (roleName == "runningValue") return modelItem.runningValue;
    else if (roleName == "runningMaxValue") return modelItem.runningMaxValue;
    else if (roleName == "runningOperation") return modelItem.runningOperation;
    else if (roleName == "isChecked") return modelItem.isChecked;
    else if (roleName == "isDirty") return modelItem.isDirty;
    else if (roleName == "isConnected") return modelItem.isConnected;
    else if (roleName == "timestamp") return modelItem.timestamp;

    return QString("");
}

QVariant CloudDriveModel::get(const int index)
{
    if (index >= 0 && index < rowCount()) {
        QMap<QString,QVariant> jsonObj;
        foreach(int role, roleNames().keys()) {
            QString propertyName = QString(roleNames().value(role));
            QVariant propertyValue = data(createIndex(index,0), role);
            jsonObj[propertyName] = propertyValue;
        }
//        qDebug() << "CloudDriveModel::get" << QVariant(jsonObj);
        return QVariant(jsonObj);
    }

    return QVariant();
}

void CloudDriveModel::setProperty(const int index, QString roleName, QVariant value)
{
    bool isChanged = false;
    if (index >= 0 && index < rowCount()) {
        // Update to limited properties.
        CloudDriveModelItem item = m_modelItemList->at(index);
        if (roleName == "isRunning" && item.isRunning != value.toBool()) {
            item.isRunning = value.toBool();
            isChanged = true;
        } else if (roleName == "runningValue" && item.runningValue != value.toLongLong()) {
            item.runningValue = value.toLongLong();
            isChanged = true;
        } else if (roleName == "runningMaxValue" && item.runningMaxValue != value.toLongLong()) {
            item.runningMaxValue = value.toLongLong();
            isChanged = true;
        } else if (roleName == "runningOperation" && item.runningOperation != value.toInt()) {
            item.runningOperation = value.toInt();
            isChanged = true;
        } else if (roleName == "isChecked" && item.isChecked != value.toBool()) {
            item.isChecked = value.toBool();
            isChanged = true;
        } else if (roleName == "isDirty" && item.isDirty != value.toBool()) {
            item.isDirty = value.toBool();
            isChanged = true;
        } else if (roleName == "isConnected" && item.isConnected != value.toBool()) {
            item.isConnected = value.toBool();
            isChanged = true;
        } else if (roleName == "timestamp" && item.timestamp != value.toLongLong()) {
            item.timestamp = value.toLongLong();
            isChanged = true;
        } else if (roleName == "source" && item.source != value.toString()) {
            item.source = value.toString();
            isChanged = true;
        } else if (roleName == "alternative" && item.alternative != value.toString()) {
            item.alternative = value.toString();
            isChanged = true;
        } else if (roleName == "thumbnail" && item.thumbnail != value.toString()) {
            item.thumbnail = value.toString();
            isChanged = true;
        } else if (roleName == "thumbnail128" && item.thumbnail128 != value.toString()) {
            item.thumbnail128 = value.toString();
            isChanged = true;
        } else if (roleName == "preview" && item.preview != value.toString()) {
            item.preview = value.toString();
            isChanged = true;
        } else if (roleName == "downloadUrl" && item.downloadUrl != value.toString()) {
            item.downloadUrl = value.toString();
            isChanged = true;
        } else if (roleName == "webContentLink" && item.webContentLink != value.toString()) {
            item.webContentLink = value.toString();
            isChanged = true;
        } else if (roleName == "embedLink" && item.embedLink != value.toString()) {
            item.embedLink = value.toString();
            isChanged = true;
        }

        if (isChanged) {
//            qDebug() << "CloudDriveModel::setProperty" << index << roleName << value << "isChanged" << isChanged;
            m_modelItemList->replace(index, item);
            // Emit data changed.
            emit dataChanged(createIndex(index,0), createIndex(index, 0));
        }
    } else {
        // TODO do nothing.
    }
}

bool CloudDriveModel::removeRow(int row, const QModelIndex &parent)
{
    return removeRows(row, 1, parent);
}

bool CloudDriveModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = row; i < row+count; i++) {
        m_modelItemList->removeAt(i);
    }

    endRemoveRows();

    return true;
}

void CloudDriveModel::clear()
{
    m_modelItemList->clear();
    refreshItems();
}

QString CloudDriveModel::getRemoteParentPath()
{
    return m_remoteParentPath;
}

void CloudDriveModel::setRemoteParentPath(QString remoteParentPath)
{
    m_remoteParentPath = remoteParentPath;
}

QString CloudDriveModel::getRemoteParentPathName()
{
    return m_remoteParentPathName;
}

void CloudDriveModel::setRemoteParentPathName(QString remoteParentPathName)
{
    m_remoteParentPathName = remoteParentPathName;
}

QString CloudDriveModel::getRemoteParentParentPath()
{
    return m_remoteParentParentPath;
}

void CloudDriveModel::setRemoteParentParentPath(QString remoteParentParentPath)
{
    m_remoteParentParentPath = remoteParentParentPath;
}

int CloudDriveModel::getSelectedIndex()
{
    return m_selectedIndex;
}

void CloudDriveModel::setSelectedIndex(int index)
{
    m_selectedIndex = index;
}

QString CloudDriveModel::getSelectedRemotePath()
{
    return m_selectedRemotePath;
}

void CloudDriveModel::setSelectedRemotePath(QString remotePath)
{
    m_selectedRemotePath = remotePath;
}

QString CloudDriveModel::dirtyHash() const
{
    return DirtyHash;
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
            int len = sc.property("length").toInteger();
            for (i = 0; i < len; i++) {
                QApplication::processEvents();

                CloudDriveJob job(sc.property(i));

                // Add job to jobModel for displaying on job page.
                if (job.jobId != "") {
                    m_cloudDriveJobs->insert(job.jobId, job);
                    emit jobEnqueuedSignal(job.jobId, job.localFilePath);
                }
            }
        }
    }

    qDebug() << "CloudDriveModel::loadCloudDriveJobs" << i;

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
            QApplication::processEvents();

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
    defaultClient = new CloudDriveClient(this);
    initializeDropboxClient();
    initializeSkyDriveClient();
    initializeGoogleDriveClient();
    initializeFtpClient();
    initializeWebDAVClient();
    initializeBoxClient();
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
    connect(client, SIGNAL(migrateFilePutReplySignal(QString,int,QString,QString)), SLOT(migrateFilePutReplyFilter(QString,int,QString,QString)) ); // Added because FtpClient doesn't provide QNetworkReply.
    connect(client, SIGNAL(fileGetResumeReplySignal(QString,int,QString,QString)), SLOT(fileGetResumeReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(filePutResumeReplySignal(QString,int,QString,QString)), SLOT(filePutResumeReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(deltaReplySignal(QString,int,QString,QString)), SLOT(deltaReplyFilter(QString,int,QString,QString)) );
    connect(client, SIGNAL(logRequestSignal(QString,QString,QString,QString,int)), SIGNAL(logRequestSignal(QString,QString,QString,QString,int)) );
}

void CloudDriveModel::refreshDropboxClient()
{
    initializeDropboxClient();
}

void CloudDriveModel::initializeDropboxClient() {
    qDebug() << "CloudDriveModel::initializeDropboxClient";

    if (dbClient != 0) {
        dbClient->deleteLater();
    }

    dbClient = new DropboxClient(this);
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

void CloudDriveModel::initializeBoxClient()
{
    // TODO Generalize initialization.
    qDebug() << "CloudDriveModel::initializeBoxClient";

    if (boxClient != 0) {
        boxClient->deleteLater();
    }

    boxClient = new BoxClient(this);
    connectCloudClientsSignal(boxClient);
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
    CloudDriveItem parentItem = selectItemByPrimaryKeyFromDB(type, uid, localPath);
    if (parentItem.type == type && parentItem.uid == uid && parentItem.localPath == localPath) {
        list.append(parentItem);
    }
    qDebug() << "CloudDriveModel::findItemWithChildren localPath" << localPath << "list.count" << list.count();
    // Get it's children
    list.append(selectChildrenByPrimaryKeyFromDB(type, uid, localPath));
    qDebug() << "CloudDriveModel::findItemWithChildren localPath" << localPath << "with children list.count" << list.count();

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
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
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
        QSqlQuery qry(DatabaseManager::defaultManager().getDB());
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

void CloudDriveModel::clearLocalPathFlagCache(QMap<QString, bool> *localPathFlagCache, QString localPath, bool includeChildren)
{
    if (includeChildren) {
        QMap<QString, bool>::iterator it = localPathFlagCache->find(localPath);
        while (it != localPathFlagCache->end()) {
            QApplication::processEvents();

            QMap<QString, bool>::iterator prev = it;
            ++it;

            QString k = prev.key();
            if (k == localPath || k.startsWith(localPath + "/")) {
                //            qDebug() << "CloudDriveModel::clearLocalPathFlagCache erase cache key" << k;
                localPathFlagCache->erase(prev);
            } else {
                //            qDebug() << "CloudDriveModel::clearLocalPathFlagCache break cache key" << k;
                break;
            }
        }
    } else {
        localPathFlagCache->remove(localPath);
    }
}

void CloudDriveModel::clearConnectedRemoteDirtyCache(QString localPath, bool includeChildren)
{
    // Implement clear localPath with children.
    clearLocalPathFlagCache(m_isConnectedCache, localPath, includeChildren);
    clearLocalPathFlagCache(m_isDirtyCache, localPath, includeChildren);
    clearLocalPathFlagCache(m_isSyncingCache, localPath, includeChildren);
}

int CloudDriveModel::clearRunningJobCache(QString localPath)
{
    QFileInfo info(localPath);

    if (!info.isAbsolute() || !info.exists()) {
        return 0;
    }

    int c = m_runningJobCache->remove(localPath);
    QDir path = info.absoluteDir();
    do {
        c += m_runningJobCache->remove(path.absolutePath());
    } while (path.cdUp());

    return c;
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
    if (res) {
        emit refreshFolderCacheSignal(absPath);
    }

    return res;
}

bool CloudDriveModel::requestMoveToTrash(const QString nonce, const QString absPath)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Update job running flag.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit moveToTrashRequestSignal(nonce, absPath);

    removeJob("CloudDriveModel::requestMoveToTrash", nonce);
}

QString CloudDriveModel::getFileName(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.fileName();
}

QString CloudDriveModel::getNewFileName(const QString remotePathName)
{
    int foundIndex = findIndexByRemotePathName(remotePathName);
    if (foundIndex > -1) {
        QString newRemotePathName = "";
        QStringList tokens = remotePathName.split(".");

        if (tokens.at(0).lastIndexOf(tr("_Copy")) > -1) {
            QStringList nameTokens = tokens.at(0).split(tr("_Copy"));

            if (nameTokens.length() > 1 && nameTokens.at(1).toInt() != 0) {
                qDebug() << "CloudDriveModel::getNewFileName" << remotePathName << nameTokens << nameTokens.at(1).toInt();
                newRemotePathName += nameTokens.at(0) + tr("_Copy") + QString("%1").arg(nameTokens.at(1).toInt() + 1);
            } else {
                newRemotePathName += nameTokens.at(0) + tr("_Copy") + "2";
            }
        } else {
            newRemotePathName += tokens.at(0) + tr("_Copy");
        }

        if (tokens.length() > 1) {
            tokens.removeFirst();
            newRemotePathName += "." + tokens.join(".");
        }

        return getNewFileName(newRemotePathName);
    } else {
        return remotePathName;
    }
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
    case Box:
        return Box;
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
    } else if (typeText.indexOf(QRegExp("boxclient|box", Qt::CaseInsensitive)) != -1) {
        return Box;
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
    case Box:
        return "Box";
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
    case LoadCloudDriveJobs:
        return tr("LoadCloudDriveJobs");
    case InitializeCloudClients:
        return tr("InitializeCloudClients");
    case Disconnect:
        return tr("Disconnect");
    case DeleteLocal:
        return tr("Delete");
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
    case RemoveJobs:
        return tr("Cancel Cloud Drive Jobs");
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
    return defaultClient->formatJSONDateString(datetime);
}

QString CloudDriveModel::getPathFromUrl(QString urlString)
{
    if (urlString.startsWith("http")) {
        // Use RegExp to parse path from url string. To support # in path.
        QRegExp rx("^(http|https|ftp)://([^/]+)(/.*)");
        rx.indexIn(urlString);
        qDebug() << "CloudDriveModel::getPathFromUrl" << urlString << "capturedTexts" << rx.capturedTexts();
        if (rx.captureCount() >= 3) {
            return rx.cap(3);
        } else {
            return "";
        }
    } else {
        return urlString;
    }
}

QDateTime CloudDriveModel::parseUTCDateString(QString utcString)
{
    return QDateTime::fromString(utcString, "ddd, dd MMM yyyy hh:mm:ss +0000");
}

QDateTime CloudDriveModel::parseJSONDateString(QString jsonString)
{
    return QDateTime::fromString(jsonString, Qt::ISODate);
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

bool CloudDriveModel::isSharable(CloudDriveModel::ClientTypes type, QString uid)
{
    return (type != Ftp && type != WebDAV)
            || (type == WebDAV && getConnectionBoolProperty(type, uid, "share.enabled", true)); // Default as enabled.
}

bool CloudDriveModel::isMediaEnabled(CloudDriveModel::ClientTypes type, QString uid)
{
    return getCloudClient(type)->isMediaEnabled(uid);
}

bool CloudDriveModel::isImageUrlCachable(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isImageUrlCachable();
}

bool CloudDriveModel::isUnicodeSupported(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isUnicodeSupported();
}

bool CloudDriveModel::isDirtyBeforeSync(CloudDriveModel::ClientTypes type)
{
    QString clientObjectName = getCloudClient(type)->objectName();
    return m_settings.value(QString("%1.dirtyBeforeSync.enabled").arg(clientObjectName), false).toBool();
}

void CloudDriveModel::setMediaEnabled(CloudDriveModel::ClientTypes type, QString uid, bool flag)
{
    getCloudClient(type)->setMediaEnabled(uid, flag);
}

void CloudDriveModel::initScheduler()
{
    connect(&m_schedulerTimer, SIGNAL(timeout()), this, SLOT(schedulerTimeoutFilter()) );
    m_schedulerTimer.setInterval(m_settings.value("CloudDriveModel.schedulerTimer.interval", 60000).toInt());
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

QString CloudDriveModel::getRunningJob(QString localPath)
{
    if (m_runningJobCache->contains(localPath)) {
        return m_runningJobCache->value(localPath);
    }

    foreach (CloudDriveJob job, m_cloudDriveJobs->values()) {
        if (job.localFilePath == localPath && job.isRunning) {
            qDebug() << "CloudDriveModel::getRunningJob" << localPath << job.jobId;
            m_runningJobCache->insert(localPath, job.jobId);
            return job.jobId;
        } else if (job.localFilePath.startsWith(localPath + "/") && job.isRunning) {
            qDebug() << "CloudDriveModel::getRunningJob" << localPath << job.jobId;
            m_runningJobCache->insert(localPath, job.jobId);
            return job.jobId;
        }
    }

    return "";
}

QVariant CloudDriveModel::getJobJson(QString jobId)
{
    CloudDriveJob job = m_cloudDriveJobs->value(jobId);

    return job.toJson();
}

void CloudDriveModel::addJob(CloudDriveJob job, bool runAsNextJob, bool emitJobEnqueuedSignal)
{
    mutex.lock();
    m_cloudDriveJobs->insert(job.jobId, job);
    if (runAsNextJob) {
        m_jobQueue->insert(0, job.jobId);
    } else {
        m_jobQueue->enqueue(job.jobId);
    }
    mutex.unlock();

    if (job.localFilePath != "") {
        clearConnectedRemoteDirtyCache(job.localFilePath);
    }

    if (emitJobEnqueuedSignal) {
        emit jobEnqueuedSignal(job.jobId, job.localFilePath);
    }
}

void CloudDriveModel::updateJob(CloudDriveJob job, bool emitJobUpdatedSignal, bool clearCache)
{
    if (clearCache) {
        clearConnectedRemoteDirtyCache(job.localFilePath);
    }
    if (job.isRunning) {
        job.lastStartedTime = QDateTime::currentDateTime();
    } else {
        job.lastStoppedTime = QDateTime::currentDateTime();
    }
    if (job.err == 0) {
        // Reset retry count if job is done successfully.
        job.retryCount = 0;
    }
    mutex.lock();
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

    if (!m_cloudDriveJobs->contains(nonce)) {
        qDebug() << "CloudDriveModel::removeJob caller" << caller << "nonce" << nonce << "jobId is not found. Operation is aborted";
        return;
    }

    qDebug() << "CloudDriveModel::removeJob caller" << caller << "nonce" << nonce << "started.";

    qDebug() << "CloudDriveModel::removeJob caller" << caller << "nonce" << nonce << "managing job's data.";
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    clearConnectedRemoteDirtyCache(job.localFilePath);
    clearRunningJobCache(job.localFilePath);
    clearConnectedRemoteDirtyCache(job.newLocalFilePath);
    clearRunningJobCache(job.newLocalFilePath);
    mutex.lock();
    int removeCount = m_cloudDriveJobs->remove(nonce);
    mutex.unlock();

    // Remove temp file if it exists.
    qDebug() << "CloudDriveModel::removeJob caller" << caller << "nonce" << nonce << "removing job's temp file.";
    QString tempFilePath = m_settings.value("temp.path", TEMP_PATH).toString() + "/" + nonce;
    if (QFileInfo(tempFilePath).exists()) {
        QFile(tempFilePath).remove();
        qDebug() << "CloudDriveModel::removeJob caller" << caller << "temp file" << tempFilePath << "is removed";
    }

    if (removeCount > 0) {
        emit jobRemovedSignal(nonce);
    }

    // Remove thread from hash.
    m_threadHash->remove(nonce);

    qDebug() << "CloudDriveModel::removeJob caller" << caller << "nonce" << nonce << "removeCount" << removeCount << "m_threadHash->count()" << m_threadHash->count() << "m_cloudDriveJobs->count()" << m_cloudDriveJobs->count();
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
    // Enqueue job.
    CloudDriveJob job(createNonce(), RemoveJobs, -1, "", "", "", -1);
    addJob(job, true);
}

void CloudDriveModel::removeJobs(bool removeAll)
{
    suspendNextJob();

    qDebug() << "CloudDriveModel::removeJobs removeAll" << removeAll;
    // Remove unqueued jobs.
    foreach (CloudDriveJob job, m_cloudDriveJobs->values()) {
        QApplication::processEvents();

        if (removeAll || !job.isRunning) {
            removeJob("CloudDriveModel::removeJobs", job.jobId);
        }
    }

    resumeNextJob();
}

void CloudDriveModel::addItem(CloudDriveItem item)
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
    clearConnectedRemoteDirtyCache(item.localPath);
}

void CloudDriveModel::addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash, int syncDirection, bool addOnly)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    if (item.localPath != "") {
        // Found existing item. Update it.
        if (!addOnly) {
            qDebug() << "CloudDriveModel::addItem found." << item;
            item = CloudDriveItem(type, uid, localPath, remotePath, hash, QDateTime::currentDateTime());
            item.syncDirection = syncDirection;
            updateItemToDB(item);
        }
    } else {
        qDebug() << "CloudDriveModel::addItem not found.";
        // Not found, add it right away.
        item = CloudDriveItem(type, uid, localPath, remotePath, hash, QDateTime::currentDateTime());
        item.syncDirection = syncDirection;
        insertItemToDB(item);
    }

    // Remove cache for furthur refresh.
    clearConnectedRemoteDirtyCache(localPath);
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

//    int removeCount = 0;
//    int deleteCount = 0;

//    foreach (CloudDriveItem item, findItemWithChildren(type, uid, localPath)) {
//        qDebug() << "CloudDriveModel::removeItemWithChildren item" << item;

//        // Process events to avoid freezing UI.
//        QApplication::processEvents(QEventLoop::AllEvents, 50);

//        // TODO Remove only localPath's children which have specified type and uid.
//        removeCount += m_cloudDriveItems->remove(item.localPath, item);
//        deleteCount += deleteItemToDB(type, uid, item.localPath);

//        // Remove cache for furthur refresh.
//        clearConnectedRemoteDirtyCache(item.localPath);
//    }

//    qDebug() << "CloudDriveModel::removeItemWithChildren removeCount" << removeCount << "deleteCount" << deleteCount;

    // TODO Remove item with children with 1 command. It must also clear cache.
    deleteItemWithChildrenFromDB(type, uid, localPath);
    // Remove cache for furthur refresh.
    clearConnectedRemoteDirtyCache(localPath);
}

void CloudDriveModel::removeItems(QString localPath)
{
    QList<CloudDriveItem> items = getItemList(localPath);
    foreach (CloudDriveItem item, items) {
        m_cloudDriveItems->remove(item.localPath, item);

        // Remove cache for furthur refresh.
        clearConnectedRemoteDirtyCache(item.localPath);
    }

    if (getItemList(localPath).isEmpty()) {
        qDebug() << "CloudDriveModel::removeItems items" << getItemList(localPath);
    }
}

int CloudDriveModel::removeItemByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath)
{
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
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
    if (item.localPath != "") {
        m_cloudDriveItems->remove(item.localPath, item); // Remove found item.
        item.hash = hash;
    }
    int updateCount = updateItemToDB(item);

    // Remove cache for furthur refresh.
    clearConnectedRemoteDirtyCache(localPath);

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
    clearConnectedRemoteDirtyCache(localPath);

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
        clearConnectedRemoteDirtyCache(item.localPath);
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
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
    qry.prepare("UPDATE cloud_drive_item SET cron_exp = :cron_exp WHERE type = :type AND uid = :uid AND local_path = :local_path");
    qry.bindValue(":cron_exp", cronExp);
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":local_path", localPath);
    bool res = qry.exec();
    if (res) {
        DatabaseManager::defaultManager().getDB().commit();
        m_itemCache->remove(getItemCacheKey(type, uid, localPath));
        qDebug() << "CloudDriveModel::updateItemCronExpToDB done" << type << uid << localPath << cronExp << "numRowsAffected" << qry.numRowsAffected();
        return qry.numRowsAffected();
    } else {
        qDebug() << "CloudDriveModel::updateItemCronExpToDB failed" << type << uid << localPath << cronExp << "error" << qry.lastError();
        return qry.numRowsAffected();
    }
}

QString CloudDriveModel::getItemCronExp(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    QString cronExp = "";

    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
    qry.prepare("SELECT cron_exp FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":local_path", localPath);
    bool res = qry.exec();
    if (res && qry.next()) {
        cronExp = qry.value(qry.record().indexOf("cron_exp")).toString();
        qDebug() << "CloudDriveModel::getItemCronExp done" << type << uid << localPath << cronExp;
    } else {
        qDebug() << "CloudDriveModel::getItemCronExp failed" << type << uid << localPath << "error" << qry.lastError();
    }

    return cronExp;
}

void CloudDriveModel::dirtyScheduledItems(QString cronValue)
{
    // Match each item in next minute. Cache matched items for next firing.
    QString cronExp;
    int type;
    QString uid;
    QString localPath;
    QSqlQuery ps(DatabaseManager::defaultManager().getDB());
    ps.prepare("SELECT * FROM cloud_drive_item WHERE cron_exp <> ''");
    if (ps.exec()) {
        while (ps.next()) {
            // Process pending events.
            QApplication::processEvents();

            if (ps.isValid()) {
                type = ps.value(ps.record().indexOf("type")).toInt();
                uid = ps.value(ps.record().indexOf("uid")).toString();
                localPath = ps.value(ps.record().indexOf("local_path")).toString();
                cronExp = ps.value(ps.record().indexOf("cron_exp")).toString();
                // Match cronExp with cronValue. Ex. cronValue 0 8 12 10 5 <-- 8:00 on 12-Oct Friday
                if (matchCronExp(cronExp, cronValue)) {
                    qDebug() << "CloudDriveModel::dirtyScheduledItems schedule sync item" << type << getCloudName(type) << uid << localPath;
                    updateItem(getClientType(type), uid, localPath, DirtyHash);
                } else {
//                    qDebug() << "CloudDriveModel::dirtyScheduledItems discard item" << type << getCloudName(type) << uid << localPath;
                }
            } else {
                qDebug() << "CloudDriveModel::dirtyScheduledItems record position is invalid. ps.lastError()" << ps.lastError();
                break;
            }
        }
    }
}

void CloudDriveModel::syncDirtyItems()
{
    if (!m_settings.value("CloudDriveModel.syncDirtyItems.enabled", QVariant(false)).toBool()) {
        return;
    }

    CloudDriveItem lastItem;
    QSqlQuery ps(DatabaseManager::defaultManager().getDB());
    ps.prepare("SELECT * FROM cloud_drive_item WHERE hash = :hash ORDER BY type, uid, local_path;");
    ps.bindValue(":hash", DirtyHash);
    foreach (CloudDriveItem item, getItemListFromPS(ps)) {
        // Process pending events.
        QApplication::processEvents();

        qDebug() << "CloudDriveModel::syncDirtyItems item" << item;
        if (!cleanItem(item)) {
            // Suppress sync if any items' parent is in queued jobs.
            if (item.type == lastItem.type
                    && item.uid == lastItem.uid
                    && item.localPath.startsWith(lastItem.localPath)
                    && lastItem.localPath != "") {
                qDebug() << "CloudDriveModel::syncDirtyItems suppress item" << item << "as its parent already in queue.";
                continue;
            }

            // Sync dirty item.
            metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1);
        }

        lastItem = item;
    }
}

int CloudDriveModel::updateItemSyncDirection(CloudDriveModel::ClientTypes type, QString uid, QString localPath, CloudDriveModel::SyncDirections syncDirection)
{
    int c = 0;
    bool res = false;
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());

    // Update itself.
    qry.prepare("UPDATE cloud_drive_item SET sync_direction = :sync_direction WHERE type = :type AND uid = :uid AND local_path = :local_path");
    qry.bindValue(":sync_direction", syncDirection);
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":local_path", localPath);
    res = qry.exec();
    if (res) {
        qDebug() << "CloudDriveModel::updateItemSyncDirection done" << type << uid << localPath << syncDirection << "numRowsAffected" << qry.numRowsAffected();
        c += qry.numRowsAffected();
    } else {
        qDebug() << "CloudDriveModel::updateItemSyncDirection failed" << type << uid << localPath << syncDirection << "error" << qry.lastError();
        c += qry.numRowsAffected();
    }

    // Update its children.
    qry.prepare("UPDATE cloud_drive_item SET sync_direction = :sync_direction WHERE type = :type AND uid = :uid AND local_path LIKE :local_path || '/%' ");
    qry.bindValue(":sync_direction", syncDirection);
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":local_path", localPath);
    res = qry.exec();
    if (res) {
        qDebug() << "CloudDriveModel::updateItemSyncDirection children done" << type << uid << localPath << syncDirection << "numRowsAffected" << qry.numRowsAffected();
        c += qry.numRowsAffected();
    } else {
        qDebug() << "CloudDriveModel::updateItemSyncDirection children failed" << type << uid << localPath << syncDirection << "error" << qry.lastError();
        c += qry.numRowsAffected();
    }

    DatabaseManager::defaultManager().getDB().commit();
    removeItemCacheWithChildren(type, uid, localPath);

    return c;
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
        // Process pending events.
        QApplication::processEvents();

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
//    qDebug() << "CloudDriveModel::matchCronExp start cronExp" << cronExp << "cronValue" << cronValue;

    QStringList cronExpList = cronExp.split(" ");
    QStringList cronValueList = cronValue.split(" ");

    if (cronExpList.length() != cronValueList.length()) {
//        qDebug() << "CloudDriveModel::matchCronExp failed cronValueList" << cronValueList << "length" << cronValueList.length();
        return false;
    }

    QString e;
    QString v;
    for (int i = 0; i < cronExpList.length(); i++) {
        e = cronExpList.at(i);
        v = cronValueList.at(i);
//        qDebug() << "CloudDriveModel::matchCronExp i" << i << "e" << e << "v" << v;

        // TODO Support - and , in e.
        if (e == "*") {
            // continue.
        } else if (e.indexOf("*/") == 0) {
            int div = e.mid(e.indexOf("/")+1).toInt();
            int mod = v.toInt() % div;
            if (mod == 0) {
                // continue.
            } else {
//                qDebug() << "CloudDriveModel::matchCronExp failed i" << i << "e" << e << "v" << v << "div" << div << "mod" << mod;
                return false;
            }
        } else if (e.toInt() == v.toInt()) {
            // continue.
        } else {
//            qDebug() << "CloudDriveModel::matchCronExp failed i" << i << "e" << e << "v" << v;
            return false;
        }
    }

//    qDebug() << "CloudDriveModel::matchCronExp success cronExp" << cronExp << "cronValue" << cronValue;
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

QVariantList CloudDriveModel::getItemListJson(QString localPath)
{
    QVariantList jsonArray;
    foreach (CloudDriveItem item, getItemList(localPath)) {
        jsonArray.append( item.toJson() );
    }

    return jsonArray;
}

QVariantList CloudDriveModel::getItemListJsonByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath)
{
    QVariantList jsonArray;
    foreach (CloudDriveItem item, findItemsByRemotePath(type, uid, remotePath)) {
        jsonArray.append( item.toJson() );
    }

    return jsonArray;
}

QVariant CloudDriveModel::getItemJson(QString localPath, CloudDriveModel::ClientTypes type, QString uid)
{
    CloudDriveItem item = getItem(localPath, type, uid);
    return item.toJson();
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
    return dbClient->isAuthorized()
            || skdClient->isAuthorized()
            || gcdClient->isAuthorized()
            || ftpClient->isAuthorized()
            || webDavClient->isAuthorized()
            || boxClient->isAuthorized();
}

bool CloudDriveModel::isAuthorized(CloudDriveModel::ClientTypes type)
{
    return getCloudClient(type)->isAuthorized();
}

QStringList CloudDriveModel::getStoredUidList()
{
    QStringList uidList;

    // TODO Generalize.
    // ISSUE Martin has got missing ftp uid in list.
    // TODO Refactor to return QVariantList which will be JSON automatically in QML.
    uidList.append(dbClient->getStoredUidList());
    uidList.append(gcdClient->getStoredUidList());
    uidList.append(skdClient->getStoredUidList());
    uidList.append(boxClient->getStoredUidList());
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

QString CloudDriveModel::getEmail(CloudDriveModel::ClientTypes type, QString uid)
{
    return getCloudClient(type)->getEmail(uid);
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
    } else if (!info.isAbsolute()) {
        isInvalid = true;
    } else if (item.localPath.startsWith(":") || item.localPath == "" || item.remotePath == "") {
        // TODO override remove for unwanted item.
        isInvalid = true;
    }

    if (isInvalid) {
        qDebug() << "CloudDriveModel::cleanItem remove item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
        m_cloudDriveItems->remove(item.localPath, item);
        deleteItemWithChildrenFromDB(getClientType(item.type), item.uid, item.localPath);
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
    syncItems(GoogleDrive);
    syncItems(SkyDrive);
    syncItems(Ftp);
    syncItems(WebDAV);

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
        CloudDriveClient *client = getCloudClient(getClientType(item.type));
        if (client->isAuthorized() && client->getStoredUid(item.uid) != "") {
            metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1);
        } else {
            qDebug() << "CloudDriveModel::syncItem skipped and disconnecting item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
            deleteItemWithChildrenFromDB(getClientType(item.type), item.uid, item.localPath);
        }
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
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
    qry.prepare("SELECT * FROM cloud_drive_item where type = :type AND uid = :uid AND remote_path = :remote_path");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remotePath);

    bool res = false;
    foreach (CloudDriveItem item, getItemListFromPS(qry)) {
        qDebug() << "CloudDriveModel::syncItemByRemotePath item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
        CloudDriveClient *client = getCloudClient(getClientType(item.type));
        if (client->isAuthorized() && client->getStoredUid(item.uid) != "") {
            if (newHash != "") {
                qDebug() << "CloudDriveModel::syncItemByRemotePath updating hash" << newHash << "to item" << item;
                updateItem(type, uid, item.localPath, newHash);
            }
            metadata(getClientType(item.type), item.uid, item.localPath, item.remotePath, -1, forcePut, forceGet);
            res = true;
        } else {
            qDebug() << "CloudDriveModel::syncItemByRemotePath skipped and disconnecting item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;
            deleteItemWithChildrenFromDB(getClientType(item.type), item.uid, item.localPath);
        }
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
    addJob(job);
}

void CloudDriveModel::authorize(CloudDriveModel::ClientTypes type)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Authorize, type, "", "", "", -1);
    addJob(job);
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
    addJob(job);
}

void CloudDriveModel::refreshToken(CloudDriveModel::ClientTypes type, QString uid, QString nextNonce)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), RefreshToken, type, uid, "", "", -1);
    job.nextJobId = nextNonce;
    addJob(job, true);
}

void CloudDriveModel::accountInfo(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), AccountInfo, type, uid, "", "", -1);
    addJob(job);
}

void CloudDriveModel::quota(CloudDriveModel::ClientTypes type, QString uid)
{
    if (type == Ftp
            || (type == WebDAV && !getConnectionBoolProperty(type, uid, "quota.enabled", true))) { // Default as enabled.
        CloudDriveJob job(createNonce(), Quota, type, uid, "", "", -1);
        m_cloudDriveJobs->insert(job.jobId, job);
        quotaReplyFilter(job.jobId, 0, "", "{ }", 0, 0, 0);
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), Quota, type, uid, "", "", -1);
    addJob(job);
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
    job.downloadOffset = 0;
    job.bytesTotal = remoteFileSize;
    addJob(job);
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
    job.newRemoteFileName = remoteFileName;
    job.uploadOffset = 0;
    job.bytesTotal = QFileInfo(localFilePath).size();
    addJob(job);
}

void CloudDriveModel::metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex, bool forcePut, bool forceGet)
{
    if (localFilePath == "") {
        qDebug() << "CloudDriveModel::metadata localFilePath" << localFilePath << " is empty, can't sync.";
        return;
    }

    if (!QFileInfo(localFilePath).isAbsolute()) {
        qDebug() << "CloudDriveModel::metadata localFilePath" << localFilePath << " is invalid, can't sync.";
        return;
    }

    if (remoteFilePath == "") {
        qDebug() << "CloudDriveModel::metadata remoteFilePath" << remoteFilePath << " is empty, can't sync.";
        return;
    }

    if (!m_settings.value("CloudDriveModel.metadata.root.connection.enabled", QVariant(false)).toBool()
        && (isRemoteRoot(type, uid, remoteFilePath) || remoteFilePath == "/") ) {
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
    job.forcePut = forcePut;
    job.forceGet = forceGet;
    addJob(job);
}

void CloudDriveModel::browse(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath)
{
    if (!getCloudClient(type)->isAuthorized(uid)) {
        emit browseReplySignal(createNonce(), -1, tr("User ID is not authorized."), "", false);
        return;
    }

    // Reset sortFlag if requested from drive page or remoteParentPath changed.
    if (remoteFilePath == "" || m_remoteParentPath != remoteFilePath) {
        m_sortFlag = -1;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), Browse, type, uid, "", remoteFilePath, -1);
    job.isRunning = true;
    // Add job to jobModel for displaying on job page.
    m_cloudDriveJobs->insert(job.jobId, job);
    emit jobEnqueuedSignal(job.jobId, "");

    // Force start thread.
    CloudDriveModelThread *t = new CloudDriveModelThread(this);
    t->setNonce(job.jobId); // Set job ID to thread. It will invoke parent's dispatchJob later.
    t->setDirectInvokation(false);
    m_browseThreadPool.start(t);
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

void CloudDriveModel::syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, int modelIndex, bool forcePut, QStringList remotePathList)
{
    qDebug() << "----- CloudDriveModel::syncFromLocal -----" << type << uid << localPath << "remoteParentPath" << remoteParentPath << "forcePut" << forcePut << "remotePathList.length()" << remotePathList.length();
    if (m_settings.value("Logging.enabled", false).toBool()) {
        qDebug() << "CloudDriveModel::syncFromLocal" << type << uid << localPath << "remotePathList" << remotePathList;
    }

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
    job.remotePathList = remotePathList;
    addJob(job);
}

void CloudDriveModel::syncFromLocal_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, int modelIndex, bool forcePut, bool isRootLocalPath, QStringList remotePathList)
{
    // This method is invoked from dir only as file which is not found will be put right away.
    qDebug() << "----- CloudDriveModel::syncFromLocal_Block -----" << nonce << type << uid << localPath << remoteParentPath << modelIndex << "forcePut" << forcePut << "remotePathList.length()" << remotePathList.length();
    if (m_settings.value("Logging.enabled", false).toBool()) {
        qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << type << uid << localPath << "remotePathList" << remotePathList;
    }

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
    if (!cloudClient->isAuthorized()) {
        return;
    }

//    qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "remotePathList" << remotePathList;

    QFileInfo info(localPath);
    if (info.isDir()) {
        // Sync based on local contents.

        // TODO create remote directory if no content or pending refresh metadata.
        CloudDriveItem parentCloudDriveItem = getItem(localPath, type, uid);
        if (parentCloudDriveItem.syncDirection == SyncBackward) {
            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "suppress createFolder syncDirection" << parentCloudDriveItem.syncDirection;
        } else if (parentCloudDriveItem.localPath == "" || parentCloudDriveItem.hash == CloudDriveModel::DirtyHash) {
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
            m_cloudDriveJobs->remove(job.jobId); // Remove dummy job after used.
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
                addItem(type, uid, localPath, createdRemotePath, hash, parentCloudDriveItem.syncDirection);

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
            if (!cloudDriveItem.isValid()) {
                cloudDriveItem.syncDirection = parentCloudDriveItem.syncDirection;
            }
            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "item" << type << uid << localFilePath << "cloudDriveItem" << cloudDriveItem.toJsonText();

            // TODO Cache isSyncing(type, uid, localPath)
            bool isNewItem = cloudDriveItem.syncDirection != SyncBackward
                    && !isSyncing(type, uid, localFilePath)
                    && cloudDriveItem.localPath == ""
                    && (cloudDriveItem.hash == "" || cloudDriveItem.remotePath == "");
            bool isDeletedItem = cloudDriveItem.remotePath != ""
                    && remotePathList.indexOf(cloudDriveItem.remotePath) == -1
                    && remotePathList.indexOf("*") == -1;
//            qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "cloudDriveItem.remotePath" << cloudDriveItem.remotePath << "isNewItem" << isNewItem << "isDeletedItem" << isDeletedItem;

            // If dir/file don't have localHash which means it's not synced, put it right away.
            // If forcePut, put it right away.
            if (forcePut || isNewItem) {
                // Sync dir/file then it will decide whether get/put/do nothing by metadataReply.
                qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "new local item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "parentCloudDriveItem.remotePath" << parentCloudDriveItem.remotePath << "isNewItem" << isNewItem;

                if (item.isDir()) {
                    // Drilldown local dir recursively.
                    // NOTE Don't pass remotePathList will cause removing items.
                    syncFromLocal(type, uid, localFilePath, parentCloudDriveItem.remotePath, -1, forcePut);
                } else {
                    // Put file to remote parent path.
                    // TODO Add more comparing logic whether upload or just update hash.
                    filePut(type, uid, localFilePath, parentCloudDriveItem.remotePath, getFileName(localFilePath), -1);
                }
            } else if (isDeletedItem) {
                // Configurable file removing if it's deleted item (there is remotePathList and cloudDriveItem.remotePath not in list).
                if (cloudDriveItem.syncDirection != SyncForward
                        && m_settings.value("CloudDriveModel.syncFromLocal.deleteLocalFile.enabled", true).toBool()) {
                    qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "trash local item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "cloudDriveItem.remotePath" << cloudDriveItem.remotePath << "isDeletedItem" << isDeletedItem;
                    deleteLocal(type, uid, localFilePath);
                } else {
                    qDebug() << "CloudDriveModel::syncFromLocal_Block" << nonce << "remove link to existing local item" << type << uid << localFilePath << "cloudDriveItem.hash" << cloudDriveItem.hash << "cloudDriveItem.remotePath" << cloudDriveItem.remotePath << "isDeletedItem" << isDeletedItem;
                    disconnect(type, uid, localFilePath);
                }
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
    if (newRemoteFolderName.trimmed() == "") {
        qDebug() << "CloudDriveModel::createFolder newRemoteFolderName" << newRemoteFolderName << "is empty. Operation is aborted.";
        return;
    }

    if (remoteParentPath == "") {
        qDebug() << "CloudDriveModel::createFolder remoteParentPath" << remoteParentPath << "is empty. Operation is aborted.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), CreateFolder, type, uid, localPath, remoteParentPath, -1);
    job.newRemoteFileName = newRemoteFolderName.trimmed();
    addJob(job);
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
    if (!cloudClient->isAuthorized()) {
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

void CloudDriveModel::migrateFilePut_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName)
{
    qDebug() << "----- CloudDriveModel::migrateFilePut_Block -----" << nonce << type << uid << remoteFilePath << remoteFileSize << targetType << targetUid << targetRemoteParentPath << targetRemoteFileName;

    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    CloudDriveClient *sourceClient = getCloudClient(type);
    CloudDriveClient *targetClient = getCloudClient(targetType);
    QString tempFilePath = m_settings.value("temp.path", TEMP_PATH).toString() + "/" + nonce;

    // Migrates using synchronous methods.
    // TODO Limit migration size to fit in RAM. Larger file needs to be downloaded to local before proceed uploading.
    if (sourceClient->isFileGetResumable(remoteFileSize)) {
        // Get each chunk to file.
        while (job.downloadOffset < remoteFileSize && !m_isAborted) {
            QApplication::processEvents();

            QIODevice *source = sourceClient->fileGet(nonce, uid, remoteFilePath, job.downloadOffset, true);
            if (source == 0) {
                migrateFilePutReplyFilter(nonce, -1, "Service is not implemented or host is not accessible.", "{ }");
                return;
            }
            // Handle error.
            // TODO Check type before cast. Use qobject_cast. It will return NULL if casting failed.
            QNetworkReply *sourceReply = dynamic_cast<QNetworkReply *>(source);
            if (sourceReply != 0 && sourceReply->error() != QNetworkReply::NoError) {
                migrateFilePutReplyFilter(nonce, sourceReply->error(), sourceReply->errorString(), sourceReply->readAll());
                return;
            }
            qDebug() << "CloudDriveModel::migrateFilePut_Block" << nonce << "chunk is downloaded. size" << source->size() << "bytesAvailable" << source->bytesAvailable() << "job" << job.toJsonText();

            // Stream source to a file on localPath.
            // TODO Reuse fileGetReplySaveChunk()
            qDebug() << "CloudDriveModel::migrateFilePut_Block" << nonce << "tempFilePath" << tempFilePath;
            qint64 totalBytes = sourceClient->writeToFile(source, tempFilePath, job.downloadOffset);
            if (totalBytes >= 0) {
                qDebug() << "CloudDriveModel::migrateFilePut_Block" << nonce << "chunk is written at offset" << job.downloadOffset << "totalBytes"  << totalBytes << "to" << tempFilePath;
            } else {
                qDebug() << "CloudDriveModel::migrateFilePut_Block" << nonce << "can't open tempFilePath" << tempFilePath;
                migrateFilePutReplyFilter(nonce, -1, QString("Can't open temp file %1").arg(tempFilePath), "{ }");
                return;
            }

            // Close source.
            source->close();
            source->deleteLater();

            job.downloadOffset += totalBytes;
            updateJob(job);
        }
    } else {
        // TODO BOX file size will always be different.
        if (QFile(tempFilePath).exists() && QFile(tempFilePath).size() == remoteFileSize) {
            // Found existing temp file.
        } else {
            // Get whole data to file with synchronous method. By removing existing file before start getting.
            QFile(tempFilePath).remove();
            job.downloadOffset = 0;
            updateJob(job);
            QString result = sourceClient->fileGet(nonce, uid, remoteFilePath, tempFilePath, true);
            QScriptEngine engine;
            QScriptValue jsonObj = engine.evaluate("(" + result + ")");
            if (jsonObj.property("error").isValid()) {
                migrateFilePutReplyFilter(nonce, jsonObj.property("error").toInteger(), jsonObj.property("error_string").toString(), "");
                return;
            }
        }

        job.downloadOffset = QFileInfo(tempFilePath).size();
        updateJob(job);
    }
    qDebug() << "CloudDriveModel::migrateFilePut_Block source" << nonce << "is downloaded to temp file. tempFilePath" << tempFilePath << "size" << job.downloadOffset << "remoteFileSize" << remoteFileSize;

    // Check isAborted.
    if (m_isAborted) {
        migrateFilePutReplyFilter(nonce, -1, "job is aborted.","");
        return;
    }

    // Put from file.
    QFile *localSourceFile = new QFile(tempFilePath);
    if (localSourceFile->open(QFile::ReadOnly)) {
        QNetworkReply *targetReply = targetClient->filePut(nonce, targetUid, localSourceFile, localSourceFile->size(), targetRemoteParentPath, targetRemoteFileName, true);
        if (targetReply == 0) {
            // FtpClient doens't provide QNetworkReply. It emits migrateFilePutReplySignal internally.
            job.uploadOffset = localSourceFile->size();
            // NOTE #FP20130408 it left a job after filePut is done.
            // It doesn't need to update as job is being removed by migrateFilePutReplySignal slot.
//            updateJob(job);
        } else {
            if (targetReply->error() == QNetworkReply::NoError){
                job.uploadOffset = localSourceFile->size();
                // Invoke slot to reset running and emit signal.
                migrateFilePutReplyFilter(nonce, targetReply->error(), targetReply->errorString(), targetReply->readAll());
            } else {
                // Invoke slot to reset running and emit signal.
                migrateFilePutReplyFilter(nonce, targetReply->error(), getCloudName(job.targetType) + " " + targetReply->errorString(), getCloudName(job.targetType).append(" ").append(targetReply->readAll()) );
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
        if (QFile(tempFilePath).remove()) {
            qDebug() << "CloudDriveModel::migrateFilePut_Block job" << nonce << "done." << tempFilePath << "is removed.";
        }
    }
}

void CloudDriveModel::migrateFilePutResume_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName)
{
    qDebug() << "----- CloudDriveModel::migrateFilePutResume_Block -----" << nonce << type << uid << remoteFilePath << remoteFileSize << targetType << targetUid << targetRemoteParentPath << targetRemoteFileName;

    // Migrates using resumable methods.
    // TODO Source and target chunk size may not be the same.
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    CloudDriveClient *sourceClient = getCloudClient(type);
    CloudDriveClient *targetClient = getCloudClient(targetType);
    QScriptEngine engine;
    QScriptValue sc;
    qDebug() << "CloudDriveModel::migrateFilePutResume_Block job" << job.toJsonText();

    // Start uploading session and get upload_id.
    if (job.uploadId == "") {
        QString startResult = targetClient->filePutResumeStart(nonce, targetUid, targetRemoteFileName, remoteFileSize, targetRemoteParentPath, true);
        if (startResult != "") {
            qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "startResult" << startResult;
            sc = engine.evaluate("(" + startResult + ")");

            if (sc.property("upload_id").isValid()) {
                job.uploadId = sc.property("upload_id").toString();
                m_cloudDriveJobs->insert(job.jobId, job);
            }
            if (sc.property("error").isValid()) {
                int err = sc.property("error").toInt32();
                QString errString = sc.property("error_string").toString();
                migrateFilePutReplyFilter(job.jobId, err, errString, "", true);
                return;
            }
        }
    } else if (job.uploadOffset < 0) {
        // Check uploading status.
        QString statusResult = targetClient->filePutResumeStatus(nonce, targetUid, targetRemoteFileName, remoteFileSize, job.uploadId, job.uploadOffset, true);
        if (statusResult != "") {
            // TODO Get range and check if resume upload is required.
            qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "statusResult" << statusResult;
            sc = engine.evaluate("(" + statusResult + ")");

            if (sc.property("offset").isValid()) {
                job.uploadOffset = sc.property("offset").toUInt32();
                m_cloudDriveJobs->insert(job.jobId, job);
            }
            if (sc.property("error").isValid()) {
                int err = sc.property("error").toInt32();
                QString errString = sc.property("error_string").toString();
                migrateFilePutReplyFilter(job.jobId, err, errString, "", true);
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
        migrateFilePutReplyFilter(nonce, -1, "job is aborted.","");
        return;
    }

    // Download chunk.
    bool noMoreSourceData = false;
    QIODevice *source = sourceClient->fileGet(nonce, uid, remoteFilePath, job.downloadOffset, true);
    if (source == 0) {
        // Error occurs in fileGet method.
        migrateFilePutReplyFilter(nonce, -1, tr("Service is not implemented or host is not accessible."), "{ }");
        return;
    } else {
        qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "chunk is downloaded. source->metaObject()->className()" << source->metaObject()->className() << "size" << source->size() << "bytesAvailable" << source->bytesAvailable() << "job" << job.toJsonText();
        // TODO Handle source error.
        if (QNetworkReply *sourceReply = dynamic_cast<QNetworkReply*>(source)) {
            if (sourceReply->error() == QNetworkReply::UnknownContentError
                    && sourceReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 416) {
                // NOTE HTTP status code 416 Requested Range Not Satisfiable
                noMoreSourceData = true;
                qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "Requested Range Not Satisfiable. No more source data.";
            } else if (sourceReply->error() != QNetworkReply::NoError) {
                migrateFilePutReplyFilter(job.jobId, sourceReply->error(), sourceReply->errorString(), QString::fromUtf8(sourceReply->readAll()));
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
        migrateFilePutReplyFilter(nonce, -1, "job is aborted.","");
        return;
    }

    QString uploadResult;
    if (source->bytesAvailable() > 0) {
        // Upload chunk and get offset (and upload_id if exists).
        uploadResult = targetClient->filePutResumeUpload(nonce, targetUid, source, targetRemoteFileName, remoteFileSize, job.uploadId, job.uploadOffset, true);
        // Default target remote path for client with remote absolute path. (Ex. Dropbox, FTP)
        job.targetRemoteFilePath = (targetClient->isRemoteAbsolutePath()) ? (targetRemoteParentPath + "/" + targetRemoteFileName) : "";
        if (uploadResult != "") {
            // Get range and check if resume upload is required.
            qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "uploadResult" << uploadResult;
            sc = engine.evaluate("(" + uploadResult + ")");

            if (sc.property("upload_id").isValid()) {
                job.uploadId = sc.property("upload_id").toString();
            }
            if (sc.property("offset").isValid()) {
                job.uploadOffset = sc.property("offset").toUInt32();
            }
            if (sc.property("absolutePath").isValid()) { // Find targetRemoteFilePath from upload result. (GoogleDrive, Dropbox?)
                job.targetRemoteFilePath = sc.property("absolutePath").toString();
                job.uploadOffset = sc.property("size").toUInt32(); // Get file size to uploadOffset.
                qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "uploaded file with targetRemoteFilePath" << job.targetRemoteFilePath;
            }
            // Update changed job.
            m_cloudDriveJobs->insert(job.jobId, job);

            if (sc.property("error").isValid()) {
                int err = sc.property("error").toInt32();
                QString errString = sc.property("error_string").toString();
                job.uploadOffset = -1; // Failed upload needs to get status on resume.
                m_cloudDriveJobs->insert(job.jobId, job); // Update changed job.
                migrateFilePutReplyFilter(job.jobId, err, errString, "", true);
                return;
            }
        } else {
            migrateFilePutReplyFilter(job.jobId, -1, "Unexpected error.", "Unexpected upload result is empty.", true);
            return;
        }
    } // if source->bytesAvailable() > 0

    // Check whether resume or commit.
    if (job.uploadOffset < job.bytesTotal && !noMoreSourceData) {
        // Enqueue and resume job.
        qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "resume uploading job" << job.toJsonText();
        m_jobQueue->insert(0, job.jobId);
        updateJob(job);
        jobDone();
        return;
    }

    // Invoke to handle successful uploading.
    qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "commit uploading job" << job.toJsonText();
    QString commitResult = targetClient->filePutCommit(nonce, targetUid, job.targetRemoteFilePath, job.uploadId, true);
    if (commitResult != "") {
        qDebug() << "CloudDriveModel::migrateFilePutResume_Block" << nonce << "commitResult" << commitResult;
        sc = engine.evaluate("(" + commitResult + ")");

        if (sc.property("error").isValid()) {
            int err = sc.property("error").toInt32();
            QString errString = sc.property("error_string").toString();
            migrateFilePutReplyFilter(job.jobId, err, errString, "", true);
            return;
        }
        // Proceed filter with commit result and job done.
        migrateFilePutReplyFilter(job.jobId, QNetworkReply::NoError, "", commitResult, true);
    } else {
        // Proceed filter with last upload result and job done.
        migrateFilePutReplyFilter(job.jobId, QNetworkReply::NoError, "", uploadResult, true);
    }
}

void CloudDriveModel::disconnect(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Disconnect, type, uid, localPath, remotePath, -1);
    addJob(job);
}

void CloudDriveModel::deleteLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), DeleteLocal, type, uid, localPath, "", -1);
    addJob(job);
}

QString CloudDriveModel::thumbnail(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString format, QString size)
{
    CloudDriveClient *client = getCloudClient(getClientType(type));
    if (!client->isAuthorized()) {
        return "";
    }

    return client->thumbnail(createNonce(), uid, remoteFilePath, format, size);
}

QString CloudDriveModel::media(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath)
{
    CloudDriveClient *client = getCloudClient(type);
    if (!client->isAuthorized()) {
        return "";
    }

    return client->media(createNonce(), uid, remoteFilePath);
}

QString CloudDriveModel::shareFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, bool synchronous)
{
    if (!isSharable(type, uid)) {
        CloudDriveJob job(createNonce(), ShareFile, type, uid, localFilePath, remoteFilePath, -1);
        m_cloudDriveJobs->insert(job.jobId, job);
        emit shareFileReplySignal(job.jobId, -1, getCloudClient(type)->objectName() + " " + "Share link", tr("Service is not available."), "", 0);
        return "";
    }

    if (remoteFilePath == "") {
        CloudDriveJob job(createNonce(), ShareFile, type, uid, localFilePath, remoteFilePath, -1);
        m_cloudDriveJobs->insert(job.jobId, job);
        emit shareFileReplySignal(job.jobId, -1, getCloudClient(type)->objectName() + " " + "Share link", tr("Cloud file ID is not available."), "", 0);
        return "";
    }

    // Check if it's synchronous request.
    if (synchronous) {
        CloudDriveClient *client = getCloudClient(type);
        if (!client->isAuthorized()) {
            return "";
        }

        return client->shareFile(createNonce(), uid, remoteFilePath, synchronous);
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), ShareFile, type, uid, localFilePath, remoteFilePath, -1);
    addJob(job);

    return "";
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
    addJob(job);
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
    addJob(job);
}

void CloudDriveModel::deleteFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, bool suppressDeleteLocal)
{
    if (remoteFilePath == "") {
        qDebug() << "CloudDriveModel::deleteFile remoteFilePath" << remoteFilePath << "is empty. Operation is aborted.";
        return;
    }

    // Enqueue job.
    CloudDriveJob job(createNonce(), DeleteFile, type, uid, localFilePath, remoteFilePath, -1);
    job.suppressDeleteLocal = suppressDeleteLocal;
    addJob(job);
}

void CloudDriveModel::delta(CloudDriveModel::ClientTypes type, QString uid)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), Delta, type, uid, "", "", -1);
    addJob(job);
}

void CloudDriveModel::filePutResume(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), FilePutResume, type, uid, localFilePath, remoteParentPath, -1);
    job.newRemoteFileName = remoteFileName;
    job.uploadId = uploadId;
    job.uploadOffset = offset;
    job.bytesTotal = QFileInfo(localFilePath).size();
    addJob(job);
}

void CloudDriveModel::fileGetResume(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFIleSize, QString localFilePath, qint64 offset)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), FileGetResume, type, uid, localFilePath, remoteFilePath, -1);
    job.downloadOffset = offset;
    job.bytesTotal = remoteFIleSize;
    addJob(job);
}

void CloudDriveModel::migrateFile(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName)
{
    // Enqueue job.
    CloudDriveJob job(createNonce(), MigrateFile, type, uid, "", remoteFilePath, -1);
    job.targetType = targetType;
    job.targetUid = targetUid;
    job.newRemoteFilePath = targetRemoteParentPath;
    job.newRemoteFileName = targetRemoteFileName;
    addJob(job);
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
    addJob(job);
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
            addItem(getClientType(job.type), job.uid, job.localFilePath, job.remoteFilePath, hash,
                    getItem(getParentLocalPath(job.localFilePath), getClientType(job.type), job.uid).syncDirection);

            // Emit signal to refresh its parent folder cache.
            emit refreshFolderCacheSignal(job.localFilePath);
        }
    } else if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    } else {
        if (job.localFilePath != "") {
            removeItem(getClientType(job.type), job.uid, job.localFilePath);

            // Emit signal to refresh its parent folder cache.
            emit refreshFolderCacheSignal(job.localFilePath);
        }
    }

    // Update job running flag.
    job.err = err;
    job.errString = errMsg;
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit fileGetReplySignal(nonce, err, errMsg, msg);

    // Remove finished job.
    if (err == 0) {
        removeJob("CloudDriveModel::fileGetReplyFilter", nonce);
    }
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
            addItem(getClientType(job.type), job.uid, job.localFilePath, remoteFilePath, hash,
                    getItem(getParentLocalPath(job.localFilePath), getClientType(job.type), job.uid).syncDirection);
        }
    } else if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
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

    // Remove finished job.
    if (err == 0) {
        removeJob("CloudDriveModel::filePutReplyFilter", nonce);
    }
}

void CloudDriveModel::metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    bool suppressRemoveJob = false;

    // Parse and process metadata.
    if (err == QNetworkReply::NoError) {
        qDebug() << "CloudDriveModel::metadataReplyFilter" << getCloudName(job.type) << nonce << err << errMsg
                 << (m_settings.value("Logging.enabled", false).toBool() ? msg : "");

        // Found metadata.
        // Parse to common json object.
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + msg + ")");
        int totalCount = jsonObj.property("totalCount").isValid() ? jsonObj.property("totalCount").toInteger() : 0;
        int offset = jsonObj.property("offset").isValid() ? jsonObj.property("offset").toInteger() : 0;
        int limit = jsonObj.property("limit").isValid() ? jsonObj.property("limit").toInteger() : 0;
        int nextOffset = offset + limit;
        suppressRemoveJob = (nextOffset < totalCount);
        CloudDriveItem parentItem = getItem(job.localFilePath, getClientType(job.type), job.uid);
        if (!parentItem.isValid()) {
            parentItem.syncDirection = getItem(getParentLocalPath(job.localFilePath), getClientType(job.type), job.uid).syncDirection;
        }

        // Suspend next job.
        suspendNextJob();

        if (jsonObj.property("isDeleted").toBool()) {
            // Item was removed remotely.
            qDebug() << "CloudDriveModel::metadataReplyFilter" << getCloudName(job.type) << job.uid << jsonObj.property("absolutePath").toString()
                     << "isDeleted" << jsonObj.property("isDeleted").toString() << "syncDirection" << parentItem.syncDirection;

            // Delete local item if direction is not SyncForward.
            if (parentItem.syncDirection != SyncForward
                    && m_settings.value("CloudDriveModel.metadataRelyFilter.deleteLocalFile.enabled", true).toBool()) {
                qDebug() << "CloudDriveModel::metadataReplyFilter" << nonce << "trash item" << job.type << job.uid << job.localFilePath;
                deleteLocal(getClientType(job.type), job.uid, job.localFilePath);
            } else {
                qDebug() << "CloudDriveModel::metadataReplyFilter" << nonce << "disconnect item" << job.type << job.uid << job.localFilePath;
                disconnect(getClientType(job.type), job.uid, job.localFilePath);
            }

            // Notify removed link.
            emit logRequestSignal(nonce,
                                  "warn",
                                  getCloudName(job.type) + " " + tr("Metadata"),
                                  tr("%1 was removed remotely.\nLink will be removed.").arg(job.localFilePath),
                                  2000);
        } else {
            // Sync starts from itself.
            if (jsonObj.property("isDir").toBool()) { // Sync folder.
                qDebug() << "CloudDriveModel::metadataReplyFilter dir" << getCloudName(job.type) << nonce
                         << "sync folder remote path" << jsonObj.property("absolutePath").toString() << "hash" << jsonObj.property("hash").toString()
                         << "local path" << job.localFilePath << " hash " << parentItem.hash << "children length" << jsonObj.property("children").property("length").toInteger()
                         << "forcePut" << job.forcePut << "forceGet" << job.forceGet << "syncDirection" << parentItem.syncDirection;

                // If there is no local folder, create it and connect.
                if (!isDir(job.localFilePath)) {
                    // TODO Add item to ListView.
                    createDirPath(job.localFilePath);
                    // Remove cache on target folders and its parents.
                    emit refreshItemAfterFileGetSignal(nonce, job.localFilePath);
                }

                // Sync based on remote contents.
                QStringList remotePathList; // NOTE job.remotePathList is default as * means keep all items.
                int r = compareMetadata(job, jsonObj, job.localFilePath);
                if (r != 0) { // Sync all json(remote)'s contents.
                    for (int i=0; i<jsonObj.property("children").property("length").toInteger(); i++) {
                        QScriptValue item = jsonObj.property("children").property(i);
                        QString itemLocalPath = getAbsolutePath(job.localFilePath, item.property("name").toString());
                        CloudDriveItem childItem = getItem(itemLocalPath, getClientType(job.type), job.uid);
                        if (!childItem.isValid()) {
                            childItem.syncDirection = parentItem.syncDirection;
                        }
                        remotePathList.append(item.property("absolutePath").toString());
                        if (item.property("isDir").toBool()) {
                            // This flow will trigger recursive metadata calling.
                            // NOTE Dropbox's metadata return rev for child dir(s) which is incomparable with hash. So it needs to get actual metadata for hash.
                            int r = compareMetadata(job, item, itemLocalPath);
                            qDebug() << "CloudDriveModel::metadataReplyFilter dir" << getCloudName(job.type) << nonce
                                     << "sync children dir remote path" << item.property("absolutePath").toString() << "hash" << item.property("hash").toString() << "local path" << itemLocalPath << "hash" << childItem.hash
                                     << "syncDirection" << childItem.syncDirection << "compare result" << r;
                            if (r != 0) {
                                metadata(getClientType(job.type), job.uid, itemLocalPath, item.property("absolutePath").toString(), -1, job.forcePut, job.forceGet);
                            }
                        } else {
                            // Check if it's cloud format.
                            if (item.property("isCloudOnly").isValid() && item.property("isCloudOnly").toBool()) {
                                qDebug() << "CloudDriveModel::metadataReplyFilter dir" << getCloudName(job.type) << nonce << "suppress sync children file remote path" << item.property("absolutePath").toString() << "mimeType" << item.property("mimeType").toString();
                                emit logRequestSignal(nonce,
                                                      "warn",
                                                      getCloudName(job.type) + " " + tr("Metadata"),
                                                      tr("%1 is cloud document format.\nIt can't be downloaded.").arg(item.property("name").toString()),
                                                      2000);
                            } else {
                                // Check if file is not sync'd.
                                int r = compareMetadata(job, item, itemLocalPath);
                                qDebug() << "CloudDriveModel::metadataReplyFilter dir" << getCloudName(job.type) << nonce
                                         << "sync children file remote path" << item.property("absolutePath").toString() << "hash" << item.property("hash").toString() << "local path" << itemLocalPath << "hash" << childItem.hash
                                         << "syncDirection" << childItem.syncDirection << "compare result" << r;
                                if (r < 0 && childItem.syncDirection != SyncForward) { // remote file is changed and not SyncForward.
                                    fileGet(getClientType(job.type), job.uid, item.property("absolutePath").toString(), item.property("size").toInt32(), itemLocalPath, -1);
                                } else if (r > 0 && childItem.syncDirection != SyncBackward) { // local file is changed and not SyncBackward.
                                    filePut(getClientType(job.type), job.uid, itemLocalPath, item.property("parentPath").toString(), item.property("name").toString(), -1);
                                } else {
                                    // TODO Detect changed name.
                                    // 1) Rename local file.
                                    // 2) Update cloud drive item.
                                    addItem(getClientType(job.type), job.uid, itemLocalPath, item.property("absolutePath").toString(), item.property("hash").toString(), childItem.syncDirection);
                                }
                            }
                        }

                        // Process UI events.
                        QApplication::processEvents();
                    }
                }
                // Append remotePathList to job if it's not empty. Otherwise, keep it with "*".
                // NOTE job.remotePathList is default as * means keep all items.
                if (!remotePathList.empty()) {
                    job.remotePathList.removeAll("*");
                    job.remotePathList.append(remotePathList);
                    updateJob(job, false);
                }

                // Add or Update timestamp from local to cloudDriveItem.
                // TODO Detect changed name.
                // 1) Rename local folder.
                // 2) Update cloud drive item of local folder and its children.  REMARK but it will have problem if local folder connected with multiple cloud folders.
                addItem(getClientType(job.type), job.uid, job.localFilePath, job.remoteFilePath, jsonObj.property("hash").toString(), parentItem.syncDirection);

                // Sync based on local contents.
                // TODO Issue: syncFromLocal can't detect deleted remote file if it still has connection, then syncFromLocal will skip it.
                qDebug() << "CloudDriveModel::metadataReplyFilter dir" << getCloudName(job.type) << nonce << "job.remotePathList.length()" << job.remotePathList.length();
                if (nextOffset >= totalCount) {
                    syncFromLocal(getClientType(job.type), job.uid, job.localFilePath, jsonObj.property("parentPath").toString(), job.modelIndex, job.forcePut, job.remotePathList);
                } else {
                    // Return as job is still running in background.
                    return;
                }
            } else { // Sync file.
                // Check if it's cloud format.
                if (jsonObj.property("isCloudOnly").isValid() && jsonObj.property("isCloudOnly").toBool()) {
                    qDebug() << "CloudDriveModel::metadataReplyFilter file" << getCloudName(job.type) << nonce << "suppress sync file remote path" << jsonObj.property("absolutePath").toString() << "mimeType" << jsonObj.property("mimeType").toString();
                    emit logRequestSignal(nonce,
                                          "warn",
                                          getCloudName(job.type) + " " + tr("Metadata"),
                                          tr("%1 is cloud document format.\nIt can't be downloaded.").arg(jsonObj.property("name").toString()),
                                          2000);
                } else {
                    // Check if file is not sync'd.
                    int r = compareMetadata(job, jsonObj, job.localFilePath);
                    qDebug() << "CloudDriveModel::metadataReplyFilter file" << getCloudName(job.type) << nonce << "sync file remote path" << jsonObj.property("absolutePath").toString() << "hash" << jsonObj.property("hash").toString() << "local path" << job.localFilePath << "hash" << parentItem.hash << "forcePut" << job.forcePut << "forceGet" << job.forceGet << "syncDirection" << parentItem.syncDirection << "compare result" << r;
                    if (r < 0 && parentItem.syncDirection != SyncForward) { // remote file is changed and not SyncForward.
                        fileGet(getClientType(job.type), job.uid, jsonObj.property("absolutePath").toString(), jsonObj.property("size").toInt32(), job.localFilePath, job.modelIndex);
                    } else if (r > 0 && parentItem.syncDirection != SyncBackward) { // local file is changed and not SyncBackward.
                        filePut(getClientType(job.type), job.uid, job.localFilePath, jsonObj.property("parentPath").toString(), jsonObj.property("name").toString(), job.modelIndex);
                    } else {
                        // TODO Detect changed name.
                        // 1) Rename local file.
                        // 2) Update cloud drive item.
                        addItem(getClientType(job.type), job.uid, job.localFilePath, job.remoteFilePath, jsonObj.property("hash").toString(), parentItem.syncDirection);
                    }
                }
            }
        }

        // Resume next jobs.
        resumeNextJob();
    } else if (err == 203) { // If metadata is not found, put it to cloud right away recursively.
        // Suspend next job.
        suspendNextJob();

        QString localParentPath = getParentLocalPath(job.localFilePath);
        QString remoteParentPath = "";
        QString remoteFileName = getFileName(job.localFilePath);

        // NOTE
        // Differentiate from newly sync and remotely removed item.
        // By checking if there is a cloud item for localPath.
        CloudDriveItem cloudItem = getItem(job.localFilePath, getClientType(job.type), job.uid);
        if (job.localFilePath == cloudItem.localPath && job.type == cloudItem.type && job.uid == cloudItem.uid) {
            // Existing connected item.
        } else {
            // Not existing item.
            if (isRemoteAbsolutePath(getClientType(job.type))) {
                remoteParentPath = getParentRemotePath(getClientType(job.type), job.remoteFilePath);
            } else {
                remoteParentPath = getItemRemotePath(localParentPath, getClientType(job.type), job.uid);
            }
        }
        qDebug() << "CloudDriveModel::metadataReplyFilter" << err << errMsg << msg << "localParentPath" << localParentPath << "remoteParentPath" << remoteParentPath;

        // Proceed put if remoteParentPath is available. Otherwise remove the link.
        if (remoteParentPath != "") {
            if (cloudItem.syncDirection != SyncBackward) { // syncFromLocal if direction is not SyncBackward.
                qDebug() << "CloudDriveModel::metadataReplyFilter" << getCloudName(job.type) << "put" << job.localFilePath << "to" << job.remoteFilePath;
                if (isDir(job.localFilePath)) {
                    // Remote folder will be created in syncFromLocal if it's required.
                    syncFromLocal(getClientType(job.type), job.uid, job.localFilePath, remoteParentPath, job.modelIndex);
                } else {
                    filePut(getClientType(job.type), job.uid, job.localFilePath, remoteParentPath, remoteFileName, job.modelIndex);
                }
            }
        } else {
            // Delete local item if direction is not SyncForward.
            if (cloudItem.syncDirection != SyncForward
                    && m_settings.value("CloudDriveModel.metadataRelyFilter.deleteLocalFile.enabled", true).toBool()) {
                qDebug() << "CloudDriveModel::metadataReplyFilter" << nonce << "trash item" << job.type << job.uid << job.localFilePath;
                deleteLocal(getClientType(job.type), job.uid, job.localFilePath);
            } else {
                qDebug() << "CloudDriveModel::metadataReplyFilter" << nonce << "disconnect item" << job.type << job.uid << job.localFilePath;
                disconnect(getClientType(job.type), job.uid, job.localFilePath);
            }

            // Notify removed link.
            emit logRequestSignal(nonce,
                                  "warn",
                                  getCloudName(job.type) + " " + tr("Metadata"),
                                  tr("%1 was removed remotely.\nLink will be removed.").arg(job.localFilePath),
                                  2000);
        }

        // Resume next jobs.
        resumeNextJob();
    } else if (err == 204) { // Refresh token
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job running flag.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Update job running flag.
    job.isRunning = false;
    updateJob(job);
    jobDone();

    emit metadataReplySignal(nonce, err, errMsg, msg, suppressRemoveJob);

    // Remove only success job.
    if ((err == 0 || err == 203) && !suppressRemoveJob) {
        removeJob("CloudDriveModel::metadataReplyFilter", nonce);
    }
}

void CloudDriveModel::browseReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
#ifdef Q_OS_SYMBIAN
    // Remove all threads from pool once there is no active thread.
    // NOTE To prevent thread panicking on Symbian. (#FP20130153)
    if (m_browseThreadPool.activeThreadCount() <= 0) {
        m_browseThreadPool.waitForDone();
        qDebug() << "CloudDriveModel::browseReplyFilter waitForDone() is invoked. m_browseThreadPool" << m_browseThreadPool.activeThreadCount() << "/" << m_browseThreadPool.maxThreadCount();
    }
#endif

    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    bool suppressRemoveJob = false;

    // Route signal to caller.
    if (job.operation == MigrateFile) {
        migrateFileReplyFilter(nonce, err, errMsg, msg);
        return;
    }

    // Populate internal list model to reduce parse processing on QML side.
    // TODO Figure out exchanging value objects between specific cloud client and CDM.
    // TODO Use QMultiMap to sort while inserting?

    if (err == QNetworkReply::NoError) {
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "CloudDriveModel::browseReplyFilter" << nonce << msg.size()
                 << (m_settings.value("Logging.enabled", false).toBool() ? msg : "");

        QScriptEngine engine;
        QScriptValue json = engine.evaluate("(" + msg + ")");
        int totalCount = json.property("totalCount").isValid() ? json.property("totalCount").toInteger() : 0;
        int offset = json.property("offset").isValid() ? json.property("offset").toInteger() : 0;
        int limit = json.property("limit").isValid() ? json.property("limit").toInteger() : 0;
        int nextOffset = offset + limit;
        suppressRemoveJob = (nextOffset < totalCount);
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "CloudDriveModel::browseReplyFilter" << nonce << "msg is parsed to JSON object.";

        // Clear model item list if offset == 0 (first chunk).
        if (json.property("offset").toInteger() == 0) {
            m_modelItemList->clear();
        }

        // Populate model item list.
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "CloudDriveModel::browseReplyFilter" << nonce << "children.length" << json.property("children").property("length").toInteger();

        m_remoteParentPath = json.property("absolutePath").toString();
        m_remoteParentPathName = isRemoteAbsolutePath(getClientType(job.type)) ? m_remoteParentPath : json.property("name").toString();
        m_remoteParentParentPath = json.property("parentPath").toString();

        for (int i = 0; i < json.property("children").property("length").toInt32(); i++) {
            QScriptValue childItem = json.property("children").property(i);

            CloudDriveModelItem modelItem = getCloudClient(job.type)->parseCloudDriveModelItem(engine, childItem);
            modelItem.isConnected = isRemotePathConnected(getClientType(job.type), job.uid, modelItem.absolutePath);

            m_modelItemList->append(modelItem);

            // Process UI events.
            QApplication::processEvents();
        }
        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "CloudDriveModel::browseReplyFilter" << nonce << "model is populated. m_modelItemList->size()" << m_modelItemList->size();

        // Sort model item list.
        sortItemList(m_modelItemList, getSortFlag(getClientType(job.type), job.uid, m_remoteParentPath));

        emit remoteParentPathChanged();
        emit rowCountChanged();

        // Emit data changed.
        beginResetModel();
        emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
        endResetModel();

        // Update job running flag.
        job.isRunning = false;
        updateJob(job);
        jobDone();

        emit browseReplySignal(nonce, err, errMsg, "{}", suppressRemoveJob);
    } else if (err == QNetworkReply::AuthenticationRequiredError) { // Refresh token
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    } else {
        // Update job running flag.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        emit browseReplySignal(nonce, err, errMsg, msg, false);
    }

    // Remove finished job.
    if (!suppressRemoveJob) {
        removeJob("CloudDriveModel::browseReplyFilter", nonce);
    }
}

void CloudDriveModel::migrateFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
    bool suppressRemoveJob = false;

    if (err == QNetworkReply::NoError) {
        qDebug() << "CloudDriveModel::migrateFileReplyFilter" << getCloudName(job.type) << nonce << err << errMsg
                 << (m_settings.value("Logging.enabled", false).toBool() ? msg : "");

        // Parse to common json object.
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + msg + ")");
        int totalCount = jsonObj.property("totalCount").isValid() ? jsonObj.property("totalCount").toInteger() : 0;
        int offset = jsonObj.property("offset").isValid() ? jsonObj.property("offset").toInteger() : 0;
        int limit = jsonObj.property("limit").isValid() ? jsonObj.property("limit").toInteger() : 0;
        int nextOffset = offset + limit;
        suppressRemoveJob = (nextOffset < totalCount);

        // Suspend next job.
        suspendNextJob();

        if (jsonObj.property("isDeleted").toBool()) {
            // Do nothing.
        } else {
            // Migration starts from itself.
            if (jsonObj.property("isDir").toBool()) { // Migrate folder.
                // Create target folder.
                QString createdRemoteFolderPath = createFolder_Block(getClientType(job.targetType), job.targetUid, job.newRemoteFilePath, job.newRemoteFileName);
                if (createdRemoteFolderPath != "") {
                    for (int i=0; i<jsonObj.property("children").property("length").toInteger(); i++) {
                        QScriptValue item = jsonObj.property("children").property(i);
                        if (item.property("isDir").toBool()) {
                            // This flow will trigger recursive migrateFile calling.
                            migrateFile(getClientType(job.type), job.uid, item.property("absolutePath").toString(), getClientType(job.targetType), job.targetUid, createdRemoteFolderPath, item.property("name").toString());
                        } else {
                            // Migrate file.
                            migrateFilePut(getClientType(job.type), job.uid, item.property("absolutePath").toString(), item.property("size").toInteger(), getClientType(job.targetType), job.targetUid, createdRemoteFolderPath, item.property("name").toString());
                        }

                        // Process UI events.
                        QApplication::processEvents();
                    }
                } else {
                    // Update job running flag.
                    job.isRunning = false;
                    updateJob(job);
                    jobDone();
                    emit migrateFileReplySignal(nonce, QNetworkReply::UnknownContentError, tr("Error"), tr("Can't create folder. Migration is aborted."), false);
                    // Resume next jobs.
                    resumeNextJob();
                    return;
                }
            } else { // Migrate file.
                migrateFilePut(getClientType(job.type), job.uid, job.remoteFilePath, jsonObj.property("size").toInteger(), getClientType(job.targetType), job.targetUid, job.newRemoteFilePath, job.newRemoteFileName);
            }
        }

        // Resume queued jobs once all items have been queued.
        if (nextOffset >= totalCount) {
            // Update job running flag.
            job.isRunning = false;
            updateJob(job);
            jobDone();
            emit migrateFileReplySignal(nonce, err, errMsg, "{}", suppressRemoveJob);
            // Resume next jobs.
            resumeNextJob();
        }
    } else if (err == QNetworkReply::AuthenticationRequiredError) { // Refresh token
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job running flag.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    } else {
        // Update job running flag.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        emit migrateFileReplySignal(nonce, err, errMsg, msg, false);
    }

    // Remove finished job.
    if (!suppressRemoveJob) {
        removeJob("CloudDriveModel::migrateFileReplyFilter", nonce);
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

    if (err == QNetworkReply::NoError) {
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
    } else if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit createFolderReplySignal(nonce, err, errMsg, msg);

    removeJob("CloudDriveModel::createFolderReplyFilter", nonce);
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
    } else if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit moveFileReplySignal(nonce, err, errMsg, msg);

    removeJob("CloudDriveModel::moveFileReplyFilter", nonce);
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
    } else if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit copyFileReplySignal(nonce, err, errMsg, msg);

    removeJob("CloudDriveModel::copyFileReplyFilter", nonce);
}

void CloudDriveModel::deleteFileReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Remove deleted item from cloud folder page.
    if (job.remoteFilePath != "") {
        int i = findIndexByRemotePath(job.remoteFilePath);
        if (i > -1) {
            removeRow(i);
        }
    }

    // Disconnect deleted local path.
    if (job.remoteFilePath != "") {        
        foreach (CloudDriveItem item, findItemsByRemotePath(getClientType(job.type), job.uid, job.remoteFilePath)) {
            removeItemWithChildren(getClientType(item.type), item.uid, item.localPath);
            // Request trash local path if it's not connected and not suppress deletion.
            if (item.syncDirection != SyncForward
                    && !job.suppressDeleteLocal
                    && m_settings.value("CloudDriveModel.deleteFileReplyFilter.deleteLocalFile.enabled", true).toBool()
                    && !isConnected(item.localPath)) {
                qDebug() << "CloudDriveModel::deleteFileReplyFilter" << nonce << "trash local path" << item.localPath;
                requestMoveToTrash(nonce, item.localPath);
            }
        }
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit deleteFileReplySignal(nonce, err, errMsg, msg);

    removeJob("CloudDriveModel::deleteFileReplyFilter", nonce);
}

void CloudDriveModel::shareFileReplyFilter(QString nonce, int err, QString errMsg, QString msg, QString url, int expires)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Stop running.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit shareFileReplySignal(nonce, err, errMsg, msg, url, expires);

    removeJob("CloudDriveModel::shareFileReplyFilter", nonce);
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
            for (int i = 0; i < parsedObj.property("children").property("length").toInteger(); i++) {
                // Process pending events.
                QApplication::processEvents();

                QScriptValue childObj = parsedObj.property("children").property(i);
                bool isDeleted = childObj.property("isDeleted").toBool();
                QString remoteFilePath = childObj.property("absolutePath").toString();
                QString remoteParentPath = childObj.property("parentPath").toString();
                //        qDebug() << "CloudDriveModel::deltaReplyFilter remoteFilePath" << remoteFilePath << "remoteParentPath" << remoteParentPath << "isDeleted" << isDeleted;

                if (isDeleted) {
                    if (remotePathHash.value(remoteFilePath)) {
                        qDebug() << "CloudDriveModel::deltaReplyFilter" << nonce << "suppress remove remoteFilePath" << remoteFilePath;
                    } else {
                        foreach (CloudDriveItem item, findItemsByRemotePath(getClientType(job.type), job.uid, remoteFilePath, isRemotePathCaseInsensitive(getClientType(job.type)))) {
                            // Configurable file removing.
                            if (m_settings.value("CloudDriveModel.deltaReplyFilter.deleteLocalFile.enabled", true).toBool()) {
                                qDebug() << "CloudDriveModel::deltaReplyFilter" << nonce << "trash item" << item.type << item.uid << item.localPath << "remotePath" << item.remotePath << "hash" << item.hash;
                                deleteLocal(getClientType(item.type), item.uid, item.localPath);
                            } else {
                                qDebug() << "CloudDriveModel::deltaReplyFilter" << nonce << "disconnect item" << item.type << item.uid << item.localPath << "remotePath" << item.remotePath << "hash" << item.hash;
                                disconnect(getClientType(item.type), item.uid, item.localPath);
                            }
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
    } else if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Update job running flag.
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit deltaReplySignal(nonce, err, errMsg, msg);

    removeJob("CloudDriveModel::deltaReplyFilter", nonce);
}

void CloudDriveModel::migrateFilePutReplyFilter(QString nonce, int err, QString errMsg, QString msg, bool errorOnTarget)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    if (err == QNetworkReply::AuthenticationRequiredError) {
        // Check if token is expired (err 204), then refresh token.
        if (errorOnTarget) {
            refreshToken(getClientType(job.targetType), job.targetUid, job.jobId);
        } else {
            refreshToken(getClientType(job.type), job.uid, job.jobId);
        }
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    // Update job running flag.
    job.err = err;
    job.errString = errMsg;
//    job.errMessage = msg.replace(QRegExp("\""), "\\\""); // TODO encode to ?
    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit migrateFilePutReplySignal(nonce, err, errMsg, msg, errorOnTarget);

    // Remove finished job.
    if (err == 0) {
        removeJob("CloudDriveModel::migrateFilePutReplyFilter", nonce);
    }
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
    } else if (err == QNetworkReply::UnknownContentError) {
        QScriptEngine engine;
        QScriptValue jsonObj = engine.evaluate("(" + msg + ")");
        if (jsonObj.property("http_status_code").isValid() && jsonObj.property("http_status_code").toInteger() == 416) {
            // NOTE HTTP status code 416 Requested Range Not Satisfiable
            qDebug() << "CloudDriveModel::fileGetResumeReplyFilter commit downloading job" << job.toJsonText();
            fileGetReplyFilter(job.jobId, 0, errMsg, msg);
            return;
        } else {
            qDebug() << "CloudDriveModel::fileGetResumeReplyFilter failed. Operation is aborted. jobId" << nonce << "error" << err << errMsg << msg;
            fileGetReplyFilter(job.jobId, err, errMsg, msg);
            return;
        }
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
    } else if (err != 0 && job.type == GoogleDrive) {
        // For GoogleDrive only.
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

    removeJob("CloudDriveModel::refreshRequestFilter", nonce);
}

void CloudDriveModel::schedulerTimeoutFilter()
{
    if (m_isSchedulerSuspended) {
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter suspended m_isSchedulerSuspended" << m_isSchedulerSuspended << QDateTime::currentDateTime().toString("d/M/yyyy hh:mm:ss.zzz");
        return;
    }

    if (runningJobCount > 0) {
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter suspended runningJobCount" << runningJobCount << QDateTime::currentDateTime().toString("d/M/yyyy hh:mm:ss.zzz");
        return;
    }

    if (m_settings.value("Logging.enabled", false).toBool()) {
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_deviceInfo.batteryStatus()" << m_deviceInfo.batteryStatus();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_deviceInfo.batteryLevel()" << m_deviceInfo.batteryLevel();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_deviceInfo.currentPowerState()" << m_deviceInfo.currentPowerState();
#if defined(Q_WS_HARMATTAN)
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.getChargerType()" << m_batteryInfo.getChargerType();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.getChargingState()" << m_batteryInfo.getChargingState();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.getBatteryState()" << m_batteryInfo.getBatteryState();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.getRemainingCapacityPct()" << m_batteryInfo.getRemainingCapacityPct();
#else
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.chargerType()" << m_batteryInfo.chargerType();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.chargingState()" << m_batteryInfo.chargingState();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.batteryStatus()" << m_batteryInfo.batteryStatus();
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_batteryInfo.remainingCapacityPercent()" << m_batteryInfo.remainingCapacityPercent();
#endif
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_networkInfo.currentMode()" << m_networkInfo.currentMode() << "m_networkInfo.networkStatus(currentMode)" << m_networkInfo.networkStatus(m_networkInfo.currentMode());
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode)" << m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode);
    }

    if (m_settings.value("CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnCharging.enabled", false).toBool()
            && !isDeviceBatteryCharging()) {
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter suspended isDeviceBatteryCharging()" << isDeviceBatteryCharging();
        return;
    }

    if (m_settings.value("CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnBatteryOk.enabled", false).toBool()
            && m_deviceInfo.batteryStatus() < QSystemDeviceInfo::BatteryNormal) {
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter suspended m_deviceInfo.batteryStatus()" << m_deviceInfo.batteryStatus() << "m_deviceInfo.batteryLevel()" << m_deviceInfo.batteryLevel();
        return;
    }

    if (m_settings.value("CloudDriveModel.schedulerTimeoutFilter.syncOnlyOnWlan.enabled", false).toBool()
            && m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode) != QSystemNetworkInfo::Connected) {
        qDebug() << "CloudDriveModel::schedulerTimeoutFilter suspended m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode)" << m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode);
        return;
    }

//    qDebug() << "CloudDriveModel::schedulerTimeoutFilter" << QDateTime::currentDateTime().toString("d/M/yyyy hh:mm:ss.zzz");

    // Set dirty to scheduled items.
    QString cronValue = QDateTime::currentDateTime().toString("m h d M ddd");
    dirtyScheduledItems(cronValue);

    // Sync dirty items.
    syncDirtyItems();

    // Request delta if there is no running job.
    scheduleDeltaJobs(cronValue);

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
    QSqlQuery query(DatabaseManager::defaultManager().getDB());
    bool res;

    // Test database table.
    // NOTE QSqlError 10, 11, 19 requires reindex.
    res = query.exec("SELECT count(*) FROM cloud_drive_item WHERE local_path = ''");
    if (res) {
        qDebug() << "CloudDriveModel::fixDamagedDB cloud_drive_item is valid. Operation is aborted.";
        return;
    } else {
        qDebug() << "CloudDriveModel::fixDamagedDB cloud_drive_item is malformed. Error" << query.lastError();
    }

    // Drop index to fix QSqlError 11 database disk image is malformed.
    res = query.exec("DROP INDEX cloud_drive_item_pk;");
    if (res) {
        qDebug() << "CloudDriveModel::fixDamagedDB DROP INDEX is done.";
    } else {
        qDebug() << "CloudDriveModel::fixDamagedDB DROP INDEX is failed. Error" << query.lastError();
    }

    res = query.exec("DELETE FROM cloud_drive_item WHERE local_path = ''");
    if (res) {
        qDebug() << "CloudDriveModel::fixDamagedDB DELETE item with empty local_path is done." << query.numRowsAffected();
    } else {
        qDebug() << "CloudDriveModel::fixDamagedDB DELETE is failed. Error" << query.lastError() << query.lastQuery();
    }

    // TODO Workaround. Delete duplicated unique item to fix QSqlError 19 indexed columns are not unique.
    QSqlQuery deleteDuplicatedPS(DatabaseManager::defaultManager().getDB());
    deleteDuplicatedPS.prepare("DELETE FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path AND rowid < :rowid;");

    res = query.exec("SELECT type, uid, local_path, count(*) c, max(rowid) max_rowid FROM cloud_drive_item GROUP BY type, uid, local_path HAVING count(*) > 1;");
    if (res) {
        qDebug() << "CloudDriveModel::fixDamagedDB find duplicated unique key. numRowsAffected" << query.numRowsAffected();
        QSqlRecord rec = query.record();
        while (query.next()) {
            // Process pending events.
            QApplication::processEvents();

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

    DatabaseManager::defaultManager().getDB().commit();

    // Process pending events.
    QApplication::processEvents();
}

void CloudDriveModel::initializeDB()
{
    // Prepare queries.
    m_selectByPrimaryKeyPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_selectByPrimaryKeyPS.prepare("SELECT * FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path");

    m_selectByTypePS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_selectByTypePS.prepare("SELECT * FROM cloud_drive_item WHERE type = :type ORDER BY uid, local_path");

    m_selectByTypeAndUidPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_selectByTypeAndUidPS.prepare("SELECT * FROM cloud_drive_item WHERE type = :type AND uid = :uid ORDER BY local_path");

    m_selectByLocalPathPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_selectByLocalPathPS.prepare("SELECT * FROM cloud_drive_item WHERE local_path = :local_path ORDER BY type, uid");

    m_selectChildrenByPrimaryKeyPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_selectChildrenByPrimaryKeyPS.prepare("SELECT * FROM cloud_drive_item c WHERE c.local_path like :local_path||'/%' and c.type = :type and c.uid = :uid ORDER BY c.local_path");

    m_insertPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_insertPS.prepare("INSERT INTO cloud_drive_item(type, uid, local_path, remote_path, hash, last_modified, sync_direction)"
                       " VALUES (:type, :uid, :local_path, :remote_path, :hash, :last_modified, :sync_direction)");

    m_updatePS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_updatePS.prepare("UPDATE cloud_drive_item SET"
                       " remote_path = :remote_path, hash = :hash, last_modified = :last_modified, sync_direction = :sync_direction"
                       " WHERE type = :type AND uid = :uid AND local_path = :local_path");

    m_updateHashByLocalPathPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_updateHashByLocalPathPS.prepare("UPDATE cloud_drive_item SET"
                       " hash = :hash"
                       " WHERE local_path = :local_path");

    m_deletePS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_deletePS.prepare("DELETE FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :local_path");

    m_countPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_countPS.prepare("SELECT count(*) count FROM cloud_drive_item");

    m_countByLocalPathPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_countByLocalPathPS.prepare("SELECT count(*) count FROM cloud_drive_item where local_path = :local_path");
}

bool CloudDriveModel::updateDropboxPrefix(bool fullAccess)
{
    qDebug() << "CloudDriveModel::updateDropboxPrefix started. fullAccess" << fullAccess;
    QSqlQuery updQry(DatabaseManager::defaultManager().getDB());
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
        DatabaseManager::defaultManager().getDB().commit();

        // Clear item cache.
        m_itemCache->clear();
        qDebug() << "CloudDriveModel::updateDropboxPrefix itemCache is cleared.";
    } else {
        DatabaseManager::defaultManager().getDB().rollback();
    }

    return res;
}

bool CloudDriveModel::testConnection(CloudDriveModel::ClientTypes type, QString uid, QString hostname, QString username, QString password, QString token, QString authHostname)
{
    return getCloudClient(type)->testConnection(uid, hostname, username, password, token, authHostname);
}

bool CloudDriveModel::saveConnection(CloudDriveModel::ClientTypes type, QString uid, QString hostname, QString username, QString password, QString token)
{
    return getCloudClient(type)->saveConnection(uid, hostname, username, password, token);
}

QVariant CloudDriveModel::getConnectionProperty(CloudDriveModel::ClientTypes type, QString uid, QString name, QVariant defaultValue)
{
    QString clientObjectName = getCloudClient(type)->objectName();
    return m_settings.value(QString("%1.%2.%3").arg(clientObjectName).arg(uid).arg(name), defaultValue);
}

bool CloudDriveModel::getConnectionBoolProperty(CloudDriveModel::ClientTypes type, QString uid, QString name, bool defaultValue)
{
    QString clientObjectName = getCloudClient(type)->objectName();
    return m_settings.value(QString("%1.%2.%3").arg(clientObjectName).arg(uid).arg(name), defaultValue).toBool();
}

void CloudDriveModel::setConnectionProperty(CloudDriveModel::ClientTypes type, QString uid, QString name, QVariant value)
{
    QString clientObjectName = getCloudClient(type)->objectName();
    m_settings.setValue(QString("%1.%2.%3").arg(clientObjectName).arg(uid).arg(name), value);
}

QString CloudDriveModel::getItemCacheKey(int type, QString uid, QString localPath)
{
    return QString("%1,%2,%3").arg(type).arg(uid).arg(localPath);
}

void CloudDriveModel::cleanDB()
{
    QSqlQuery updQry(DatabaseManager::defaultManager().getDB());
    bool res = false;
    updQry.prepare("DELETE FROM cloud_drive_item WHERE local_path = ''");
    res = updQry.exec();
    if (res) {
        qDebug() << "CloudDriveModel::cleanDB" << updQry.lastQuery() << "res" << res << "numRowsAffected" << updQry.numRowsAffected();
        DatabaseManager::defaultManager().getDB().rollback();
    } else {
        qDebug() << "CloudDriveModel::cleanDB" << updQry.lastQuery() << "res" << res << "error" << updQry.lastError();
        DatabaseManager::defaultManager().getDB().commit();
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
                item.syncDirection = ps.value(rec.indexOf("sync_direction")).toInt();

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
    QSqlQuery query(DatabaseManager::defaultManager().getDB());
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
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
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
    m_insertPS.bindValue(":sync_direction", item.syncDirection);
    res = m_insertPS.exec();
    if (res) {
        if (!suppressMessages) {
            qDebug() << "CloudDriveModel::insertItemToDB insert done" << item << "res" << res << "numRowsAffected" << m_insertPS.numRowsAffected();
        }
        DatabaseManager::defaultManager().getDB().commit();

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
    m_updatePS.bindValue(":sync_direction", item.syncDirection);
    res = m_updatePS.exec();
    if (res) {
        if (!suppressMessages) {
            qDebug() << "CloudDriveModel::updateItemToDB update done" << item << "res" << res << "numRowsAffected" << m_updatePS.numRowsAffected();
        }
        DatabaseManager::defaultManager().getDB().commit();

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
        DatabaseManager::defaultManager().getDB().commit();

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
        DatabaseManager::defaultManager().getDB().commit();

        // Remove item from itemCache.
        m_itemCache->remove(itemCacheKey);
    } else {
        qDebug() << "CloudDriveModel::deleteItemToDB delete failed" << itemCacheKey << "res" << res << m_deletePS.lastError();
    }

    return m_deletePS.numRowsAffected();
}

void CloudDriveModel::removeItemCacheWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    // NOTE It works if parent item is found. Otherwise, nothing will be removed.
    QString itemKey = getItemCacheKey(type, uid, localPath);
    QMap<QString, CloudDriveItem>::iterator it = m_itemCache->find(itemKey);
    while (it != m_itemCache->end()) {
        QApplication::processEvents();

        QMap<QString, CloudDriveItem>::iterator prev = it;
        ++it;

        QString k = prev.key();
        if (k == itemKey || k.startsWith(itemKey + "/")) {
            qDebug() << "CloudDriveModel::removeItemCacheWithChildren erase cache key" << k;
            m_itemCache->erase(prev);
        } else {
            qDebug() << "CloudDriveModel::removeItemCacheWithChildren break cache key" << k;
            break;
        }
    }
}

int CloudDriveModel::deleteItemWithChildrenFromDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath)
{
    qDebug() << "CloudDriveModel::deleteItemWithChildrenFromDB" << type << uid << localPath;

    int res = 0;
    QSqlQuery qry("DELETE FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path = :localPath", DatabaseManager::defaultManager().getDB());
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":localPath", localPath);
    if (qry.exec()) {
        res += qry.numRowsAffected();
    }
    QSqlQuery childrenQry("DELETE FROM cloud_drive_item WHERE type = :type AND uid = :uid AND local_path LIKE :localPath", DatabaseManager::defaultManager().getDB());
    childrenQry.bindValue(":type", type);
    childrenQry.bindValue(":uid", uid);
    childrenQry.bindValue(":localPath", localPath + "/%");
    if (childrenQry.exec()) {
        res += childrenQry.numRowsAffected();
    }

    // Remove related cache items.
    removeItemCacheWithChildren(type, uid, localPath);

    qDebug() << "CloudDriveModel::deleteItemWithChildrenFromDB" << type << uid << localPath << "res" << res;

    return res;
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

    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
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
    m_cloudDriveJobs->insert(job.jobId, job);
//    updateJob(job, false); // Omit signal as it's too noisy and freeze UI on job page.

//    emit uploadProgress(nonce, bytesSent, bytesTotal);
}

void CloudDriveModel::downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);
//    qDebug() << "CloudDriveModel::downloadProgressFilter" << nonce << getOperationName(job.operation) << bytesReceived << bytesTotal << job.downloadOffset;
    job.bytes = job.downloadOffset + bytesReceived; // Add job.downloadOffset to support fileGetResume.
    m_cloudDriveJobs->insert(job.jobId, job);
//    updateJob(job, false); // Omit signal as it's too noisy and freeze UI on job page.

//    emit downloadProgress(nonce, bytesReceived, bytesTotal);
}

void CloudDriveModel::jobDone() {
    mutex.lock();
    runningJobCount--;
    runningJobCount = (runningJobCount < 0) ? 0 : runningJobCount;
    mutex.unlock();

    emit runningJobCountChanged();

    qDebug() << "CloudDriveModel::jobDone runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue->count() << "m_cloudDriveJobs" << m_cloudDriveJobs->count();
}

bool CloudDriveModel::isSyncOperation(int operation)
{
    switch (operation) {
    case FileGet:
    case FilePut:
    case Metadata:
    case ScheduleSync:
    case MigrateFile:
    case MigrateFilePut:
    case Delta:
    case SyncFromLocal:
    case FilePutResume:
    case FilePutCommit:
    case FileGetResume:
    case FileGetCommit:
        return true;
    default:
        return false;
    }
}

void CloudDriveModel::proceedNextJob() {
    // Proceed next job in queue. Any jobs which haven't queued will be ignored.
//    if (runningJobCount > 0) {
//        qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "CloudDriveModel::proceedNextJob waiting runningJobCount" << runningJobCount
//                 << "m_browseThreadPool" << m_browseThreadPool.activeThreadCount() << "/" << m_browseThreadPool.maxThreadCount()
//                 << "m_jobQueue" << m_jobQueue->count() << "m_cloudDriveJobs" << m_cloudDriveJobs->count() << "m_isSuspended" << m_isSuspended << "m_isAborted" << m_isAborted << "m_isPaused" << m_isPaused
//                 << "m_threadHash count" << m_threadHash->count();
//    }

    // Emit status signal.
    emit jobQueueStatusSignal(runningJobCount, m_jobQueue->count(), m_cloudDriveJobs->count(), getItemCount());

    if (runningJobCount >= MaxRunningJobCount || m_jobQueue->count() <= 0 || m_isSuspended || m_isAborted || m_isPaused) {
        // Check if there is no running job and is aborted.
        if (runningJobCount <= 0 && m_isAborted) {
            QApplication::quit();
        }

        return;
    }

    QString nonce = m_jobQueue->dequeue();
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    if (isSyncOperation(job.operation)
            && m_settings.value("CloudDriveModel.proceedNextJob.syncOnlyOnCharging.enabled", false).toBool()
            && !isDeviceBatteryCharging()) {
        qDebug() << "CloudDriveModel::proceedNextJob suspended isDeviceBatteryCharging()" << isDeviceBatteryCharging();
        job.err = -1;
        job.errString = tr("Suspend");
        job.errMessage = tr("Sync only on charging is enabled.");
        updateJob(job);
        emit logRequestSignal(nonce, "warn", job.errString, job.errMessage, 2000);
        return;
    }

    if (isSyncOperation(job.operation)
            && m_settings.value("CloudDriveModel.proceedNextJob.syncOnlyOnBatteryOk.enabled", false).toBool()
            && m_deviceInfo.batteryStatus() < QSystemDeviceInfo::BatteryNormal) {
        qDebug() << "CloudDriveModel::proceedNextJob suspended m_deviceInfo.batteryStatus()" << m_deviceInfo.batteryStatus() << "m_deviceInfo.batteryLevel()" << m_deviceInfo.batteryLevel();
        job.err = -1;
        job.errString = tr("Suspend");
        job.errMessage = tr("Sync only if battery is OK (>40%) is enabled.");
        updateJob(job);
        emit logRequestSignal(nonce, "warn", job.errString, job.errMessage, 2000);
        return;
    }

    if (isSyncOperation(job.operation)
            && m_settings.value("CloudDriveModel.proceedNextJob.syncOnlyOnWlan.enabled", false).toBool()
            && m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode) != QSystemNetworkInfo::Connected) {
        qDebug() << "CloudDriveModel::proceedNextJob suspended m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode)" << m_networkInfo.networkStatus(QSystemNetworkInfo::WlanMode);
        job.err = -1;
        job.errString = tr("Suspend");
        job.errMessage = tr("Sync only on WiFi is enabled.");
        updateJob(job);
        emit logRequestSignal(nonce, "warn", job.errString, job.errMessage, 2000);
        return;
    }

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

    qDebug() << "CloudDriveModel::proceedNextJob proceed jobId" << nonce << "operation" << getOperationName(job.operation) << "runningJobCount" << runningJobCount << "jobCount" << m_cloudDriveJobs->count();

    // Increase runningJobCount.
    mutex.lock();
    runningJobCount++;
    mutex.unlock();
    emit runningJobCountChanged();

    // Dispatch job.
    CloudDriveModelThread *t = new CloudDriveModelThread(this);
    t->setNonce(nonce); // Set job ID to thread. It will invoke parent's dispatchJob later.
    t->setDirectInvokation(false);

    // These are blocking operations.
    switch (job.operation) {
    case LoadCloudDriveJobs:
    case InitializeCloudClients:
    case Disconnect:
    case DeleteLocal:
    case SyncFromLocal:
    case MigrateFilePut:
    case RemoveJobs:
        t->setDirectInvokation(true);
        break;
    }

    // TODO NOTE Try to avoid thread panic by invoking dispatchJob() directly for any QNAM implementation methods.
    if (t->isDirectInvokation()) {
        QThread *thread = new QThread(this);
        m_threadHash->insert(t->getNonce(), thread);
        connect(thread, SIGNAL(started()), t, SLOT(start()));
        connect(thread, SIGNAL(finished()), t, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        connect(thread, SIGNAL(finished()), this, SLOT(threadFinishedFilter()));
        thread->start(QThread::LowPriority);

        qDebug() << "CloudDriveModel::proceedNextJob jobId" << nonce << "is started. runningJobCount" << runningJobCount << "m_threadHash count" << m_threadHash->count();
    } else {
        qDebug() << "CloudDriveModel::proceedNextJob jobId" << nonce << "is dispatching. runningJobCount" << runningJobCount << "m_threadHash count" << m_threadHash->count();
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
    case Box:
        return boxClient;
    default:
        qDebug() << "CloudDriveModel::getCloudClient type" << type << "is not implemented yet.";
    }

    // NOTE Return default to avoid crash while invoking method on null pointer.
    return defaultClient;
}

CloudDriveClient * CloudDriveModel::getCloudClient(const int type)
{
    return getCloudClient(getClientType(type));
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

    qDebug() << "CloudDriveModel::dispatchJob thread" << QThread::currentThread() << "job" << job.jobId << job.operation << getOperationName(job.operation) << job.type << cloudClient->objectName();

    // Update job status and emit signal to update UI.
    job.isRunning = true;
    updateJob(job);

    switch (job.operation) {
    case LoadCloudDriveJobs:
        loadCloudDriveJobs(job.jobId);
        break;
    case InitializeCloudClients:
        initializeCloudClients(job.jobId);
        break;
    case FileGet:
        // Remove existing file if it's new job.
        if (job.downloadOffset == 0) {
            QFile(job.localFilePath).remove();
        }
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
            migrateFilePutResume_Block(job.jobId, getClientType(job.type), job.uid, job.remoteFilePath, job.bytesTotal, getClientType(job.targetType), job.targetUid, job.newRemoteFilePath, job.newRemoteFileName);
        } else {
            migrateFilePut_Block(job.jobId, getClientType(job.type), job.uid, job.remoteFilePath, job.bytesTotal, getClientType(job.targetType), job.targetUid, job.newRemoteFilePath, job.newRemoteFileName);
        }
        break;
    case Disconnect:
        removeItemWithChildren(getClientType(job.type), job.uid, job.localFilePath);
        refreshRequestFilter(job.jobId);
        break;
    case DeleteLocal:
        removeItemWithChildren(getClientType(job.type), job.uid, job.localFilePath);
        if (!isConnected(job.localFilePath)) {
            requestMoveToTrash(job.jobId, job.localFilePath);
        } else {
            qDebug() << "CloudDriveModel::dispatchJob DeleteLocal localFilePath" << job.localFilePath << "is still connected. Invoking requestMoveToTrash() is suppressed.";
            refreshRequestFilter(job.jobId);
        }
        break;
    case SyncFromLocal:
        syncFromLocal_Block(job.jobId, getClientType(job.type), job.uid, job.localFilePath, job.remoteFilePath, job.modelIndex, job.forcePut, true, job.remotePathList);
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
    case RemoveJobs:
        m_jobQueue->clear();
        removeJobs();
        jobDone();
        removeJob("CloudDriveModel::dispatchJob", job.jobId);
    }
}

void CloudDriveModel::suspendJob(const QString jobId)
{
    qDebug() << "CloudDriveModel::suspendJob" << jobId;

    // Suspend job.
    CloudDriveJob job = m_cloudDriveJobs->value(jobId);

    // Actually abort job.
    CloudDriveClient *client = getCloudClient(getClientType(job.type));
    if (client->isAuthorized()) {
        if (!client->abort(jobId)) {
            // Job is not aborted. It may finish already. Then fail job.
            job.err = -1;
            job.errString = "Operation is aborted manually.";
            job.isRunning = false;
            updateJob(job);
            jobDone();
        }
    }
}

void CloudDriveModel::resumeJob(const QString jobId)
{
    // Enqueue job.
    CloudDriveJob job = m_cloudDriveJobs->value(jobId);
    if (!job.isRunning && !m_jobQueue->contains(jobId)) {
        job.retryCount = 0; // Reset retry count.
        updateJob(job, false);
        m_jobQueue->enqueue(jobId);
    }
}

void CloudDriveModel::suspendScheduledJob()
{
    m_isSchedulerSuspended = true;
}

void CloudDriveModel::resumeScheduledJob()
{
    m_isSchedulerSuspended = false;
}

bool CloudDriveModel::isPaused()
{
    return m_isPaused;
}

void CloudDriveModel::setIsPaused(bool pause)
{
    m_isPaused = pause;
}

void CloudDriveModel::resetRunningJobCount()
{
    mutex.lock();
    runningJobCount = 0;
    mutex.unlock();
    emit runningJobCountChanged();

    qDebug() << "CloudDriveModel::resetRunningJobCount runningJobCount" << runningJobCount << " m_jobQueue" << m_jobQueue->count() << "m_cloudDriveJobs" << m_cloudDriveJobs->count();
}

void CloudDriveModel::threadFinishedFilter()
{
    qDebug() << "CloudDriveModel::threadFinishedFilter" << sender();
}

/*
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

    if (err == QNetworkReply::NoError) {
        authorize(getClientType(job.type));
    }

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit requestTokenReplySignal(nonce, err, errMsg, msg);

    removeJob("CloudDriveModel::requestTokenReplyFilter", nonce);
}

void CloudDriveModel::authorizeRedirectFilter(QString nonce, QString url, QString redirectFrom)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit authorizeRedirectSignal(nonce, url, redirectFrom);

    removeJob("CloudDriveModel::authorizeRedirectFilter", nonce);
}

void CloudDriveModel::accessTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    job.isRunning = false;
    updateJob(job);

    qDebug() << "CloudDriveModel::accessTokenReplyFilter" << nonce << err << errMsg << msg;

    if (err == QNetworkReply::NoError) {
        // Check if job.nextJobId is specified. (From quota request)
        if (job.nextJobId != "") {
            // Insert job.nextJobId to first in queue.
            qDebug() << "CloudDriveModel::accessTokenReplyFilter insert job.nextJobId" << job.nextJobId;
            m_jobQueue->insert(0, job.nextJobId);
        } else {
            QScriptEngine engine;
            QScriptValue sc;
            QHash<QString, QString> m_paramMap;

            // Proceed to accountInfo for authorization's accessToken.
            switch (job.type) {
            case Dropbox:
                // Dropbox provides uid but no email yet. Get email from accountInfo.
                sc = engine.evaluate("(" + msg + ")");
                m_paramMap["access_token"] = sc.property("access_token").toString();
                m_paramMap["uid"] = sc.property("uid").toString();
                qDebug() << "CloudDriveModel::accessTokenReplyFilter Dropbox uid" << m_paramMap["uid"];

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

    removeJob("CloudDriveModel::accessTokenReplyFilter", nonce);
}

void CloudDriveModel::accountInfoReplyFilter(QString nonce, int err, QString errMsg, QString msg)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Check if token is expired (err 204), then refresh token.
    if (err == QNetworkReply::AuthenticationRequiredError) {
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit accountInfoReplySignal(nonce, err, errMsg, msg);

    removeJob("CloudDriveModel::accountInfoReplyFilter", nonce);
}

void CloudDriveModel::quotaReplyFilter(QString nonce, int err, QString errMsg, QString msg, qint64 normalBytes, qint64 sharedBytes, qint64 quotaBytes)
{
    CloudDriveJob job = m_cloudDriveJobs->value(nonce);

    // Check if token is expired (err 204), then refresh token.
    if (err == QNetworkReply::AuthenticationRequiredError) {
        refreshToken(getClientType(job.type), job.uid, job.jobId);
        // Update job status but keep job for resume after token is refreshed.
        job.isRunning = false;
        updateJob(job);
        jobDone();
        return;
    }

    job.isRunning = false;
    updateJob(job);

    // Notify job done.
    jobDone();

    emit quotaReplySignal(nonce, err, errMsg, msg, normalBytes, sharedBytes, quotaBytes);

    removeJob("CloudDriveModel::quotaReplyFilter", nonce);
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

QVariant CloudDriveModel::getCloudDriveItemProperty(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString propertyName, QVariant defaultValue)
{
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
    qry.prepare("SELECT property_value FROM cloud_drive_item_property WHERE type = :type AND uid = :uid AND remote_path = :remote_path AND property_name = :property_name");
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remoteFilePath);
    qry.bindValue(":property_name", propertyName);
    if (qry.exec()) {
        if (qry.first()) {
            QSqlRecord rec = qry.record();
            QVariant value = qry.value(rec.indexOf("property_value"));
            qDebug() << "CloudDriveModel::getCloudDriveItemProperty done" << type << uid << remoteFilePath << propertyName << value;
            return value;
        } else {
            qDebug() << "CloudDriveModel::getCloudDriveItemProperty done" << type << uid << remoteFilePath << propertyName << "defaultValue" << defaultValue;
            return defaultValue;
        }
    } else {
        qDebug() << "CloudDriveModel::getCloudDriveItemProperty failed" << type << uid << remoteFilePath << propertyName << "error" << qry.lastError() << "defaultValue" << defaultValue;
        return defaultValue;
    }
}

int CloudDriveModel::getSortFlag(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath)
{
    if (m_sortFlag == -1) {
        m_sortFlag = getCloudDriveItemProperty(type, uid, remoteFilePath, "sortFlag", QVariant(SortByName)).toInt();
    }

    return m_sortFlag;
}

bool CloudDriveModel::setCloudDriveItemProperty(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString propertyName, QVariant value)
{
    QSqlQuery qry(DatabaseManager::defaultManager().getDB());
    qry.prepare("UPDATE cloud_drive_item_property SET property_value = :property_value WHERE type = :type AND uid = :uid AND remote_path = :remote_path AND property_name = :property_name");
    qry.bindValue(":property_value", value);
    qry.bindValue(":type", type);
    qry.bindValue(":uid", uid);
    qry.bindValue(":remote_path", remoteFilePath);
    qry.bindValue(":property_name", propertyName);
    if (qry.exec()) {
        int c = qry.numRowsAffected();
        DatabaseManager::defaultManager().getDB().commit();
        if (c > 0) {
            qDebug() << "CloudDriveModel::setCloudDriveItemProperty done" << type << uid << remoteFilePath << propertyName << value << "numRowsAffected" << c;
            return true;
        } else {
            // Insert new record.
            qry.prepare("INSERT INTO cloud_drive_item_property VALUES (:type, :uid, :remote_path, :property_name, :property_value)");
            qry.bindValue(":type", type);
            qry.bindValue(":uid", uid);
            qry.bindValue(":remote_path", remoteFilePath);
            qry.bindValue(":property_name", propertyName);
            qry.bindValue(":property_value", value);
            if (qry.exec()) {
                qDebug() << "CloudDriveModel::setCloudDriveItemProperty insert done" << type << uid << remoteFilePath << propertyName << value;
                DatabaseManager::defaultManager().getDB().commit();
                return true;
            } else {
                qDebug() << "CloudDriveModel::setCloudDriveItemProperty insert failed" << type << uid << remoteFilePath << propertyName << value << "error" << qry.lastError();
                DatabaseManager::defaultManager().getDB().rollback();
                return false;
            }
        }
    } else {
        qDebug() << "CloudDriveModel::setCloudDriveItemProperty failed" << type << uid << remoteFilePath << propertyName << value << "error" << qry.lastError();
        DatabaseManager::defaultManager().getDB().rollback();
        return false;
    }
}

void CloudDriveModel::setSortFlag(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, int sortFlag)
{
    if (setCloudDriveItemProperty(type, uid, remoteFilePath, "sortFlag", QVariant(sortFlag))) {
        m_sortFlag = sortFlag;
    }

    // Sorting.
    // TODO Sort in thread.
    sortItemList(m_modelItemList, m_sortFlag);

    // Emit data changed.
    beginResetModel();
    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
    endResetModel();

    emit browseReplySignal(createNonce(), 0, "Set sort flag", "{}", false);
}

bool CloudDriveModel::isAnyItemChecked()
{
    for (int i = 0; i < m_modelItemList->length(); i++) {
        QApplication::processEvents();
        if (m_modelItemList->at(i).isChecked) {
            return true;
        }
    }

    return false;
}

bool CloudDriveModel::areAllItemChecked()
{
    for (int i = 0; i < m_modelItemList->length(); i++) {
        QApplication::processEvents();
        if (!m_modelItemList->at(i).isChecked) {
            return false;
        }
    }

    return (m_modelItemList->length() > 0);
}

void CloudDriveModel::markAll()
{
    for (int i = 0; i < m_modelItemList->length(); i++) {
        QApplication::processEvents();
        CloudDriveModelItem item = m_modelItemList->at(i);
        item.isChecked = true;
        m_modelItemList->replace(i, item);
    }

    refreshItems();
}

void CloudDriveModel::markAllFiles()
{
    for (int i = 0; i < m_modelItemList->length(); i++) {
        QApplication::processEvents();
        CloudDriveModelItem item = m_modelItemList->at(i);
        if (!item.isDir) {
            item.isChecked = true;
            m_modelItemList->replace(i, item);
        }
    }

    refreshItems();
}

void CloudDriveModel::markAllFolders()
{
    for (int i = 0; i < m_modelItemList->length(); i++) {
        QApplication::processEvents();
        CloudDriveModelItem item = m_modelItemList->at(i);
        if (item.isDir) {
            item.isChecked = true;
            m_modelItemList->replace(i, item);
        }
    }

    refreshItems();
}

void CloudDriveModel::unmarkAll()
{
    for (int i = 0; i < m_modelItemList->length(); i++) {
        QApplication::processEvents();
        CloudDriveModelItem item = m_modelItemList->at(i);
        item.isChecked = false;
        m_modelItemList->replace(i, item);
    }

    refreshItems();
}

void CloudDriveModel::clearCachedImagesOnCurrentRemotePath(bool clearThumbnail, bool clearThumbnail128, bool clearPreview)
{
#if defined(Q_WS_HARMATTAN)
    const QString IMAGE_CACHE_PATH = "/home/user/MyDocs/temp/.filesplus";
#else
    const QString IMAGE_CACHE_PATH = "E:/temp/.filesplus";
#endif

    for (int i = 0; i < m_modelItemList->length(); i++) {
        QApplication::processEvents();
        CloudDriveModelItem item = m_modelItemList->at(i);

        if (clearThumbnail) {
            QFile(CacheImageWorker::getCachedRemotePath(item.thumbnail, QSize(48, 48), m_settings.value("image.cache.path", IMAGE_CACHE_PATH).toString())).remove();
        }
        if (clearThumbnail128) {
            QFile(CacheImageWorker::getCachedRemotePath(item.thumbnail128, QSize(128, 128), m_settings.value("image.cache.path", IMAGE_CACHE_PATH).toString())).remove();
            QFile(CacheImageWorker::getCachedRemotePath(item.thumbnail128, QSize(160, 160), m_settings.value("image.cache.path", IMAGE_CACHE_PATH).toString())).remove();
        }
        if (clearPreview) {
            QFile(CacheImageWorker::getCachedRemotePath(item.preview, QSize(-1, -1), m_settings.value("image.cache.path", IMAGE_CACHE_PATH).toString())).remove();
        }
    }

    refreshItems();
}

void CloudDriveModel::refreshItems()
{
    beginResetModel();
    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
    endResetModel();
}

int CloudDriveModel::findIndexByRemotePath(QString remotePath)
{
    for (int i=0; i<m_modelItemList->count(); i++) {
        if (m_modelItemList->at(i).absolutePath == remotePath) {
            return i;
        }
    }

    return -1;
}

int CloudDriveModel::findIndexByRemotePathName(QString remotePathName) {
    for (int i=0; i<m_modelItemList->count(); i++) {
        if (m_modelItemList->at(i).name == remotePathName) {
            return i;
        }
    }

    return -1;
}

int CloudDriveModel::findIndexByNameFilter(QString nameFilter, int startIndex, bool backward)
{
    QRegExp rx(nameFilter, Qt::CaseInsensitive);

    if (backward) {
        startIndex = (startIndex == -1) ? (m_modelItemList->count() - 1) : startIndex;
        startIndex = (startIndex < 0 || startIndex >= m_modelItemList->count()) ? (m_modelItemList->count() - 1) : startIndex;
        for (int i=startIndex; i>=0; i--) {
            if (rx.indexIn(m_modelItemList->at(i).name) != -1) {
                return i;
            }
        }
    } else {
        startIndex = (startIndex == -1) ? 0 : startIndex;
        startIndex = (startIndex < 0 || startIndex >= m_modelItemList->count()) ? 0 : startIndex;
        for (int i=startIndex; i<m_modelItemList->count(); i++) {
            if (rx.indexIn(m_modelItemList->at(i).name) != -1) {
                return i;
            }
        }
    }

    return -1;
}

void CloudDriveModel::sortItemList(QList<CloudDriveModelItem> *itemList, int sortFlag)
{
    QDateTime start = QDateTime::currentDateTime();
    qDebug() << start.toString(Qt::ISODate) << "CloudDriveModel::sortItemList sortFlag" << sortFlag << "itemList->length()" << itemList->length();

    switch (sortFlag) {
    case SortByName:
        qSort(itemList->begin(), itemList->end(), nameLessThan);
        break;
    case SortByNameWithDirectoryFirst:
        qSort(itemList->begin(), itemList->end(), nameLessThanWithDirectoryFirst);
        break;
    case SortByTime:
        qSort(itemList->begin(), itemList->end(), timeGreaterThan);
        break;
    case SortByType:
        qSort(itemList->begin(), itemList->end(), typeLessThan);
        break;
    case SortBySize:
        qSort(itemList->begin(), itemList->end(), sizeGreaterThan);
        break;
    }

    QDateTime end = QDateTime::currentDateTime();
    qDebug() << end.toString(Qt::ISODate) << "CloudDriveModel::sortItemList done elapse" << start.msecsTo(end) << "ms. itemList->length()" << itemList->length();
}

int CloudDriveModel::compareMetadata(CloudDriveJob job, QScriptValue &jsonObj, QString localFilePath)
{
    CloudDriveClient *client = getCloudClient(job.type);
    QFileInfo localFileInfo(localFilePath);
    CloudDriveItem item = getItem(localFilePath, getClientType(job.type), job.uid);
    QDateTime jsonObjLastModified = parseJSONDateString(jsonObj.property("lastModified").toString());
    int result = 0;

    if (jsonObj.property("isDir").toBool()) {
        QDir localDir(localFilePath);
        localDir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        int itemCount = localDir.entryList().count();
        result = client->compareDirMetadata(job, jsonObj, localFilePath, item);
        qDebug() << "CloudDriveModel::compareMetadata dir"
                 << getCloudName(job.type)
                 << "remote path" << jsonObj.property("absolutePath").toString() << "hash" << jsonObj.property("hash").toString() << "childrenCount" << jsonObj.property("children").property("length").toInteger() << "lastModified" << jsonObj.property("lastModified").toString() << jsonObjLastModified
                 << "local path" << localFilePath << "hash" << item.hash << "itemCount" << itemCount << "lastModified" << localFileInfo.lastModified()
                 << "forcePut" << job.forcePut << "forceGet" << job.forceGet << "result" << result;
    } else {
        result = client->compareFileMetadata(job, jsonObj, localFilePath, item);
        qDebug() << "CloudDriveModel::compareMetadata file"
                 << getCloudName(job.type)
                 << "remote path" << jsonObj.property("absolutePath").toString() << "hash" << jsonObj.property("hash").toString() << "size" << jsonObj.property("size").toInt32() << "lastModified" << jsonObj.property("lastModified").toString() << jsonObjLastModified
                 << "local path" << localFilePath << "hash" << item.hash << "size" << localFileInfo.size() << "lastModified" << localFileInfo.lastModified()
                 << "forcePut" << job.forcePut << "forceGet" << job.forceGet << "result" << result;
    }

    return result;
}

bool CloudDriveModel::isDeviceBatteryCharging()
{
#if defined(Q_WS_HARMATTAN)
    return (m_batteryInfo.getChargingState() == MeeGo::QmBattery::StateCharging);
#else
    return (m_batteryInfo.chargingState() == QSystemBatteryInfo::Charging);
#endif
}
