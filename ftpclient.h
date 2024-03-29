#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include "clouddriveclient.h"
#include "sleeper.h"
#include "tokenpair.h"
#include "qftpwrapper.h"

class FtpClient : public CloudDriveClient
{
    Q_OBJECT
public:
    explicit FtpClient(QObject *parent = 0);
    ~FtpClient();

    bool testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname = "");
    bool saveConnection(QString id, QString hostname, QString username, QString password, QString token);

    QString getRemoteRoot(QString uid);
    bool isRemoteAbsolutePath();
    bool isConfigurable();
    bool isUnicodeSupported();

    void quota(QString nonce, QString uid);
    QString fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous = false);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous = false);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = true);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);

    bool abort(QString nonce);
signals:

public slots:
    QString fileGetReplyFinished(QString nonce, bool error, bool synchronous = false);
    void filePutReplyFinished(QString nonce, bool error, bool synchronous = false);
protected:

private:
    QHash<QString, QFtpWrapper*> *m_ftpHash;
    QHash<QString, QString> m_remoteRootHash; // NOTE Use UID as key.

    QFtpWrapper *connectToHost(QString nonce, QString uid);
    QString property(QString nonce, QString uid, QString remoteFilePath);
    bool deleteRecursive(QFtpWrapper *m_ftp, QString remoteFilePath);

    QString getPropertyJson(const QString parentPath, const QUrlInfo item);
    QString getRootPropertyJson();
    QString getItemListJson(const QString parentPath, const QList<QUrlInfo> itemList);
    QString getParentRemotePath(QString remotePath);
    QString getRemoteFileName(QString remotePath);
    QString mergePropertyAndFilesJson(QString propertyJsonText, QString filesJsonText);
};

#endif // FTPCLIENT_H
