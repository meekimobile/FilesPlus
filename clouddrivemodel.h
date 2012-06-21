#ifndef CLOUDDRIVEMODEL_H
#define CLOUDDRIVEMODEL_H

#include <QDeclarativeItem>
#include <QtNetwork>
#include <QHash>
#include <QMultiHash>
#include <QThread>
#include <QAbstractListModel>
#include "clouddriveitem.h"
#include "clouddrivejob.h"
#include "clouddrivemodelthread.h"
#include "dropboxclient.h"
#include "gcdclient.h"

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

    enum ClientTypes {
        AnyClient,
        Dropbox,
        GoogleDrive,
        SkyDrive,
        SugarSync
    };

    enum Operations {
        FileGet,
        FilePut,
        Metadata,
        CreateFolder,
        RequestToken,
        Authorize,
        AccessToken,
        AccountInfo,
        LoadCloudDriveItems
    };

    explicit CloudDriveModel(QDeclarativeItem *parent = 0);
    ~CloudDriveModel();

    QString dirtyHash() const;

    void addItem(QString localPath, CloudDriveItem item);
    QList<CloudDriveItem> getItemList(QString localPath);
    CloudDriveItem getItem(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    QList<CloudDriveItem> findItemList(QString pattern);
    void cleanItems();
    bool cleanItem(const CloudDriveItem &item);

    // CloudDriveItem management.
    Q_INVOKABLE bool isConnected(QString localPath);
    Q_INVOKABLE bool isDirty(QString localPath, QDateTime lastModified);
    Q_INVOKABLE bool isSyncing(QString localPath);
    Q_INVOKABLE QString getFirstJobJson(QString localPath);
    Q_INVOKABLE QString getJobJson(QString jobId);
    Q_INVOKABLE void removeJob(QString nonce);
    Q_INVOKABLE void addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash, bool addOnly = false);
    Q_INVOKABLE void removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE void updateItems(CloudDriveModel::ClientTypes type, QString localPath, QString hash);
    Q_INVOKABLE QString getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemListJson(QString localPath);
    Q_INVOKABLE QString getDefaultLocalFilePath(const QString &remoteFilePath);
    Q_INVOKABLE QString getDefaultRemoteFilePath(const QString &localFilePath);
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE bool isAuthorized(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE QStringList getStoredUidList(CloudDriveModel::ClientTypes type);

    // Sync items.
    Q_INVOKABLE void syncItems();
    Q_INVOKABLE void syncFolder(const QString localFilePath);

    // Service Proxy
    Q_INVOKABLE void requestToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void authorize(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool parseAuthorizationCode(CloudDriveModel::ClientTypes type, QString text);
    Q_INVOKABLE void accessToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void accountInfo(CloudDriveModel::ClientTypes type, QString uid);

    // Service Proxy with Job Queuing.
    Q_INVOKABLE void fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath, int modelIndex);
    Q_INVOKABLE void filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
    Q_INVOKABLE void metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
    Q_INVOKABLE void syncFromLocal(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex, bool forcePut = false);
    Q_INVOKABLE void createFolder(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, int modelIndex);
signals:
    void dataLoadedSignal();
    void proceedNextJobSignal();
    void localChangedSignal(QString localPath);

    void requestTokenReplySignal(int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString url, QString redirectFrom);
    void accessTokenReplySignal(int err, QString errMsg, QString msg);
    void accountInfoReplySignal(int err, QString errMsg, QString msg);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void createFolderReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
public slots:
    void dataLoadedFilter();
    void proceedNextJob();

    void fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void createFolderReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgressFilter(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
private:
    QMultiMap<QString, CloudDriveItem> m_cloudDriveItems;
    QHash<QString, CloudDriveJob> m_cloudDriveJobs;
    QQueue<QString> m_jobQueue;
    int runningJobCount;

    CloudDriveModelThread m_thread;

    QMutex m_mutex;

    void loadCloudDriveItems();
    void saveCloudDriveItems();
    QString createNonce();
    void jobDone();
    CloudDriveModelThread::ClientTypes mapToThreadClientTypes(CloudDriveModel::ClientTypes type);
    CloudDriveModelThread::ClientTypes mapToThreadClientTypes(int type);
};

#endif // CLOUDDRIVEMODEL_H
