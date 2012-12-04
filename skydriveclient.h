#ifndef SkyDriveClient_H
#define SkyDriveClient_H

#include <QDeclarativeItem>
#include <QtNetwork>
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"

class SkyDriveClient : public QObject
{
    Q_OBJECT
public:
    static const QString KeyStoreFilePath;
    static const QString consumerKey;
    static const QString consumerSecret;
    static const QString clientTypeName;

    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString quotaURI;
    static const QString logoutURI;

    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString metadataURI;
    static const QString propertyURI;
    static const QString createFolderURI;
    static const QString moveFileURI;
    static const QString copyFileURI;
    static const QString deleteFileURI;
    static const QString sharesURI;

    explicit SkyDriveClient(QDeclarativeItem *parent = 0);
    ~SkyDriveClient();

    QString createTimestamp();
    QString createNormalizedQueryString(QMap<QString, QString> sortMap);
    QString encodeURI(const QString uri);

    Q_INVOKABLE void authorize(QString nonce);
    Q_INVOKABLE void accessToken(QString nonce, QString pin);
    Q_INVOKABLE void refreshToken(QString nonce, QString uid);
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE QStringList getStoredUidList();
    Q_INVOKABLE int removeUid(QString uid);
    Q_INVOKABLE void accountInfo(QString nonce, QString uid);
    Q_INVOKABLE void quota(QString nonce, QString uid);

    Q_INVOKABLE void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    Q_INVOKABLE QString getDefaultLocalFilePath(const QString &remoteFilePath);
    Q_INVOKABLE void filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    Q_INVOKABLE QString getDefaultRemoteFilePath(const QString &localFilePath);
    Q_INVOKABLE void metadata(QString nonce, QString uid, QString remoteFilePath);
    Q_INVOKABLE void browse(QString nonce, QString uid, QString remoteFilePath);
    Q_INVOKABLE QString property(QString nonce, QString uid, QString remoteFilePath);
    Q_INVOKABLE void createFolder(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    Q_INVOKABLE void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath);
    Q_INVOKABLE void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath);
    Q_INVOKABLE void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    Q_INVOKABLE void shareFile(QString nonce, QString uid, QString remoteFilePath);
signals:
    void authorizeRedirectSignal(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplySignal(QString nonce, int err, QString errMsg, QString msg);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void browseReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);

    void createFolderReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void quotaReplyFinished(QNetworkReply *reply);

    void fileGetReplyFinished(QNetworkReply *reply);
    void filePutReplyFinished(QNetworkReply *reply);
    void metadataReplyFinished(QNetworkReply *reply);
    void browseReplyFinished(QNetworkReply *reply);

    void createFolderReplyFinished(QNetworkReply *reply);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    void deleteFileReplyFinished(QNetworkReply *reply);
    void shareFileReplyFinished(QNetworkReply *reply);
private:
    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;
    TokenPair requestTokenPair;
    QString localPath;
    QHash<QString, QFile*> m_localFileHash;
    QString refreshTokenUid;

    void loadAccessPairMap();
    void saveAccessPairMap();
};

#endif // SkyDriveClient_H
