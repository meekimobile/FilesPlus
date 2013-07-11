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
    explicit WebDavClient(QObject *parent = 0);
    ~WebDavClient();

    bool testConnection(QString id, QString hostname, QString username, QString password, QString token, QString authHostname);
    bool saveConnection(QString id, QString hostname, QString username, QString password, QString token);

    bool isRemoteAbsolutePath();
    bool isConfigurable();
    bool isFileGetResumable(qint64 fileSize);
    QString getRemoteRoot(QString uid);
    QDateTime parseReplyDateString(QString dateString);

    void authorize(QString nonce, QString hostname);
    void accessToken(QString nonce, QString pin = "");
    void accountInfo(QString nonce, QString uid); // TODO Still not work.
    void quota(QString nonce, QString uid);

    QIODevice * fileGet(QString nonce, QString uid, QString remoteFilePath, qint64 offset = -1, bool synchronous = false);
    QIODevice * fileGetResume(QString nonce, QString uid, QString remoteFilePath, QString localFilePath, qint64 offset);
    QNetworkReply * filePut(QString nonce, QString uid, QIODevice * source, qint64 bytesTotal, QString remoteParentPath, QString remoteFileName, bool synchronous = false);
    void metadata(QString nonce, QString uid, QString remoteFilePath);
    void browse(QString nonce, QString uid, QString remoteFilePath);
    QString createFolder(QString nonce, QString uid, QString remoteParentPath, QString newRemoteFolderName, bool synchronous = false);
    void moveFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    void copyFile(QString nonce, QString uid, QString remoteFilePath, QString newRemoteParentPath, QString newRemoteFileName);
    QString deleteFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString shareFile(QString nonce, QString uid, QString remoteFilePath, bool synchronous = false);
    QString media(QString nonce, QString uid, QString remoteFilePath);

    QNetworkReply * property(QString nonce, QString uid, QString remoteFilePath, QString requestBody = "", int depth = 0, bool synchronous = true, QString callback = "");
signals:

public slots:
    void sslErrorsReplyFilter(QNetworkReply *reply, QList<QSslError> sslErrors);
    void sslErrorsReplyFilter(QList<QSslError> sslErrors);

    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);

    void propertyReplyFinished(QNetworkReply *reply); // Includes browse, metadata and quota reply handlers.

    QString createFolderReplyFinished(QNetworkReply *reply, bool synchronous = false);
    void moveFileReplyFinished(QNetworkReply *reply);
    void copyFileReplyFinished(QNetworkReply *reply);
    QString deleteFileReplyFinished(QNetworkReply *reply, bool synchronous = false);
    QString shareFileReplyFinished(QNetworkReply *reply, bool synchronous = false);
protected:
    QScriptValue parseCommonPropertyScriptValue(QScriptEngine &engine, QScriptValue jsonObj);
    QString fileGetReplyResult(QNetworkReply *reply);
    QString filePutReplyResult(QNetworkReply *reply);
private:
    QHash<QString, QString> m_remoteRootHash;

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
