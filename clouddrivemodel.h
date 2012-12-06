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
#include "clouddriveitem.h"
#include "clouddrivejob.h"
#include "clouddrivemodelthread.h"
#include "dropboxclient.h"
//#include "gcdclient.h"
#include "skydriveclient.h"

class CloudDriveModel : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS(ClientTypes)
    Q_ENUMS(Operations)
    Q_PROPERTY(QString dirtyHash READ dirtyHash CONSTANT)
    Q_PROPERTY(bool dropboxFullAccess READ getDropboxFullAccess WRITE setDropboxFullAccess)
public:
    static const QString ITEM_DAT_PATH;
    static const QString ITEM_DB_PATH;
    static const QString ITEM_DB_CONNECTION_NAME;
    static const int MaxRunningJobCount;
    static const QString DirtyHash;
    static const QStringList restrictFileTypes;

    // ClientTypes is stored on m_cloudDriveItems. Its sequence shouldn't be changed.
    // AnyClient is technically never stored. It should be the last type.
    enum ClientTypes {
        Dropbox,
        GoogleDrive,
        SkyDrive
    };

    enum Operations {
        LoadCloudDriveItems,
        InitializeDB,
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
        ScheduleSync
    };

    explicit CloudDriveModel(QDeclarativeItem *parent = 0);
    ~CloudDriveModel();

    QString dirtyHash() const;
    bool getDropboxFullAccess();
    void setDropboxFullAccess(bool flag);

    void addItem(QString localPath, CloudDriveItem item);
    QList<CloudDriveItem> getItemList(QString localPath);
    CloudDriveItem getItem(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    QList<CloudDriveItem> findItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    QList<CloudDriveItem> findItems(CloudDriveModel::ClientTypes type, QString uid);
    void cleanItems();
    bool cleanItem(const CloudDriveItem &item);

    // CloudDriveItem management.
    Q_INVOKABLE bool isConnected(QString localPath);
    Q_INVOKABLE bool isConnected(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE bool isRemotePathConnected(CloudDriveModel::ClientTypes type, QString uid, QString remotePath);
    Q_INVOKABLE bool isDirty(QString localPath, QDateTime lastModified);
    Q_INVOKABLE bool isSyncing(QString localPath);
    Q_INVOKABLE bool isParentConnected(QString localPath);
    Q_INVOKABLE bool canSync(QString localPath);
    Q_INVOKABLE QString getFirstJobJson(QString localPath);
    Q_INVOKABLE QString getJobJson(QString jobId);
    Q_INVOKABLE void removeJob(QString nonce);
    Q_INVOKABLE int getQueuedJobCount() const;
    Q_INVOKABLE void cancelQueuedJobs();
    Q_INVOKABLE void addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash, bool addOnly = false);
    Q_INVOKABLE void removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE void removeItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE void removeItems(QString localPath);
    Q_INVOKABLE void updateItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString hash);
    Q_INVOKABLE void updateItems(QString localPath, QString hash);
    Q_INVOKABLE void updateItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString newLocalPath, QString newRemotePath, QString newChildrenHash = "", QString newHash = "");
    Q_INVOKABLE int getItemCount();
    Q_INVOKABLE QString getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemListJson(QString localPath);
    Q_INVOKABLE QString getItemJson(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getDefaultLocalFilePath(const QString &remoteFilePath); // TODO Avoid using.
    Q_INVOKABLE QString getDefaultRemoteFilePath(const QString &localFilePath); // TODO Avoid using.
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE bool isAuthorized(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE QStringList getStoredUidList();
    Q_INVOKABLE QStringList getStoredUidList(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE int removeUid(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void requestJobQueueStatus();
    Q_INVOKABLE void suspendNextJob();
    Q_INVOKABLE void resumeNextJob();

    // Scheduler.
    Q_INVOKABLE int updateItemCronExp(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString cronExp);
    Q_INVOKABLE QString getItemCronExp(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE void loadScheduledItems(QString cronValue);
    Q_INVOKABLE void syncScheduledItems();

    // Sync items.
    Q_INVOKABLE void syncItems();
    Q_INVOKABLE void syncItems(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void syncItem(const QString localFilePath);
    Q_INVOKABLE void syncItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath);

    // Migrate DAT to DB.
    Q_INVOKABLE int getCloudDriveItemsCount();
    Q_INVOKABLE void migrateCloudDriveItemsToDB();

    // Dropbox specific functions.
    Q_INVOKABLE bool updateDropboxPrefix(bool fullAccess);

    // Service Proxy with Job Queuing.
    Q_INVOKABLE void requestToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void authorize(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool parseAuthorizationCode(CloudDriveModel::ClientTypes type, QString text);
    Q_INVOKABLE void accessToken(CloudDriveModel::ClientTypes type, QString pin = "");
    Q_INVOKABLE void refreshToken(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void accountInfo(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void quota(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath, int modelIndex);
    Q_INVOKABLE void filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
    Q_INVOKABLE void metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
    Q_INVOKABLE void browse(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath);
    Q_INVOKABLE void syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex, bool forcePut = false);
    Q_INVOKABLE void syncFromLocal_SkyDrive(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex, bool forcePut = false);

    Q_INVOKABLE void createFolder(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex);
    Q_INVOKABLE void moveFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath);
    Q_INVOKABLE void copyFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath);
    Q_INVOKABLE void deleteFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath);
    Q_INVOKABLE void shareFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath);
signals:
    void loadCloudDriveItemsFinished(QString nonce);
    void initializeDBStarted(QString nonce);
    void initializeDBFinished(QString nonce);
    void proceedNextJobSignal();
    void jobQueueStatusSignal(int runningJobCount, int jobQueueCount, int itemCount);
    void localChangedSignal(QString localPath);
    void jobEnqueuedSignal(QString nonce, QString localPath);
    void refreshRequestSignal();

    void requestTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplySignal(QString nonce, int err, QString errMsg, QString msg);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void browseReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
    void createFolderReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg, QString url, QString expires);

    void migrateStartedSignal(qint64 total);
    void migrateProgressSignal(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, qint64 count, qint64 total);
    void migrateFinishedSignal(qint64 count, qint64 total);

    // Scheduler.
    void schedulerTimeoutSignal();
public slots:
    // Job dispatcher
    void proceedNextJob();

    void threadFinishedFilter();
    void loadCloudDriveItemsFilter(QString nonce);
    void requestTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectFilter(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void browseReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgressFilter(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
    void createFolderReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);

    // Scheduler
    void schedulerTimeoutFilter();
private:
    QMultiMap<QString, CloudDriveItem> *m_cloudDriveItems;
    QHash<QString, CloudDriveJob> *m_cloudDriveJobs;
    QQueue<QString> *m_jobQueue;
    QQueue<CloudDriveItem> *m_scheduledItems;
    int runningJobCount;
    bool m_isSuspended;

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
    int insertItemToDB(const CloudDriveItem item);
    int updateItemToDB(const CloudDriveItem item);
    int updateItemHashByLocalPathToDB(const QString localPath, const QString hash);
    int deleteItemToDB(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    int countItemDB();
    int countItemByLocalPathDB(const QString localPath);
    int countItemByTypeAndUidAndRemotePathFromDB(CloudDriveModel::ClientTypes type, QString uid, QString remotePath);
    QString getItemCacheKey(int type, QString uid, QString localPath);

    QHash<QString, CloudDriveItem> *m_itemCache;
    QHash<QString, bool> *m_isConnectedCache;
    QHash<QString, bool> *m_isDirtyCache;
    QHash<QString, bool> *m_isSyncingCache;

    DropboxClient *dbClient;
//    GCDClient *gcdClient;
    SkyDriveClient *skdClient;
    QString accessTokenPin;
    CloudDriveModelThread m_thread;

    QMutex mutex;

    void saveCloudDriveItems();
    void initializeDropboxClient();
    void initializeSkyDriveClient();
    QString createNonce();
    void jobDone();
    QString getFileType(QString localPath);
    QString getParentLocalPath(QString localPath);
//    CloudDriveModelThread::ClientTypes mapToThreadClientTypes(CloudDriveModel::ClientTypes type);
//    CloudDriveModelThread::ClientTypes mapToThreadClientTypes(int type);

    bool m_dropboxFullAccess;

    // Scheduler.
    QTimer m_schedulerTimer;
    void initScheduler();
    bool matchCronExp(QString cronExp, QString cronValue);
};

#endif // CLOUDDRIVEMODEL_H
