#ifndef CLOUDDRIVEMODELTHREAD_H
#define CLOUDDRIVEMODELTHREAD_H

#include <QThread>
#include "clouddriveitem.h"
#include "clouddrivemodel.h"

class CloudDriveModelThread : public QRunnable
{
public:
    CloudDriveModelThread(QObject *parent = 0);

    void setNonce(QString nonce);
    void setDirectInvokation(bool flag);

    void run();

//    void migrateFile_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName);
//    void migrateFileResume_Block(QString nonce, CloudDriveModel::ClientTypes type, QString uid, QString remoteFilePath, qint64 remoteFileSize, CloudDriveModel::ClientTypes targetType, QString targetUid, QString targetRemoteParentPath, QString targetRemoteFileName);
signals:
    void requestTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplySignal(QString nonce, int err, QString errMsg, QString msg, qint64 normalBytes, qint64 sharedBytes, qint64 quotaBytes);

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
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg, QString url, int expires);
    void deltaReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void migrateFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void migrateFilePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
public slots:

private:
    QObject *m_parent;
    QString m_nonce;
    bool m_isDirectInvokation;

//    DropboxClient *dbClient;
//    GCDClient *gcdClient;
//    SkyDriveClient *skdClient;
//    FtpClient *ftpClient;
//    WebDavClient *webDavClient;

//    CloudDriveClient * getCloudClient(const int type);
//    CloudDriveClient * getCloudClient(ClientTypes type);

//    void initializeCloudClients(QString nonce);
//    void connectCloudClientsSignal(CloudDriveClient *client);
//    void initializeDropboxClient();
//    void initializeSkyDriveClient();
//    void initializeGoogleDriveClient();
//    void initializeFtpClient();
//    void initializeWebDAVClient();
};

#endif // CLOUDDRIVEMODELTHREAD_H
