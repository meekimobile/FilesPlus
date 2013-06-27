#ifndef BOXCLIENT_H
#define BOXCLIENT_H

#include "clouddriveclient.h"
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"
#include "sleeper.h"
#include <QBool>

class BoxClient : public CloudDriveClient
{
    Q_OBJECT
public:
    static const QString consumerKey;
    static const QString consumerSecret;

    static const QString RemoteRoot;

    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString quotaURI;

    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString filePutRevURI;
    static const QString filesURI;
    static const QString propertyURI;
    static const QString createFolderURI;
    static const QString copyFileURI;
    static const QString deleteFileURI;
    static const QString sharesURI;
    static const QString patchURI;
    static const QString thumbnailURI;

    static const qint64 DefaultChunkSize;

    explicit BoxClient(QObject *parent = 0);
    ~BoxClient();

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin);
    void refreshToken(QString nonce, QString uid);
    void accountInfo(QString nonce, QString uid);
    void quota(QString nonce, QString uid);
    QString fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous = false);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName);
    void metadata(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous = false);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString targetRemoteParentPath, QString newRemoteFileName);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);
//    QString delta(QString nonce, QString uid, bool synchronous = false);
    QString thumbnail(QString nonce, QString uid, QString remoteFilePath, QString format = "jpeg", QString size = "s"); // format = {jpeg|png}, size = {xs|s|m|l|xl}
    QString media(QString nonce, QString uid, QString remoteFilePath);

    bool isRemoteDir(QString nonce, QString uid, QString remoteFilePath);
    QNetworkReply * files(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false, QString callback = "");
    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, bool isDir, bool synchronous = false, QString callback = "");
    void mergePropertyAndFilesJson(QString nonce, QString callback, QString uid);
    void renameFile(QString nonce, QString uid, QString remoteFilePath, QString newName);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    QString fileGetReplySave(QNetworkReply *reply);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);

    QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);

    QString getRemoteRoot(QString uid);
    bool isFileGetResumable(qint64 fileSize);
    bool isViewable();
    bool isMediaEnabled(QString uid);
    bool isImageUrlCachable();
    qint64 getChunkSize();
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

    QString createFolderReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    void deleteFileReplyFinished(QNetworkReply *reply);
    void shareFileReplyFinished(QNetworkReply *reply);

    void fileGetResumeReplyFinished(QNetworkReply *reply);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
private:
    QString localPath;
    QHash<QString, QFile*> m_localFileHash;
    QHash<QString, QBuffer*> m_bufferHash;
    QString refreshTokenUid;
    QHash<QString, QByteArray> *m_propertyReplyHash;
    QHash<QString, QByteArray> *m_filesReplyHash;
    QHash<QString, bool> *m_isDirHash;
};

#endif // BOXCLIENT_H
