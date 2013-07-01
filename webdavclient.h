#ifndef WEBDAVCLIENT_H
#define WEBDAVCLIENT_H

#include <QScriptEngine>
#include <QDomDocument>
#include "clouddriveclient.h"
#include "sleeper.h"
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"

class WebDavClient : public CloudDriveClient
{
    Q_OBJECT
public:
    static const QString ConsumerKey;
    static const QString ConsumerSecret;

    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString quotaURI;
    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString createFolderURI;
    static const QString moveFileURI;
    static const QString copyFileURI;
    static const QString deleteFileURI;
    static const QString sharesURI;
    static const QString propertyURI;
    static const QString renameFileURI;

    static const qint64 DefaultChunkSize;

    static const QString ReplyDateFormat;

    explicit WebDavClient(QObject *parent = 0);
    ~WebDavClient();

    bool testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname);
    bool saveConnection(QString id, QString hostname, QString username, QString password, QString token);

    QString getRemoteRoot(QString uid);
    bool isRemoteAbsolutePath();
    bool isConfigurable();
    bool isFileGetResumable(qint64 fileSize);
    qint64 getChunkSize();

    QDateTime parseReplyDateString(QString dateString);

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin = "");
    void accountInfo(QString nonce, QString uid); // TODO Still not work.
    void quota(QString nonce, QString uid);
    QString fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, bool synchronous = true);
    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);
    void filePut(QString nonce, QString uid, QString localFilePath, QString remoteParentPath, QString remoteFileName);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous = false);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    void shareFile(QString nonce, QString uid, QString remoteFilePath);
    QString media(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, QString requestBody = "", int depth = 0, bool synchronous = true, QString callback = "");
    QString fileGetReplySave(QNetworkReply *reply);
signals:

public slots:
    void sslErrorsReplyFilter(QNetworkReply *reply, QList<QSslError> sslErrors);
    void sslErrorsReplyFilter(QList<QSslError> sslErrors);

    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);

    void fileGetReplyFinished(QNetworkReply *reply);
    void filePutReplyFinished(QNetworkReply *reply);

    void propertyReplyFinished(QNetworkReply *reply); // Includes browse, metadata and quota reply handlers.

    QString createFolderReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    QString deleteFileReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void shareFileReplyFinished(QNetworkReply *reply);

    void fileGetResumeReplyFinished(QNetworkReply *reply);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
private:
    QHash<QString, QString> m_remoteRootHash;
    QHash<QString, QFile*> m_localFileHash;
    QHash<QString, QBuffer*> m_bufferHash;

    QByteArray createAuthHeader(QString uid);
    QScriptValue createScriptValue(QScriptEngine &engine, QDomNode &n, QString caller);
    QString createPropertyJson(QString replyBody, QString caller);
    QString createResponseJson(QString replyBody, QString caller);
    QString prepareRemotePath(QString uid, QString remoteFilePath);
    QString getHostname(QString email, bool hostOnly = false);

    void testSSLConnection(QString hostname);
    QSslConfiguration getSelfSignedSslConfiguration(QSslConfiguration sslConf);

    QString getParentRemotePath(QString remotePath);
    QString getRemoteFileName(QString remotePath);
};

#endif // WEBDAVCLIENT_H
