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
    explicit SkyDriveClient(QObject *parent = 0);
    ~SkyDriveClient();

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin);
    void refreshToken(QString nonce, QString uid);
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);

    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous = false);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);

    QNetworkReply * files(QString nonce, QString uid, QString remoteFilePath, int offset, bool synchronous = false, QString callback = "");
    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous = false, QString callback = "");

    bool isFileGetResumable(qint64 fileSize);
    bool isFileGetRedirected();
    bool isViewable();
    QDateTime parseReplyDateString(QString dateString);
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
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
    QString filePutReplyResult(QNetworkReply *reply);
private:
    QString logoutURI;

    void mergePropertyAndFilesJson(QString nonce, QString callback, QString uid);
    void renameFile(QString nonce, QString uid, QString remoteFilePath, QString newName);
};

#endif // SkyDriveClient_H
