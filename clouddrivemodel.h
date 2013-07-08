#ifndef CLOUDDRIVEMODEL_H
#define CLOUDDRIVEMODEL_H

#include <QDeclarativeItem>
#include <QtNetwork>
#include <QHash>
#include <QMultiHash>
#include <QThread>
#include <QAbstractListModel>
#include <QFile>
#include <QScriptEngine>
#include <QScriptValue>
#include <QThread>
#include <QDebug>
#include <QApplication>
#include <QtSql>
#include <QSettings>
#include <QSystemNetworkInfo>
#include <QSystemBatteryInfo>
#include "clouddriveitem.h"
#include "clouddrivejob.h"
#include "clouddrivemodelthread.h"
#include "clouddrivemodelitem.h"
#include "clouddriveclient.h"
#include "dropboxclient.h"
#include "gcdclient.h"
#include "skydriveclient.h"
#include "ftpclient.h"
#include "webdavclient.h"
#include "boxclient.h"
#include "cacheimageworker.h"

using namespace QtMobility;

class CloudDriveModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(ClientTypes)
    Q_ENUMS(SortFlags)
    Q_ENUMS(Operations)
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int runningJobCount READ getRunningJobCount NOTIFY runningJobCountChanged)
    Q_PROPERTY(int queuedJobCount READ getQueuedJobCount NOTIFY jobQueueStatusSignal)
    Q_PROPERTY(int jobCount READ getJobCount NOTIFY jobQueueStatusSignal)
    Q_PROPERTY(int itemCount READ getItemCount NOTIFY jobQueueStatusSignal)
    Q_PROPERTY(QString dirtyHash READ dirtyHash CONSTANT)
    Q_PROPERTY(QString remoteParentPath READ getRemoteParentPath WRITE setRemoteParentPath NOTIFY remoteParentPathChanged)
    Q_PROPERTY(QString remoteParentPathName READ getRemoteParentPathName WRITE setRemoteParentPathName NOTIFY remoteParentPathChanged)
    Q_PROPERTY(QString remoteParentParentPath READ getRemoteParentParentPath WRITE setRemoteParentParentPath NOTIFY remoteParentPathChanged)
    Q_PROPERTY(int selectedIndex READ getSelectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString selectedRemotePath READ getSelectedRemotePath WRITE setSelectedRemotePath NOTIFY selectedIndexChanged)
