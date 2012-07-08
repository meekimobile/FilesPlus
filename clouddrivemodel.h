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
#include "clouddriveitem.h"
#include "clouddrivejob.h"
#include "clouddrivemodelthread.h"
#include "dropboxclient.h"
//#include "gcdclient.h"

class CloudDriveModel : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS(ClientTypes)
    Q_ENUMS(Operations)
    Q_PROPERTY(QString dirtyHash READ dirtyHash CONSTANT)
public:
    static const QString HashFilePath;
    static const int MaxRunningJobCount;
    static const QString DirtyHash;

    // ClientTypes is stored on m_cloudDriveItems. Its sequence shouldn't be changed.
    // AnyClient is technically never stored. It should be the last type.
    enum ClientTypes {
        Dropbox,
        GoogleDrive,
        SkyDrive,
        SugarSync
    };

    enum Operations {
        LoadCloudDriveItems,
        FileGet,
        FilePut,
        Metadata,
        CreateFolder,
        RequestToken,
        Authorize,
        AccessToken,
        AccountInfo,
        DeleteFile,
        MoveFile,
        CopyFile,
        ShareFile
    };

    explicit CloudDriveModel(QDeclarativeItem *parent = 0);
    ~CloudDriveModel();

    QString dirtyHash() const;

    void addItem(QString localPath, CloudDriveItem item);
    QList<CloudDriveItem> getItemList(QString localPath);
    CloudDriveItem getItem(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    QList<CloudDriveItem> findItemWithChildren(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    void cleanItems();
    bool cleanItem(const CloudDriveItem &item);

    // CloudDriveItem management.
    Q_INVOKABLE bool isConnected(QString localPath);
    Q_INVOKABLE bool isDirty(QString localPath, QDateTime lastModified);
    Q_INVOKABLE bool isSyncing(QString localPath);
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
    Q_INVOKABLE int getItemCount() const;
    Q_INVOKABLE QString getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemListJson(QString localPath);
    Q_INVOKABLE QString getDefaultLocalFilePath(const QString &remoteFilePath);
    Q_INVOKABLE QString getDefaultRemoteFilePath(const QString &localFilePath);
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE bool isAuthorized(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE QStringList getStoredUidList(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE int removeUid(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void requestJobQueueStatus();
    // Sync items.
    Q_INVOKABLE void syncItems();
    Q_INVOKABLE void syncFolder(const QString localFilePath);

    // Service Proxy with Job Queuing.
    Q_INVOKABLE void requestToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void authorize(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool parseAuthorizationCode(CloudDriveModel::ClientTypes type, QString text);
    Q_INVOKABLE void accessToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void accountInfo(CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE void fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath, int modelIndex);
    Q_INVOKABLE void filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
    Q_INVOKABLE void metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
    Q_INVOKABLE void syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex, bool forcePut = false);
    Q_INVOKABLE void createFolder(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex);
    Q_INVOKABLE void moveFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath);
    Q_INVOKABLE void copyFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, QString newLocalFilePath, QString newRemoteFilePath);
    Q_INVOKABLE void deleteFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath);
    Q_INVOKABLE void shareFile(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath);
signals:
    void loadCloudDriveItemsFinished(QString nonce);
    void proceedNextJobSignal();
    void jobQueueStatusSignal(int runningJobCount, int jobQueueCount, int itemCount);
    void localChangedSignal(QString localPath);

    void requestTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplySignal(QString nonce, int err, QString errMsg, QString msg);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
    void createFolderReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg, QString url, QString expires);
public slots:
    void proceedNextJob();

    void threadFinishedFilter();
    void loadCloudDriveItemsFilter(QString nonce);
    void requestTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectFilter(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgressFilter(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
    void createFolderReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplyFilter(QString nonce, int err, QString errMsg, QString msg);
private:
    QMultiMap<QString, CloudDriveItem> m_cloudDriveItems;
    QHash<QString, CloudDriveJob> m_cloudDriveJobs;
    QQueue<QString> m_jobQueue;
    int runningJobCount;

    DropboxClient *dbClient;
//    GCDClient *gcdClient;
    CloudDriveModelThread m_thread;

    QMutex mutex;

    void loadCloudDriveItems();
    void saveCloudDriveItems();
    void initializeDropboxClient();
    QString createNonce();
    void jobDone();
//    CloudDriveModelThread::ClientTypes mapToThreadClientTypes(CloudDriveModel::ClientTypes type);
//    CloudDriveModelThread::ClientTypes mapToThreadClientTypes(int type);
};

#endif // CLOUDDRIVEMODEL_H
