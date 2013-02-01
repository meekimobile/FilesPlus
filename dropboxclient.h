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
    static const QString deltaURI;
    static const QString chunkedUploadURI;
    static const QString commitChunkedUploadURI;

    static const qint64 ChunkSize;

    explicit DropboxClient(QObject *parent = 0, bool fullAccess = false);
    ~DropboxClient();

    QString getDefaultLocalFilePath(const QString &remoteFilePath);
    QString getDefaultRemoteFilePath(const QString &localFilePath);

    bool isRemoteAbsolutePath();
    bool isRemotePathCaseInsensitive();
    bool isFilePutResumable(qint64 fileSize);
    bool isFileGetResumable(qint64 fileSize);
    bool isDeltaSupported();
    bool isDeltaEnabled(QString uid);

    void requestToken(QString nonce);
    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin = "");
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);
    QString fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous = false);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    void createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);
    QString delta(QString nonce, QString uid, bool synchronous = false);

    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    QString fileGetReplySave(QNetworkReply *reply);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);

    QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);
    QNetworkReply * filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset);
    QString filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous = false);
    QString filePutResumeUpload(QString nonce, QString uid, QIODevice * source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    QString filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    QString filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous = false);

    QString thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format, QString size);
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
    void fileGetResumeReplyFinished(QNetworkReply *reply);
    void filePutResumeReplyFinished(QNetworkReply *reply);

    void deltaReplyFinished(QNetworkReply *reply);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
private:
    TokenPair requestTokenPair;
    QString localPath;
    QHash<QString, QFile*> m_localFileHash;

    bool isFullAccess;
    QString consumerKey;
    QString consumerSecret;
    QString dropboxRoot;

    QSettings m_settings;

    QString createTimestamp();
    QString createNonce();
    QString createNormalizedQueryString(QMap<QString, QString> sortMap);
    QString createQueryString(QMap<QString, QString> sortMap);
    QByteArray createBaseString(QString method, QString uri, QString queryString);
    QString createSignature(QString signatureMethod, QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QString createSignatureWithHMACSHA1(QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QString createSignatureWithPLAINTEXT(QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QByteArray createPostData(QString signature, QString queryString);
    QByteArray createOAuthHeaderString(QMap<QString, QString> sortMap);
    QByteArray createOAuthHeaderForUid(QString nonce, QString uid, QString method, QString uri, QMap<QString, QString> addParamMap = QMap<QString, QString>());
    QString encodeURI(const QString uri);
    QString getParentRemotePath(QString remotePath);
    QString getRemoteName(QString remotePath);
};

#endif // DropboxClient_H