public:
    static const QString ITEM_DAT_PATH;
    static const QString ITEM_DB_PATH;
    static const QString ITEM_DB_CONNECTION_NAME;
    static const QString TEMP_PATH;
    static const QString IMAGE_CACHE_PATH;
    static const QString JOB_DAT_PATH;
    static const int MaxRunningJobCount;
    static const QString DirtyHash;
    static const QStringList restrictFileTypes;

    // ClientTypes is stored on m_cloudDriveItems. Its sequence shouldn't be changed.
    // AnyClient is technically never stored. It should be the last type.
    enum ClientTypes {
        Dropbox = 0,
        GoogleDrive,
        SkyDrive,
        Ftp,
        WebDAV,
        Box
    };

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType,
        SortByNameWithDirectoryFirst
    };

    enum Operations {
        LoadCloudDriveItems,
        LoadCloudDriveJobs,
        InitializeDB,
        InitializeCloudClients,
        FileGet,
        FilePut,
        Metadata,
        CreateFolder,
        RequestToken,
        Authorize,
        AccessToken,
        RefreshToken,
        AccountInfo,
        Quota,
        DeleteFile,
        MoveFile,
        CopyFile,
        ShareFile,
        Browse,
        Disconnect,
        DeleteLocal,
        ScheduleSync,
        MigrateFile,
        MigrateFilePut,
        Delta,
        SyncFromLocal,
        FilePutResume,
        FilePutCommit,
        FileGetResume,
        FileGetCommit
    };

    explicit CloudDriveModel(QObject *parent = 0);
    ~CloudDriveModel();

    // List model methods.
    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    Q_INVOKABLE QVariant get(const int index);
    Q_INVOKABLE void setProperty(const int index, QString roleName, QVariant value);
    bool removeRow(int row, const QModelIndex & parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex());
    Q_INVOKABLE void clear();

    QString getRemoteParentPath();
    void setRemoteParentPath(QString remoteParentPath);
    QString getRemoteParentPathName();
    void setRemoteParentPathName(QString remoteParentPathName);
    QString getRemoteParentParentPath();
    void setRemoteParentParentPath(QString remoteParentParentPath);
    int getSelectedIndex();
    void setSelectedIndex(int index);
    QString getSelectedRemotePath();
    void setSelectedRemotePath(QString remotePath);

    QString dirtyHash() const;

    void addItem(QString localPath, CloudDriveItem item);
    QList<CloudDriveItem> getItemList(QString localPath);
    CloudDriveItem getItem(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    QList<CloudDriveItem> findItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    QList<CloudDriveItem> findItems(CloudDriveModel::ClientTypes type, QString uid);
    QList<CloudDriveItem> findItemsByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath, bool caseInsensitive = false);
    void cleanItems();
    bool cleanItem(const CloudDriveItem &item);

    // CloudDriveItem management.
    Q_INVOKABLE bool isConnected(QString localPath);
    Q_INVOKABLE bool isConnected(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE bool isRemotePathConnected(CloudDriveModel::ClientTypes type, QString uid, QString remotePath, bool showDebug = false);
    Q_INVOKABLE bool isDirty(QString localPath, QDateTime lastModified);
    Q_INVOKABLE bool isSyncing(QString localPath);
    Q_INVOKABLE bool isSyncing(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE bool isParentConnected(QString localPath);
    void clearConnectedRemoteDirtyCache(QString localPath, bool includeChildren = true);
    void clearLocalPathFlagCache(QMap<QString, bool> *localPathFlagCache, QString localPath, bool includeChildren = true);
    int clearRunningJobCache(QString localPath);
    Q_INVOKABLE bool isRemoteRoot(CloudDriveModel::ClientTypes type, QString uid, QString remotePath);
    Q_INVOKABLE bool canSync(QString localPath);
    Q_INVOKABLE QString getRunningJob(QString localPath);
    Q_INVOKABLE QString getJobJson(QString jobId);
    void updateJob(CloudDriveJob job, bool emitJobUpdatedSignal = true);
    Q_INVOKABLE void removeJob(QString caller, QString nonce);
    Q_INVOKABLE void removeJobs(bool removeAll = false);
    Q_INVOKABLE int getQueuedJobCount() const;
    Q_INVOKABLE int getRunningJobCount() const;
    Q_INVOKABLE int getJobCount() const;
    Q_INVOKABLE void cancelQueuedJobs();
    Q_INVOKABLE void addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash, bool addOnly = false);
    Q_INVOKABLE void removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE void removeItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE void removeItems(QString localPath);
    Q_INVOKABLE int removeItemByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath);
    Q_INVOKABLE void updateItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString hash);
    Q_INVOKABLE void updateItems(QString localPath, QString hash);
    Q_INVOKABLE void updateItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString newLocalPath, QString newRemotePath, QString newParentHash = "", QString newChildrenHash = "");
    Q_INVOKABLE void updateItemWithChildrenByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath, QString newRemotePath, QString newParentHash = "", QString newHash = "");
    Q_INVOKABLE int getItemCount();
    Q_INVOKABLE QString getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemListJson(QString localPath);
    Q_INVOKABLE QString getItemListJsonByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath);
    Q_INVOKABLE QString getItemJson(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getDefaultLocalFilePath(const QString &remoteFilePath); // TODO Avoid using.
    Q_INVOKABLE QString getDefaultRemoteFilePath(const QString &localFilePath); // TODO Avoid using.
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE bool isAuthorized(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE QStringList getStoredUidList();
    Q_INVOKABLE QStringList getStoredUidList(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE QString getStoredUid(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getEmail(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE int removeUid(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void requestJobQueueStatus();
    Q_INVOKABLE void suspendNextJob(bool abort = false);
    Q_INVOKABLE void resumeNextJob(bool resetAbort = false);
    Q_INVOKABLE void suspendJob(const QString jobId);
    Q_INVOKABLE void resumeJob(const QString jobId);
    Q_INVOKABLE void suspendScheduledJob();
    Q_INVOKABLE void resumeScheduledJob();
    Q_INVOKABLE bool isPaused();
    Q_INVOKABLE void setIsPaused(bool pause);

    // Other.
    Q_INVOKABLE QString getRemoteRoot(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getParentRemotePath(CloudDriveModel::ClientTypes type, QString remotePath);
    Q_INVOKABLE bool isRemoteAbsolutePath(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool isRemotePathCaseInsensitive(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool isConfigurable(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool isViewable(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool isSharable(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE bool isMediaEnabled(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE bool isImageUrlCachable(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool isUnicodeSupported(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool isDirtyBeforeSync(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE QString getRemoteName(CloudDriveModel::ClientTypes type, QString remotePath);
    Q_INVOKABLE QString getRemotePath(CloudDriveModel::ClientTypes type, QString remoteParentPath, QString remotePathName);
    Q_INVOKABLE QString getParentLocalPath(const QString absFilePath);
    Q_INVOKABLE bool isDir(const QString absFilePath);
    Q_INVOKABLE bool isFile(const QString absFilePath);
    Q_INVOKABLE QString getAbsolutePath(const QString dirPath, const QString fileName);
    Q_INVOKABLE bool createDirPath(const QString absPath);
    Q_INVOKABLE bool requestMoveToTrash(const QString nonce, const QString absPath);
    Q_INVOKABLE QString getFileName(const QString absFilePath);
    Q_INVOKABLE QString getFileType(QString localPath);
    Q_INVOKABLE qint64 getFileSize(QString localPath);
    Q_INVOKABLE QString getFileLastModified(QString localPath);
    Q_INVOKABLE QString getOperationName(int operation);
    Q_INVOKABLE QDateTime parseReplyDateString(CloudDriveModel::ClientTypes type, QString dateString);
    Q_INVOKABLE QString formatJSONDateString(QDateTime datetime);
    Q_INVOKABLE QString getPathFromUrl(QString urlString);
    Q_INVOKABLE QDateTime parseUTCDateString(QString utcString);
    Q_INVOKABLE QDateTime parseJSONDateString(QString jsonString);

    // Cloud type.
    ClientTypes getClientType(int typeInt);
    ClientTypes getClientType(QString typeText);
    Q_INVOKABLE QString getCloudName(int type);

    // Scheduler.
    Q_INVOKABLE int updateItemCronExp(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString cronExp);
    Q_INVOKABLE QString getItemCronExp(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    void dirtyScheduledItems(QString cronValue);
    void syncDirtyItems();

    // Delta.
    Q_INVOKABLE bool isDeltaSupported(CloudDriveModel::ClientTypes type);
    bool isDeltaEnabled(CloudDriveModel::ClientTypes type, QString uid);
    void scheduleDeltaJobs(QString cronValue);
    Q_INVOKABLE void setDeltaCronExp(CloudDriveModel::ClientTypes type, QString uid, QString cronExp);
    Q_INVOKABLE QString getDeltaCronExp(CloudDriveModel::ClientTypes type, QString uid);

    // Sync items.
    Q_INVOKABLE void syncItems(); // NOTE Used by Settings
    Q_INVOKABLE void syncItems(CloudDriveModel::ClientTypes type); // NOTE Used by syncItems()
    Q_INVOKABLE void syncItem(const QString localFilePath); // NOTE Used by main.qml, FolderPage.qml
    Q_INVOKABLE void syncItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE bool syncItemByRemotePath(CloudDriveModel::ClientTypes type, QString uid, QString remotePath, QString newHash = "", bool forcePut = false, bool forceGet = false); // NOTE Used by reply filter slots.

    // Migrate DAT to DB.
    Q_INVOKABLE int getCloudDriveItemsCount();
    Q_INVOKABLE void migrateCloudDriveItemsToDB();

    // Dropbox specific functions.
    Q_INVOKABLE void refreshDropboxClient();
    Q_INVOKABLE bool updateDropboxPrefix(bool fullAccess);

    // FTP/WebDAV specific functions.
    Q_INVOKABLE bool testConnection(CloudDriveModel::ClientTypes type, QString uid, QString hostname, QString username, QString password, QString token, QString authHostname = "");
    Q_INVOKABLE bool saveConnection(CloudDriveModel::ClientTypes type, QString uid, QString hostname, QString username, QString password,  QString token);
    Q_INVOKABLE QVariant getConnectionProperty(CloudDriveModel::ClientTypes type, QString uid, QString name, QVariant defaultValue);
    Q_INVOKABLE bool getConnectionBoolProperty(CloudDriveModel::ClientTypes type, QString uid, QString name, bool defaultValue);
    Q_INVOKABLE void setConnectionProperty(CloudDriveModel::ClientTypes type, QString uid, QString name, QVariant value);

    // TODO Needed ?
    Q_INVOKABLE bool parseAuthorizationCode(CloudDriveModel::ClientTypes type, QString text);

    // Service Proxy with Job Queuing.
    Q_INVOKABLE void requestToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void authorize(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void accessToken(CloudDriveModel::ClientTypes type, QString pin = "");
    Q_INVOKABLE void refreshToken(CloudDriveModel::ClientTypes type, QString uid, QString nextNonce = "");
    Q_INVOKABLE void accountInfo(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void quota(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, QString localFilePath, int modelIndex);
    Q_INVOKABLE void filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, int modelIndex);
    Q_INVOKABLE void metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex, bool forcePut = false, bool forceGet = false);
    Q_INVOKABLE void browse(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath);

    Q_INVOKABLE QStringList getLocalPathList(QString localParentPath);
    Q_INVOKABLE void syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, int modelIndex, bool forcePut = false, QStringList remotePathList = QStringList("*"));
    Q_INVOKABLE void syncFromLocal_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, int modelIndex, bool forcePut = false, bool isRootLocalPath = true, QStringList remotePathList = QStringList("*"));

    Q_INVOKABLE void createFolder(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remoteParentPath, QString newRemoteFolderName);
    // createFolder_Block expected to get created remote path as result. If folder already exists, return existing folder path.
    Q_INVOKABLE QString createFolder_Block(CloudDriveModel::ClientTypes type, QString uid, QString remoteParentPath, QString newRemoteFolderName);

    /*
     *NOTE
     *moveFile without newRemoteParentPath means rename.
     *moveFile with newRemoteParentPath and newRemoteFileName means move.
    */
    Q_INVOKABLE void moveFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    Q_INVOKABLE void copyFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    Q_INVOKABLE void deleteFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, bool suppressDeleteLocal = false);
    Q_INVOKABLE void delta(CloudDriveModel::ClientTypes type, QString uid);

    void filePutResume(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId = "", qint64 offset = 0);
    void fileGetResume(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFIleSize, QString localFilePath, qint64 offset = 0);

    Q_INVOKABLE void migrateFile(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName);
    Q_INVOKABLE void migrateFilePut(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 bytesTotal, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName);
    void migrateFilePut_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName);
    void migrateFilePutResume_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName);

    Q_INVOKABLE void disconnect(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath = "");
    Q_INVOKABLE void deleteLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath);

    Q_INVOKABLE QString thumbnail(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString format, QString size); // Used by DropboxClient and ImageViewPage.qml.
    Q_INVOKABLE void cacheImage(QString remoteFilePath, QString url, int w, int h, QString caller);
    Q_INVOKABLE QString media(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath);
    Q_INVOKABLE QString shareFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, bool synchronous = false);

    Q_INVOKABLE int getSortFlag(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath);
    Q_INVOKABLE void setSortFlag(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, int sortFlag);

    // Selection functions.
    Q_INVOKABLE bool isAnyItemChecked();
    Q_INVOKABLE bool areAllItemChecked();
    Q_INVOKABLE void markAll();
    Q_INVOKABLE void markAllFiles();
    Q_INVOKABLE void markAllFolders();
    Q_INVOKABLE void unmarkAll();
    Q_INVOKABLE void clearCachedImagesOnCurrentRemotePath(bool clearThumbnail = false, bool clearThumbnail128 = true, bool clearPreview = false);

    // Model functions.
    Q_INVOKABLE void refreshItems();
    Q_INVOKABLE int findIndexByRemotePath(QString remotePath);
    Q_INVOKABLE int findIndexByRemotePathName(QString remotePathName);
signals:
    void loadCloudDriveItemsFinished(QString nonce);
    void initializeDBStarted(QString nonce);
    void initializeDBFinished(QString nonce);
    void jobQueueStatusSignal(int runningJobCount, int jobQueueCount, int jobCount , int itemCount);
    void localChangedSignal(QString localPath);
    void refreshFolderCacheSignal(QString localPath);
    void jobEnqueuedSignal(QString nonce, QString localPath);
    void jobUpdatedSignal(QString nonce);
    void jobRemovedSignal(QString nonce);
    void refreshRequestSignal(QString nonce);
    void refreshItemAfterFileGetSignal(QString nonce, QString localPath);
    void moveToTrashRequestSignal(QString nonce, QString localPath);
    void cacheImageFinished(QString absoluteFilePath, int err, QString errMsg, QString caller);
    void logRequestSignal(QString nonce, QString logType, QString titleText, QString message, int autoCloseInterval);

    void requestTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplySignal(QString nonce, int err, QString errMsg, QString msg, qint64 normalBytes, qint64 sharedBytes, qint64 quotaBytes);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg, bool suppressRemoveJob);
    void browseReplySignal(QString nonce, int err, QString errMsg, QString msg, bool suppressRemoveJob);
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
    void createFolderReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg, QString url, int expires);
    void deltaReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void migrateFileReplySignal(QString nonce, int err, QString errMsg, QString msg, bool suppressRemoveJob);
    void migrateFilePutReplySignal(QString nonce, int err, QString errMsg, QString msg, bool errorOnTarget);

    void migrateStartedSignal(qint64 total);
    void migrateProgressSignal(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, qint64 count, qint64 total);
    void migrateFinishedSignal(qint64 count, qint64 total);

    // Scheduler.
    void schedulerTimeoutSignal();

    void sortFlagChanged();
    void remoteParentPathChanged();
    void rowCountChanged();
    void runningJobCountChanged();
    void queuedJobCountChanged();
    void jobCountChanged();
    void selectedIndexChanged();
public slots:
    void proceedNextJob();
    void dispatchJob(const QString jobId);
    void dispatchJob(CloudDriveJob job);

    void threadFinishedFilter();
//    void loadCloudDriveItemsFilter(QString nonce);
    void requestTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectFilter(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplyFilter(QString nonce, int err, QString errMsg, QString msg, qint64 normalBytes, qint64 sharedBytes, qint64 quotaBytes);

    void fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void browseReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void createFolderReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplyFilter(QString nonce, int err, QString errMsg, QString msg, QString url, int expires);
    void deltaReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void migrateFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void migrateFilePutReplyFilter(QString nonce, int err, QString errMsg, QString msg, bool errorOnTarget = false);
    void fileGetResumeReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void filePutResumeReplyFilter(QString nonce, int err, QString errMsg, QString msg);

    void uploadProgressFilter(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal);

    // Refresh request
    void refreshRequestFilter(QString nonce);

    // Scheduler
    void schedulerTimeoutFilter();

    // Cache image.
    void cacheImageFinishedFilter(QString absoluteFilePath, int err, QString errMsg, QString caller);
private:
    QMultiMap<QString, CloudDriveItem> *m_cloudDriveItems;
    QHash<QString, CloudDriveJob> *m_cloudDriveJobs;
    QQueue<QString> *m_jobQueue;
    int runningJobCount;
    bool m_isSuspended;
    bool m_isAborted;
    bool m_isPaused;

    QHash<QString, QThread*> *m_threadHash;
    QThreadPool m_browseThreadPool;
    QThreadPool m_cacheImageThreadPool;

    // System information.
    QSystemNetworkInfo m_networkInfo;
    QSystemBatteryInfo m_batteryInfo;

    // Job queue processor.
    QTimer m_jobQueueTimer;
    void initJobQueueTimer();

    QSqlDatabase m_db;
    QSqlQuery m_selectByPrimaryKeyPS;
    QSqlQuery m_selectByLocalPathPS;
    QSqlQuery m_selectByTypePS;
    QSqlQuery m_selectByTypeAndUidPS;
    QSqlQuery m_selectChildrenByPrimaryKeyPS;
    QSqlQuery m_insertPS;
    QSqlQuery m_updatePS;
    QSqlQuery m_updateHashByLocalPathPS;
    QSqlQuery m_deletePS;
    QSqlQuery m_countPS;
    QSqlQuery m_countByLocalPathPS;

    void initializeDB(QString nonce);
    void fixDamagedDB();
    void cleanDB();
    void closeDB();

    QList<CloudDriveItem> getItemListFromPS(QSqlQuery ps);
    CloudDriveItem selectItemByPrimaryKeyFromDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    QList<CloudDriveItem> selectItemsFromDB(CloudDriveModel::ClientTypes type, QString uid = "", QString localPath = "");
    QList<CloudDriveItem> selectItemsByLocalPathFromDB(QString localPath);
    QList<CloudDriveItem> selectItemsByTypeFromDB(CloudDriveModel::ClientTypes type);
    QList<CloudDriveItem> selectItemsByTypeAndUidFromDB(CloudDriveModel::ClientTypes type, QString uid);
    QList<CloudDriveItem> selectChildrenByPrimaryKeyFromDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    QList<CloudDriveItem> selectItemsByTypeAndUidAndRemotePathFromDB(CloudDriveModel::ClientTypes type, QString uid, QString remotePath);
    int insertItemToDB(const CloudDriveItem item, bool suppressMessages = false);
    int updateItemToDB(const CloudDriveItem item, bool suppressMessages = false);
    int updateItemHashByLocalPathToDB(const QString localPath, const QString hash);
    int deleteItemToDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    int deleteItemWithChildrenFromDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    int countItemDB();
    int countItemByLocalPathDB(const QString localPath);
    int countItemByTypeAndUidAndRemotePathFromDB(CloudDriveModel::ClientTypes type, QString uid, QString remotePath);
    QString getItemCacheKey(int type, QString uid, QString localPath);

    QMap<QString, CloudDriveItem> *m_itemCache;
    QMap<QString, bool> *m_isConnectedCache;
    QMap<QString, bool> *m_isDirtyCache;
    QMap<QString, bool> *m_isSyncingCache;
    QMap<QString, QString> *m_runningJobCache;

    CloudDriveClient *defaultClient;
    DropboxClient *dbClient;
    GCDClient *gcdClient;
    SkyDriveClient *skdClient;
    FtpClient *ftpClient;
    WebDavClient *webDavClient;
    BoxClient *boxClient;
    QString accessTokenPin;

    QMutex mutex;

    void loadCloudDriveItems(QString nonce);
    void saveCloudDriveItems();

    void loadCloudDriveJobs(QString nonce);
    void saveCloudDriveJobs();

    CloudDriveClient * getCloudClient(const int type);
    CloudDriveClient * getCloudClient(ClientTypes type);

    void initializeCloudClients(QString nonce);
    void connectCloudClientsSignal(CloudDriveClient *client);
    void initializeDropboxClient();
    void initializeSkyDriveClient();
    void initializeGoogleDriveClient();
    void initializeFtpClient();
    void initializeWebDAVClient();
    void initializeBoxClient();

    QString createNonce();
    void jobDone();

    // Scheduler.
    bool m_isSchedulerSuspended;
    QTimer m_schedulerTimer;
    void initScheduler();
    bool matchCronExp(QString cronExp, QString cronValue);

    QSettings m_settings;

    // Create temp path for storing temporary downloaded file during migration.
    void createTempPath();

    // List model and sorting.
    QList<CloudDriveModelItem> *m_modelItemList;
    QString m_remoteParentPath;
    QString m_remoteParentPathName;
    QString m_remoteParentParentPath;
    int m_selectedIndex;
    QString m_selectedRemotePath;
    void sortItemList(QList<CloudDriveModelItem> *itemList, int sortFlag);

    int compareMetadata(CloudDriveJob job, QScriptValue &jsonObj, QString localFilePath);
};

#endif // CLOUDDRIVEMODEL_H
