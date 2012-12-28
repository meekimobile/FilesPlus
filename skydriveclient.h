#ifndef SkyDriveClient_H
#define SkyDriveClient_H

#include "clouddriveclient.h"
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"
#include "sleeper.h"

class SkyDriveClient : public CloudDriveClient
{
    Q_OBJECT
public:
    static const QString consumerKey;
    static const QString consumerSecret;
    static const QString SkyDriveRoot;

    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString quotaURI;
    static const QString logoutURI;

    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString filesURI;
    static const QString propertyURI;
    static const QString createFolderURI;
    static const QString moveFileURI;
    static const QString copyFileURI;
    static const QString deleteFileURI;
    static const QString renameFileURI;
    static const QString sharesURI;

    explicit SkyDriveClient(QObject *parent = 0);
    ~SkyDriveClient();

    void authorize(QString nonce);
    void accessToken(QString nonce, QString pin);
    void refreshToken(QString nonce, QString uid);
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);
    void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    void createFolder(QString nonce, QString uid, QString newRemoteFolderName, QString remoteParentPath);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply * files(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false, QString callback = "");
    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false, QString callback = "");
    QNetworkReply * createFolder(QString nonce, QString uid, QString newRemoteFolderName, QString remoteParentPath, bool synchronous = false);
    QNetworkReply * deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    void renameFile(QString nonce, QString uid, QString remoteFilePath, QString newName);
signals:

public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void quotaReplyFinished(QNetworkReply *reply);

    void fileGetReplyFinished(QNetworkReply *reply);
    void filePutReplyFinished(QNetworkReply *reply);
    void metadataReplyFinished(QNetworkReply *reply);
    void browseReplyFinished(QNetworkReply *reply);

    void propertyReplyFinished(QNetworkReply *reply);
    void filesReplyFinished(QNetworkReply *reply);

    void createFolderReplyFinished(QNetworkReply *reply);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    void deleteFileReplyFinished(QNetworkReply *reply);
    void shareFileReplyFinished(QNetworkReply *reply);
private:
    QString localPath;
    QHash<QString, QFile*> m_localFileHash;
    QHash<QString, QBuffer*> m_bufferHash;
    QString refreshTokenUid;
    QHash<QString, QByteArray> *m_propertyReplyHash;
    QHash<QString, QByteArray> *m_filesReplyHash;

    QString createTimestamp();
    QString createNormalizedQueryString(QMap<QString, QString> sortMap);
    QString encodeURI(const QString uri);
};

#endif // SkyDriveClient_H
