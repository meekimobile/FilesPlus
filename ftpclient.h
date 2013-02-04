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
    static const QString FtpRoot;

    explicit FtpClient(QObject *parent = 0);
    ~FtpClient();

    bool testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname = "");
    void saveConnection(QString id, QString hostname, QString username, QString password, QString token);

    bool isRemoteAbsolutePath();
    bool isConfigurable();

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
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
signals:

public slots:
    void fileGetReplyFinished(QString nonce, bool error);
    void filePutReplyFinished(QString nonce, bool error);
    void deleteFileReplyFinished(QString nonce, int error, QString errorString);
protected:
    // TODO FTP should return common object directly.
//    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
private:
    QHash<QString, QFtpWrapper*> *m_ftpHash;
    QHash<QString, QFile*> m_localFileHash;

    QFtpWrapper *connectToHost(QString nonce, QString uid);
    QString property(QString nonce, QString uid, QString remoteFilePath);
    bool deleteRecursive(QFtpWrapper *m_ftp, QString remoteFilePath);

    QString getPropertyJson(const QString parentPath, const QUrlInfo item);
    QString getItemListJson(const QString parentPath, const QList<QUrlInfo> itemList);
    QString getParentRemotePath(QString remotePath);
    QString getRemoteFileName(QString remotePath);
};

#endif // FTPCLIENT_H
