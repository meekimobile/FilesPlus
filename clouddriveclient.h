#ifndef CLOUDDRIVECLIENT_H
#define CLOUDDRIVECLIENT_H

#include <QObject>
#include <QtNetwork>
#include "tokenpair.h"

class CloudDriveClient : public QObject
{
    Q_OBJECT
public:
    static const QString KeyStoreFilePath;

    explicit CloudDriveClient(QObject *parent = 0);
    virtual ~CloudDriveClient();

    bool isAuthorized();
    QStringList getStoredUidList();
    int removeUid(QString uid);

    virtual bool isRemoteAbsolutePath();
    virtual QString getRemoteRoot();
    virtual bool isFilePutResumable(QString localFilePath);
    virtual bool isFileGetResumable(qint64 remoteFileSize);

    virtual bool testConnection(QString hostname, quint16 port, QString username, QString password);
    virtual void saveConnection(QString id, QString hostname, quint16 port, QString username, QString password);

    virtual void requestToken(QString nonce);
    virtual void authorize(QString nonce);
    virtual void accessToken(QString nonce, QString pin);
    virtual void refreshToken(QString nonce, QString uid);
    virtual void accountInfo(QString nonce, QString uid);
    virtual void quota(QString nonce, QString uid);
    virtual void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    virtual void filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    virtual void metadata(QString nonce, QString uid, QString remoteFilePath);
    virtual void browse(QString nonce, QString uid, QString remoteFilePath);
    virtual void createFolder(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFolderName);
    virtual void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    virtual void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    virtual void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    virtual void shareFile(QString nonce, QString uid, QString remoteFilePath);
    virtual QString delta(QString nonce, QString uid, bool synchronous = false);

    virtual QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous);
    virtual QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = 0);
    virtual QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, QString uploadId = "", qint64 offset = 0);

    virtual QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);
    virtual QNetworkReply * filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteFilePath, QString uploadId, qint64 offset);
    virtual QNetworkReply * filePutCommit(QString nonce, QString uid, QString localFilePath, QString remoteFilePath, QString uploadId);

    virtual QString thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format, QString size);
signals:
    void requestTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void browseReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void createFolderReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void deltaReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void fileGetResumeReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutResumeReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutCommitReplySignal(QString nonce, int err, QString errMsg, QString msg);

    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
public slots:

protected:
    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;

    void loadAccessPairMap();
    void saveAccessPairMap();
private:

};

#endif // CLOUDDRIVECLIENT_H
