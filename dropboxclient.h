#ifndef DropboxClient_H
#define DropboxClient_H

#include <QDeclarativeItem>
#include <QtNetwork>
#include "tokenpair.h"
#include "qnetworkreplywrapper.h"

class DropboxClient : public QDeclarativeItem
{
    Q_OBJECT
public:
    static const QString KeyStoreFilePath;
    static const QString consumerKey;
    static const QString consumerSecret;
    static const QString dropboxRoot;
    static const QString signatureMethod;
    static const QString requestTokenURI;
    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;
    static const QString fileGetURI;
    static const QString filePutURI;
    static const QString createFolderURI;
    static const QString metadataURI;

    explicit DropboxClient(QDeclarativeItem *parent = 0);
    ~DropboxClient();

    QString createTimestamp();
    QString createNonce();
    QString createNormalizedQueryString(QMap<QString, QString> sortMap);
    QByteArray createBaseString(QString method, QString uri, QString queryString);
    QString createSignatureWithHMACSHA1(QString consumerSecret, QString tokenSecret, QByteArray baseString);
    QByteArray createPostData(QString signature, QString queryString);
    QByteArray createOAuthHeaderString(QMap<QString, QString> sortMap);
    QByteArray createOAuthHeaderForUid(QString uid, QString method, QString uri, QMap<QString, QString> addParamMap = QMap<QString, QString>());

    Q_INVOKABLE void requestToken();
    Q_INVOKABLE void authorize();
    Q_INVOKABLE void accessToken();
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE QStringList getStoredUidList();
    Q_INVOKABLE void accountInfo(QString uid);

    Q_INVOKABLE void fileGet(QString nonce, QString uid, QString remoteFilePath, QString localFilePath);
    Q_INVOKABLE QString getDefaultLocalFilePath(const QString &remoteFilePath);
    Q_INVOKABLE void filePut(QString nonce, QString uid, QString localFilePath, QString remoteFilePath);
    Q_INVOKABLE QString getDefaultRemoteFilePath(const QString &localFilePath);
    Q_INVOKABLE void metadata(QString nonce, QString uid, QString remoteFilePath);
signals:
    void requestTokenReplySignal(int err, QString errMsg, QString msg);
    void authorizeRedirectSignal(QString url, QString redirectForm);
    void accessTokenReplySignal(int err, QString errMsg, QString msg);
    void accountInfoReplySignal(int err, QString errMsg, QString msg);

    void fileGetReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void filePutReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void metadataReplySignal(QString nonce, int err, QString errMsg, QString msg);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);
public slots:
    void requestTokenReplyFinished(QNetworkReply *reply);
    void accessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);

    void fileGetReplyFinished(QNetworkReply *reply);
    void filePutReplyFinished(QNetworkReply *reply);
    void metadataReplyFinished(QNetworkReply *reply);
private:
    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;
    TokenPair requestTokenPair;
    QString localPath;
    QFile *localSourcefile;
    QFile *localTargetfile;

    void loadAccessPairMap();
    void saveAccessPairMap();
};

#endif // DropboxClient_H
