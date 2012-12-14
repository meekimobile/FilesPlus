#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QApplication>
#include <QtNetwork>
#include <QList>
#include "sleeper.h"
#include "tokenpair.h"
#include "qftpwrapper.h"

class FtpClient : public QObject
{
    Q_OBJECT
public:
    static const QString KeyStoreFilePath;
    static const QString consumerKey;
    static const QString consumerSecret;
    static const QString clientTypeName;
    static const QString FtpRoot;

    explicit FtpClient(QObject *parent = 0);
    ~FtpClient();

    bool isAuthorized();
    QStringList getStoredUidList();
    int removeUid(QString uid);

    bool testConnection(QString hostname, quint16 port, QString username, QString password);
    void saveConnection(QString id, QString hostname, quint16 port, QString username, QString password);

    void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    void createFolder(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
//    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath);
//    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath);
    void deleteFile(QString nonce, QString uid, QString remoteFilePath);
//    void shareFile(QString nonce, QString uid, QString remoteFilePath);
signals:
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
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg);

public slots:

private:
    QHash<QString, QFtpWrapper*> *m_ftpHash;

    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;
    QHash<QString, QFile*> m_localFileHash;

    QFtpWrapper *connectToHost(QString nonce, QString uid);
    QString property(QString nonce, QString uid, QString remoteFilePath);
    bool deleteRecursive(QFtpWrapper *ftp, QString remoteFilePath);

    QString getPropertyJson(const QString parentPath, const QUrlInfo item);
    QString getItemListJson(const QString parentPath, const QList<QUrlInfo> itemList);
    QString getParentRemotePath(QString remotePath);
    QString getRemoteFileName(QString remotePath);

    void loadAccessPairMap();
    void saveAccessPairMap();
};

#endif // FTPCLIENT_H
