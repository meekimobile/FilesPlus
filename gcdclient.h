#ifndef GCDClient_H
#define GCDClient_H

#include "clouddriveclient.h"
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"
#include "sleeper.h"
#include <QScriptEngine>

class GCDClient : public CloudDriveClient
{
    Q_OBJECT
public:
    static const QString consumerKey;
    static const QString consumerSecret;

    static const QString RemoteRoot;

    static const QString authorizationScope;

    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString quotaURI;

    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString filesURI;
    static const QString propertyURI;
    static const QString createFolderURI;
    static const QString copyFileURI;
    static const QString deleteFileURI;
    static const QString patchFileURI;
    static const QString sharesURI;
    static const QString trashFileURI;
    static const QString startResumableUploadURI;
    static const QString resumeUploadURI;
    static const QString deltaURI;

    static const qint64 DefaultChunkSize;

    explicit GCDClient(QObject *parent = 0);
    ~GCDClient();

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin);
    void refreshToken(QString nonce, QString uid);
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);
    QString fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous = false);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    void createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void deleteFile(QString nonce, QString uid, QString remoteFilePath);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply * files(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false, QString callback = "");
    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false, QString callback = "");
    void mergePropertyAndFilesJson(QString nonce, QString callback);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous);
    QNetworkReply * patchFile(QString nonce, QString uid, QString remoteFilePath, QByteArray postData);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    QString fileGetReplySave(QNetworkReply *reply);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    QNetworkReply * filePutMulipart(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);

    QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);
    QNetworkReply * filePutResume(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName, QString uploadId, qint64 offset);
    QString filePutResumeStart(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString remoteParentPath, bool synchronous = false);
    QString filePutResumeUpload(QString nonce, QString uid, QIODevice * source, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    QString filePutResumeStatus(QString nonce, QString uid, QString fileName, qint64 bytesTotal, QString uploadId, qint64 offset, bool synchronous = false);
    QString filePutCommit(QString nonce, QString uid, QString remoteFilePath, QString uploadId, bool synchronous = false);

    QString delta(QString nonce, QString uid, bool synchronous);
    QString media(QString nonce, QString uid, QString remoteFilePath);
    QString searchFileId(QString nonce, QString uid, QString remoteParentPath, QString remoteFileName);

    QString getRemoteRoot(QString uid);
    bool isFilePutResumable(qint64 fileSize);
    bool isFileGetResumable(qint64 fileSize);
    bool isDeltaSupported();
    bool isDeltaEnabled(QString uid);
    bool isViewable();
    qint64 getChunkSize();
signals:

public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void quotaReplyFinished(QNetworkReply *reply);

    void fileGetReplyFinished(QNetworkReply *reply);
    void filePutReplyFinished(QNetworkReply *reply);
    void filePutMultipartReplyFinished(QNetworkReply *reply);
    void metadataReplyFinished(QNetworkReply *reply);
    void browseReplyFinished(QNetworkReply *reply);

    void propertyReplyFinished(QNetworkReply *reply);
    void filesReplyFinished(QNetworkReply *reply);

    QString createFolderReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    void deleteFileReplyFinished(QNetworkReply *reply);
    void shareFileReplyFinished(QNetworkReply *reply);

    void fileGetResumeReplyFinished(QNetworkReply *reply);
    void filePutResumeStartReplyFinished(QNetworkReply *reply);
    void filePutResumeUploadReplyFinished(QNetworkReply *reply);
    void filePutResumeStatusReplyFinished(QNetworkReply *reply);

    QString deltaReplyFinished(QNetworkReply *reply);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
private:
    QHash<QString, QFile*> m_localFileHash;
    QHash<QString, QDateTime> m_sourceFileTimestampHash;
    QHash<QString, QBuffer*> m_bufferHash;
    QString refreshTokenUid;
    QHash<QString, QByteArray> *m_propertyReplyHash;
    QHash<QString, QByteArray> *m_filesReplyHash;
    QHash<QString, QString> *m_downloadUrlHash;

    QSettings m_settings;

    QString createTimestamp();
    QString createNormalizedQueryString(QMap<QString, QString> sortMap);
    QString encodeURI(const QString uri);
    QString createQueryString(QMap<QString, QString> sortMap);
    QMap<QString, QString> createMapFromJson(QString jsonText);
    QHash<QString, QString> createHashFromScriptValue(QString parentName, QScriptValue sc);
    QHash<QString, QString> createHashFromJson(QString jsonText);
    QByteArray encodeMultiPart(QString boundary, QMap<QString, QString> paramMap, QString fileParameter, QString fileName, QByteArray fileData, QString contentType);
    QString getRedirectedUrl(QString url);
    bool isCloudOnly(QScriptValue jsonObj);
};

#endif // GCDClient_H
