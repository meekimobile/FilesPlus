#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include <QApplication>
#include <QFtp>
#include "sleeper.h"
#include "tokenpair.h"

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

    void connectToHost(QString uid);
    void disconnectFromHost();

    bool testConnection(QString hostname, quint16 port, QString username, QString password);
    void saveConnection(QString id, QString hostname, quint16 port, QString username, QString password);

//    void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
//    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath);
//    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
//    QNetworkReply * files(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false, QString callback = "");
//    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false, QString callback = "");
//    QNetworkReply * createFolder(QString nonce, QString uid, QString newRemoteFolderName, QString remoteParentPath, bool synchronous = false);
//    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath);
//    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath);
//    QNetworkReply * deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
//    void shareFile(QString nonce, QString uid, QString remoteFilePath);
signals:

public slots:
    void commandStarted(int id);
    void commandFinished(int id, bool error);
    void addToList(const QUrlInfo &i);
    void updateDataTransferProgress(qint64 done, qint64 total);
    void stateChangedFilter(int state);
    void doneFilter(bool error);

private:
    QFtp *m_ftp;
    bool m_isDone;

    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;
    QString localPath;
    QHash<QString, QFile*> m_localFileHash;

    void loadAccessPairMap();
    void saveAccessPairMap();
};

#endif // FTPCLIENT_H
