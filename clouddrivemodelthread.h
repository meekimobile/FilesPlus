#ifndef CLOUDDRIVEMODELTHREAD_H
#define CLOUDDRIVEMODELTHREAD_H

#include <QThread>
#include <QMultiMap>
#include <QFile>
#include <QTime>
#include "clouddriveitem.h"
#include "dropboxclient.h"
#include "gcdclient.h"

class CloudDriveModelThread : public QThread
{
    Q_OBJECT
public:
    explicit CloudDriveModelThread(QObject *parent = 0);
    
    enum ClientTypes {
        Dropbox,
        GoogleDrive,
        SkyDrive,
        SugarSync
    };

    enum RunnableMethods {
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

    QMultiMap<QString, CloudDriveItem> getCloudDriveItems();

    bool isAuthorized();
    bool isAuthorized(ClientTypes type);
    QStringList getStoredUidList(ClientTypes type);

    QString getHashFilePath() const;
    void setHashFilePath(const QString hashFilePath);
    void setNonce(QString nonce);
    void setRunMethod(int method);
    void setClientType(ClientTypes clientType);
    void setUid(QString uid);
    void setLocalFilePath(QString localFilePath);
    void setRemoteFilePath(QString remoteFilePath);

    void run();
    void requestToken(ClientTypes type);
    void authorize(ClientTypes type);
    void accessToken(ClientTypes type);
    void accountInfo(ClientTypes type, QString uid);
    void fileGet(ClientTypes type, QString uid, QString remoteFilePath, QString localFilePath);
    void filePut(ClientTypes type, QString uid, QString localFilePath, QString remoteFilePath);
    void metadata(ClientTypes type, QString uid, QString remoteFilePath);
    void createFolder(ClientTypes type, QString uid, QString localPath, QString remotePath);
signals:
    void dataLoadedSignal();
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

private:
    QString m_hashFilePath;
    QMultiMap<QString, CloudDriveItem> m_cloudDriveItems;
    DropboxClient *dbClient;
    GCDClient *gcdClient;

    QString m_nonce;
    int m_runMethod;
    ClientTypes m_clientType;
    QString m_uid;
    QString m_localFilePath;
    QString m_remoteFilePath;

    void initializeDropboxClient();
    void initializeGCDClient();
    void loadCloudDriveItems();
};

#endif // CLOUDDRIVEMODELTHREAD_H
