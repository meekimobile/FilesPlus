#ifndef DropboxClient_H
#define DropboxClient_H

#include "clouddriveclient.h"
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"

class DropboxClient : public CloudDriveClient
{
    Q_OBJECT
public:
    static const QString DropboxConsumerKey;
    static const QString DropboxConsumerSecret;
    static const QString DropboxRoot;
    static const QString SandboxConsumerKey;
    static const QString SandboxConsumerSecret;
    static const QString SandboxRoot;

    static const QString signatureMethod;

    static const QString requestTokenURI;
    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString metadataURI;
    static const QString createFolderURI;
    static const QString moveFileURI;
    static const QString copyFileURI;
    static const QString deleteFileURI;
    static const QString sharesURI;
    static const QString mediaURI;
    static const QString thumbnailURI;

    explicit DropboxClient(QObject *parent = 0, bool fullAccess = false);
    ~DropboxClient();

    QString getDefaultLocalFilePath(const QString &remoteFilePath);
    QString getDefaultRemoteFilePath(const QString &localFilePath);

    bool isRemoteAbsolutePath();

    void requestToken(QString nonce);
    void authorize(QString nonce);
    void accessToken(QString nonce, QString pin = "");
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);
    void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    void createFolder(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFolderName);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteFilePath, QString newRemoteFileName);
    void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply * createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, QString remoteParentPath, QString remoteFileName);
signals:

public slots:
    void requestTokenReplyFinished(QNetworkReply *reply);
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
    TokenPair requestTokenPair;
    QString localPath;
    QHash<QString, QFile*> m_localFileHash;

    bool isFullAccess;
    QString consumerKey;
    QString consumerSecret;
    QString dropboxRoot;

    QString createTimestamp();
    QString createNonce();
    QString createNormalizedQueryString(QMap<QString, QString> sortMap);
    QByteArray createBaseString(QString method, QString uri, QString queryString);
    QString createSignature(QString signatureMethod, QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QString createSignatureWithHMACSHA1(QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QString createSignatureWithPLAINTEXT(QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QByteArray createPostData(QString signature, QString queryString);
    QByteArray createOAuthHeaderString(QMap<QString, QString> sortMap);
    QByteArray createOAuthHeaderForUid(QString nonce, QString uid, QString method, QString uri, QMap<QString, QString> addParamMap = QMap<QString, QString>());
    QString encodeURI(const QString uri);
    QString getParentRemotePath(QString remotePath);
};

#endif // DropboxClient_H
