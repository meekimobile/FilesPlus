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
    explicit GCDClient(QObject *parent = 0);
    ~GCDClient();

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin);
    void refreshToken(QString nonce, QString uid);
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
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous = false);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString delta(QString nonce, QString uid, bool synchronous);
    QString media(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply * files(QString nonce, QString uid, QString remoteFilePath, int offset, bool synchronous, QString callback = "");
    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous = false, QString callback = "");

    void mergePropertyAndFilesJson(QString nonce, QString callback);
    QNetworkReply * patchFile(QString nonce, QString uid, QString remoteFilePath, QByteArray postData);
    QNetworkReply * filePutMulipart(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    QString searchFileId(QString nonce, QString uid, QString remoteParentPath, QString remoteFileName);

    QString getRemoteRoot(QString uid);
    bool isFilePutResumable(qint64 fileSize);
    bool isFileGetResumable(qint64 fileSize);
    bool isDeltaSupported();
    bool isDeltaEnabled(QString uid);
    bool isViewable();
    qint64 getChunkSize();
    int compareFileMetadata(CloudDriveJob &job, QScriptValue &jsonObj, QString localFilePath, CloudDriveItem &item);
signals:

public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void quotaReplyFinished(QNetworkReply *reply);

    void metadataReplyFinished(QNetworkReply *reply);
    void browseReplyFinished(QNetworkReply *reply);

    void propertyReplyFinished(QNetworkReply *reply);
    void filesReplyFinished(QNetworkReply *reply);

    QString createFolderReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    void deleteFileReplyFinished(QNetworkReply *reply);
    QString shareFileReplyFinished(QNetworkReply *reply, bool synchronous = false);

    void filePutMultipartReplyFinished(QNetworkReply *reply);
    void filePutResumeStartReplyFinished(QNetworkReply *reply);
    QString filePutResumeUploadReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void filePutResumeStatusReplyFinished(QNetworkReply *reply);

    QString deltaReplyFinished(QNetworkReply *reply);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
    QString fileGetReplyResult(QNetworkReply *reply);
    QString filePutReplyResult(QNetworkReply *reply);
private:
    QString authorizationScope;
    QString patchFileURI;
    QString trashFileURI;
    QString startResumableUploadURI;
    QString resumeUploadURI;

    QHash<QString, QString> m_sourceFileNameHash;
    QHash<QString, QDateTime> m_sourceFileTimestampHash;
    QHash<QString, QString> *m_downloadUrlHash;

    QMap<QString, QString> createMapFromJson(QString jsonText);
    QHash<QString, QString> createHashFromScriptValue(QString parentName, QScriptValue sc);
    QHash<QString, QString> createHashFromJson(QString jsonText);
    bool isCloudOnly(QScriptValue jsonObj);
};

#endif // GCDClient_H
