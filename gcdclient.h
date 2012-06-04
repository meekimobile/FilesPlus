#ifndef GCDClient_H
#define GCDClient_H

#include <QDeclarativeItem>
#include <QtNetwork>
#include "tokenpair.h"

class GCDClient : public QDeclarativeItem
{
    Q_OBJECT
public:
    static const QString KeyStoreFilePath;
    static const QString consumerKey;
    static const QString consumerSecret;
    static const QString authorizationScope;

    static const QString authorizeURI;
    static const QString accessTokenURI;
    static const QString accountInfoURI;

    static const QString metadataURI;
    static const QString insertURI;
    static const QString patchMetadataURI;
    static const QString updateMetadataURI;

    explicit GCDClient(QDeclarativeItem *parent = 0);
    ~GCDClient();

    QString createTimestamp();
    QString createNonce();
    QString createQueryString(QMap<QString, QString> sortMap);
    QMap<QString, QString> createMapFromJson(QString jsonText);
    QHash<QString, QString> createHashFromScriptValue(QString parentName, QScriptValue sc);
    QHash<QString, QString> createHashFromJson(QString jsonText);
    QByteArray encodeMultiPart(QString boundary, QMap<QString, QString> paramMap, QString fileParameter, QString fileName, QByteArray fileData, QString contentType);

    Q_INVOKABLE void authorize();
    Q_INVOKABLE bool parseAuthorizationCode(QString text);
    Q_INVOKABLE void accessToken();
    Q_INVOKABLE QStringList getStoredUidList();
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE void refreshAccessToken();
    Q_INVOKABLE void accountInfo();
    Q_INVOKABLE QString getContentType(QString fileName);

    Q_INVOKABLE void metadata();
    Q_INVOKABLE void fileGet(QString uid, QString remoteFilePath, QString localFilePath);
    Q_INVOKABLE void filePut(QString uid, QString localFilePath, QString remoteFilePath);
signals:
    void authorizeRedirectSignal(QString url, QString redirectFrom);
    void accessTokenReplySignal(int err, QString errMsg, QString msg);
    void refreshAccessTokenReplySignal(int err, QString errMsg, QString msg);
    void accountInfoReplySignal(int err, QString errMsg, QString msg);

    void metadataReplySignal(int err, QString errMsg, QString msg);
    void fileGetReplySignal(int err, QString errMsg, QString msg);
    void filePutReplySignal(int err, QString errMsg, QString msg);

    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void refreshAccessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);

    void metadataReplyFinished(QNetworkReply *reply);
    void fileGetReplyFinished(QNetworkReply *reply);
    void filePutReplyFinished(QNetworkReply *reply);
private:
    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;
    QHash<QString, QString> m_contentTypeHash;

    void loadParamMap();
    void saveParamMap();
};

#endif // GCDClient_H
