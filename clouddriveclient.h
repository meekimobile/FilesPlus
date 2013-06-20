#ifndef CLOUDDRIVECLIENT_H
#define CLOUDDRIVECLIENT_H

#include <QObject>
#include <QtNetwork>
#include <QScriptEngine>
#include "tokenpair.h"
#include "contenttypehelper.h"

class CloudDriveClient : public QObject
{
    Q_OBJECT
public:
    static const QString KeyStoreFilePath;
    static const int FileWriteBufferSize;

    explicit CloudDriveClient(QObject *parent = 0);
    virtual ~CloudDriveClient();

    bool isAuthorized();
    QStringList getStoredUidList();
    QString getStoredUid(QString uid);
    QString getEmail(QString uid);
    int removeUid(QString uid);

    QString formatJSONDateString(QDateTime datetime);
    virtual QDateTime parseReplyDateString(QString dateString);

    virtual bool isRemoteAbsolutePath();
    virtual bool isRemotePathCaseInsensitive();
    virtual QString getRemoteRoot(QString id);
    virtual bool isFilePutResumable(qint64 fileSize = -1);
    virtual bool isFileGetResumable(qint64 fileSize = -1);
    virtual bool isDeltaSupported();
    virtual bool isDeltaEnabled(QString uid);
    virtual bool isConfigurable();
    virtual bool isViewable();
    virtual bool isImageUrlCachable();
    virtual bool isUnicodeSupported();

    virtual bool testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname);
    virtual bool saveConnection(QString id, QString hostname, QString username, QString password, QString token);

    virtual void requestToken(QString nonce);
    virtual void authorize(QString nonce, QString hostname);
    virtual void accessToken(QString nonce, QString pin);
    virtual void refreshToken(QString nonce, QString uid);
    virtual void accountInfo(QString nonce, QString uid);
    virtual void quota(QString nonce, QString uid);
    virtual QString fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous = false);
    virtual void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName);
    virtual void metadata(QString nonce, QString uid, QString remoteFilePath);
    virtual void browse(QString nonce, QString uid, QString remoteFilePath);
    virtual void createFolder(QString nonce, QString uid, QString newRemoteParentPath, QString newRemoteFolderName);
    virtual void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    virtual void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    virtual void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    virtual void shareFile(QString nonce, QString uid, QString remoteFilePath);
    virtual QString thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format, QString size);
    virtual QString media(QString nonce, QString uid, QString remoteFilePath);
    virtual QString delta(QString nonce, QString uid, bool synchronous = false);

    virtual QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous);

    virtual QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    /*
     *filePut
     *source must be seeked to required offset before invoking this method.
     *bytesTotal is total source file size.
     */
    virtual QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);

    virtual QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);
    virtual QNetworkReply * filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset);
    /*
     *filePutResumeStart
     *return json contains upload_id.
     */
    virtual QString filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous = false);
    /*
     *filePutResumeUpload
     *source must be seeked to required offset before invoking this method.
     *bytesTotal is total source file size.
     *offset is uploading offset.
     */
    virtual QString filePutResumeUpload(QString nonce, QString uid, QIODevice * source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    /*
     *filePutResumeStatus
     *return json contains next uploading offset.
     */
    virtual QString filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    virtual QString filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous = false);

    virtual bool abort(QString nonce);

    qint64 writeToFile(QIODevice *source, QString targetFilePath, qint64 offset);
signals:
    void requestTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString nonce, QString url, QString redirectFrom);
    void accessTokenReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void accountInfoReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void quotaReplySignal(QString nonce, int err, QString errMsg, QString msg, qint64 normalBytes, qint64 sharedBytes, qint64 quotaBytes);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void browseReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void createFolderReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void moveFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void copyFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void deleteFileReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void shareFileReplySignal(QString nonce, int err, QString errMsg, QString msg, QString url, int expires);
    void fileGetResumeReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutResumeReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutCommitReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void migrateFilePutReplySignal(QString nonce, int err, QString errMsg, QString msg); // Signal for supporting FTPClient.
    void deltaReplySignal(QString nonce, int err, QString errMsg, QString msg);

    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);

    void logRequestSignal(QString nonce, QString logType, QString titleText, QString message, int autoCloseInterval);
public slots:

protected:
    QString refreshTokenUid;
    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;
    QHash<QString, QNetworkReply*> *m_replyHash;

    QSettings m_settings;

    void loadAccessPairMap();
    void saveAccessPairMap();

    QString createNonce();
    QString createTimestamp();
    QString createNormalizedQueryString(QMap<QString, QString> sortMap);
    QString encodeURI(const QString uri);
    QString createQueryString(QMap<QString, QString> sortMap);
    QByteArray encodeMultiPart(QString boundary, QMap<QString, QString> paramMap, QString fileParameter, QString fileName, QByteArray fileData, QString contentType);
    QString removeDoubleSlash(QString remoteFilePath);
    QString getFileType(QString localPath);
    QString getContentType(QString fileName);

    virtual QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
    QString stringifyScriptValue(QScriptEngine &engine, QScriptValue &jsonObj);
    qint64 getOffsetFromRange(QString rangeHeader);
    QString getPathFromUrl(QString urlString);
private:

};

#endif // CLOUDDRIVECLIENT_H
