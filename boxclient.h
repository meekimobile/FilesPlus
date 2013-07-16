#ifndef BOXCLIENT_H
#define BOXCLIENT_H

#include "clouddriveclient.h"
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"
#include "sleeper.h"

class BoxClient : public CloudDriveClient
{
    Q_OBJECT
public:
    explicit BoxClient(QObject *parent = 0);
    ~BoxClient();

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin);
    void refreshToken(QString nonce, QString uid);
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);

    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous = false);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    QString shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString delta(QString nonce, QString uid, bool synchronous = false);
    QString thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format = "png", QString size = "64x64"); // format = {png}, size = {32x32,64x64,128x128,256x256}
    QString media(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply * files(QString nonce, QString uid, QString remoteFilePath, int offset, bool synchronous, QString callback);
    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous, QString callback);

    bool isDeltaSupported();
    bool isDeltaEnabled(QString uid);
    bool isViewable();
    bool isMediaEnabled(QString uid);
    bool isImageUrlCachable();
    bool isFileGetRedirected();
    QDateTime parseReplyDateString(QString dateString);
signals:

public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void quotaReplyFinished(QNetworkReply *reply);

    void propertyReplyFinished(QNetworkReply *reply);
    void filesReplyFinished(QNetworkReply *reply);

    QString createFolderReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    void deleteFileReplyFinished(QNetworkReply *reply);
    QString shareFileReplyFinished(QNetworkReply *reply, bool synchronous = false);
    QString deltaReplyFinished(QNetworkReply *reply);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
    QString filePutReplyResult(QNetworkReply *reply);
private:
    QString filePutRevURI;

    QHash<QString, bool> *m_isDirHash; // NOTE Use remoteFilePath as key.

    bool isRemoteDir(QString nonce, QString uid, QString remoteFilePath);
    QString searchFileId(QString nonce, QString uid, QString remoteParentPath, QString remoteFileName);
    void mergePropertyAndFilesJson(QString nonce, QString callback, QString uid);
    void renameFile(QString nonce, QString uid, QString remoteFilePath, QString newName);
    bool isCloudOnly(QScriptValue jsonObj);
};

#endif // BOXCLIENT_H
