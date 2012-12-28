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

    bool testConnection(QString hostname, quint16 port, QString username, QString password);
    void saveConnection(QString id, QString hostname, quint16 port, QString username, QString password);

    void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    void createFolder(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);
signals:

public slots:

private:
    QHash<QString, QFtpWrapper*> *m_ftpHash;
    QHash<QString, QFile*> m_localFileHash;

    QFtpWrapper *connectToHost(QString nonce, QString uid);
    QString property(QString nonce, QString uid, QString remoteFilePath);
    bool deleteRecursive(QFtpWrapper *ftp, QString remoteFilePath);

    QString getPropertyJson(const QString parentPath, const QUrlInfo item);
    QString getItemListJson(const QString parentPath, const QList<QUrlInfo> itemList);
    QString getParentRemotePath(QString remotePath);
    QString getRemoteFileName(QString remotePath);
};

#endif // FTPCLIENT_H
