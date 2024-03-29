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

    explicit DropboxClient(QObject *parent = 0);
    ~DropboxClient();

    bool isRemoteAbsolutePath();
    bool isRemotePathCaseInsensitive();
    bool isFilePutResumable(qint64 fileSize);
    bool isFileGetResumable(qint64 fileSize);
    bool isDeltaSupported();
    bool isDeltaEnabled(QString uid);
    bool isViewable();
    bool isImageUrlCachable();

    QDateTime parseReplyDateString(QString dateString);

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin = "");
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    QNetworkReply * filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset);
    QString filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous = false);
    QString filePutResumeUpload(QString nonce, QString uid, QIODevice * source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    QString filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    QString filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous = false);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString delta(QString nonce, QString uid, bool synchronous = false);
    QString thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format = "jpeg", QString size = "s"); // format = {jpeg|png}, size = {xs|s|m|l|xl}
    QString media(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply *property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous = false, QString callback = "");
signals:

public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void quotaReplyFinished(QNetworkReply *reply);
    void metadataReplyFinished(QNetworkReply *reply);
    void browseReplyFinished(QNetworkReply *reply);
    QString createFolderReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    QString deleteFileReplyFinished(QNetworkReply *reply, bool synchronous = false);
    QString shareFileReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void filePutResumeReplyFinished(QNetworkReply *reply);
    QString deltaReplyFinished(QNetworkReply *reply);
    QString mediaReplyFinished(QNetworkReply *reply);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
    QString fileGetReplyResult(QNetworkReply *reply);
    QString filePutReplyResult(QNetworkReply *reply);
private:
    QString signatureMethod;
    QString chunkedUploadURI;
    QString commitChunkedUploadURI;

    bool isFullAccess;
    QString dropboxRoot;

    QString getDefaultLocalFilePath(const QString &remoteFilePath);
    QString getDefaultRemoteFilePath(const QString &localFilePath);
    QByteArray createBaseString(QString method, QString uri, QString queryString);
    QString createSignature(QString signatureMethod, QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QByteArray createSignatureWithHMACSHA1(QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QByteArray createSignatureWithPLAINTEXT(QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QByteArray createPostData(QString signature, QString queryString);
    QByteArray createOAuthHeaderString(QMap<QString, QString> sortMap);
    QByteArray createOAuthHeaderForUid(QString nonce, QString uid, QString method, QString uri, QMap<QString, QString> addParamMap = QMap<QString, QString>());
    QString getParentRemotePath(QString remotePath);
    QString getRemoteName(QString remotePath);
};

#endif // DropboxClient_H
