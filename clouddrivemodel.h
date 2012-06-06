#ifndef CLOUDDRIVEMODEL_H
#define CLOUDDRIVEMODEL_H

#include <QDeclarativeItem>
#include <QtNetwork>
#include <QHash>
#include <QMultiHash>
#include <QAbstractListModel>
#include "clouddriveitem.h"
#include "clouddrivejob.h"
#include "dropboxclient.h"
#include "gcdclient.h"

class CloudDriveModel : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS(ClientTypes)
public:
    static const QString HashFilePath;
    static const int MaxRunningJobCount;

    enum ClientTypes {
        Dropbox,
        GoogleDrive,
        SkyDrive,
        SugarSync
    };

    enum Operations {
        FileGet,
        FilePut,
        Metadata
    };

    explicit CloudDriveModel(QDeclarativeItem *parent = 0);
    ~CloudDriveModel();

    void addItem(QString localPath, CloudDriveItem item);
    QList<CloudDriveItem> getItemList(QString localPath);
    CloudDriveItem getItem(QString localPath, CloudDriveModel::ClientTypes type, QString uid);

    Q_INVOKABLE bool isConnected(QString localPath);
    Q_INVOKABLE bool isSyncing(QString localPath);
    Q_INVOKABLE QString getFirstJobJson(QString localPath);
    Q_INVOKABLE QString getJobJson(QString jobId);
    Q_INVOKABLE void addItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath, QString remotePath, QString hash);
    Q_INVOKABLE void removeItem(CloudDriveModel::ClientTypes type, QString uid, QString localPath);
    Q_INVOKABLE QString getItemHash(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemRemotePath(QString localPath, CloudDriveModel::ClientTypes type, QString uid);
    Q_INVOKABLE QString getItemListJson(QString localPath);
    Q_INVOKABLE QString getDefaultLocalFilePath(const QString &remoteFilePath);
    Q_INVOKABLE QString getDefaultRemoteFilePath(const QString &localFilePath);
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE bool isAuthorized(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE QStringList getStoredUidList(CloudDriveModel::ClientTypes type);

    Q_INVOKABLE void requestToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void authorize(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE bool parseAuthorizationCode(CloudDriveModel::ClientTypes type, QString text);
    Q_INVOKABLE void accessToken(CloudDriveModel::ClientTypes type);
    Q_INVOKABLE void accountInfo(CloudDriveModel::ClientTypes type, QString uid);

    Q_INVOKABLE void fileGet(CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath, int modelIndex);
    Q_INVOKABLE void filePut(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
    Q_INVOKABLE void metadata(CloudDriveModel::ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath, int modelIndex);
signals:
    void requestTokenReplySignal(int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString url, QString redirectFrom);
    void accessTokenReplySignal(int err, QString errMsg, QString msg);
    void accountInfoReplySignal(int err, QString errMsg, QString msg);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
public slots:
    void fileGetReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplyFilter(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgressFilter(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgressFilter(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
private:
    QMultiMap<QString, CloudDriveItem> m_cloudDriveItems;
    DropboxClient *dbClient;
    GCDClient *gcdClient;
    QHash<QString, CloudDriveJob> m_cloudDriveJobs;
    QQueue<QString> m_jobQueue;
    int runningJobCount;

    void loadCloudDriveItems();
    void saveCloudDriveItems();
    void initializeDropboxClient();
    void initializeGCDClient();
    QString createNonce();
    void jobDone();
    void proceedNextJob();
};

#endif // CLOUDDRIVEMODEL_H
