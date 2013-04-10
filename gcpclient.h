#ifndef GCPClient_H
#define GCPClient_H

#include <QDeclarativeItem>
#include <QtNetwork>
#include "tokenpair.h"

class GCPClient : public QDeclarativeItem
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

    static const QString submitURI;
    static const QString jobsURI;
    static const QString deletejobURI;
    static const QString printerURI;
    static const QString searchURI;

    explicit GCPClient(QDeclarativeItem *parent = 0);
    ~GCPClient();

    QString createTimestamp();
    QString createNonce();
    QString createQueryString(QMap<QString, QString> sortMap);
    QMap<QString, QString> createMapFromJson(QString jsonText);
    QHash<QString, QString> createHashFromScriptValue(QString parentName, QScriptValue sc);
    QHash<QString, QString> createHashFromJson(QString jsonText);
    QByteArray encodeMultiPart(QString boundary, QMap<QString, QString> paramMap, QString fileParameter, QString fileName, QByteArray fileData, QString contentType);

    // Mime.types mapping.
    Q_INVOKABLE void loadContentTypeHash(QString sourceFilePath);

    Q_INVOKABLE void authorize();
    Q_INVOKABLE bool parseAuthorizationCode(QString text);
    Q_INVOKABLE void accessToken();
    Q_INVOKABLE QStringList getStoredUidList();
    Q_INVOKABLE bool isAuthorized();
    Q_INVOKABLE void refreshAccessToken();
    Q_INVOKABLE void accountInfo();
    Q_INVOKABLE QNetworkReply *search(QString q);
    Q_INVOKABLE QString getContentType(QString fileName);
    Q_INVOKABLE QString getPrinterId(int i);
    Q_INVOKABLE int getPrinterIndex(QString printerId);
    Q_INVOKABLE QStringList getStoredPrinterList();
    Q_INVOKABLE QString getPrinterType(int i);
    Q_INVOKABLE QString getPrinterType(QString printerId);
    Q_INVOKABLE QNetworkReply *submit(QString printerId, QString title, QString capabilities, QString contentPath, QString contentType, QString tag);
    Q_INVOKABLE QNetworkReply *submit(QString printerId, QString contentPath, QString capabilities); // Simplify submit method
    Q_INVOKABLE void jobs(QString printerId);
    Q_INVOKABLE void deletejob(QString jobId, bool refreshAfterDelete = true);
    Q_INVOKABLE void printer(QString printerId);
signals:
    void authorizeRedirectSignal(QString url);
    void accessTokenReplySignal(int err, QString errMsg, QString msg);
    void refreshAccessTokenReplySignal(int err, QString errMsg, QString msg);
    void accountInfoReplySignal(int err, QString errMsg, QString msg);
    void searchReplySignal(int err, QString errMsg, QString msg);
    void submitReplySignal(int err, QString errMsg, QString msg);
    void jobsReplySignal(int err, QString errMsg, QString msg);
    void deletejobReplySignal(int err, QString errMsg, QString msg);
    void printerReplySignal(int err, QString errMsg, QString msg);

    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
public slots:
    void accessTokenReplyFinished(QNetworkReply *reply);
    void refreshAccessTokenReplyFinished(QNetworkReply *reply);
    void accountInfoReplyFinished(QNetworkReply *reply);
    void searchReplyFinished(QNetworkReply *reply);
    void submitReplyFinished(QNetworkReply *reply);
    void jobsReplyFinished(QNetworkReply *reply);
    void deletejobReplyFinished(QNetworkReply *reply);
    void printerReplyFinished(QNetworkReply *reply);
private:
    QMap<QString, QString> m_paramMap;
    QMap<QString, TokenPair> accessTokenPairMap;
    QHash<QString, QString> m_printerHash;
    QHash<QString, QString> m_contentTypeHash;

    QDateTime m_paramMapLastModified;
    bool isParamMapChanged();

    void loadParamMap();
    void saveParamMap();
};

#endif // GCPClient_H
