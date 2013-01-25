#ifndef WEBDAVCLIENT_H
#define WEBDAVCLIENT_H

#include "clouddriveclient.h"
#include "sleeper.h"
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"

class WebDavClient : public CloudDriveClient
{
    Q_OBJECT
public:
    static const QString ConsumerKey;
    static const QString ConsumerSecret;
    static const QString WebDavRoot;

    static const QString loginURI;
    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString quotaURI;
    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString metadataURI;
    static const QString createFolderURI;
    static const QString moveFileURI;
    static const QString copyFileURI;
    static const QString deleteFileURI;
    static const QString sharesURI;

    explicit WebDavClient(QObject *parent = 0);
    ~WebDavClient();

    bool testConnection(QString id, QString hostname, QString username, QString password);
    void saveConnection(QString id, QString hostname, QString username, QString password);

    bool isRemoteAbsolutePath();

    void authorize(QString nonce);
    void accessToken(QString nonce, QString pin = "");
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);

    QString fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous = true);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    void createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);

    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    QString fileGetReplySave(QNetworkReply *reply);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
signals:
    void migrateFilePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void fileGetReplyFinished(QString nonce, bool error);
    void filePutReplyFinished(QString nonce, bool error);
    void deleteFileReplyFinished(QString nonce, int error, QString errorString);
private:
    QHash<QString, QFile*> m_localFileHash;

    QString property(QString nonce, QString uid, QString remoteFilePath);

    QString getParentRemotePath(QString remotePath);
    QString getRemoteFileName(QString remotePath);
};

#endif // WEBDAVCLIENT_H
