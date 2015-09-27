#include "customqnetworkaccessmanager.h"
#include <QSslConfiguration>
#include <QDebug>

CustomQNetworkAccessManager::CustomQNetworkAccessManager(QObject *parent)
{
    connect(this, SIGNAL(sslErrors(QNetworkReply*,QList<QSslError>)), this, SLOT(sslErrorsReplyFilter(QNetworkReply*,QList<QSslError>)));
}

QNetworkReply * CustomQNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    QNetworkRequest newReq = QNetworkRequest(request);

    // Force using TLSv1.
    QSslConfiguration sslConfig = newReq.sslConfiguration();
    sslConfig.setProtocol(QSsl::TlsV1);
    newReq.setSslConfiguration(sslConfig);

    // Issue: Symbian in user agent causes Dropbox to redirect to login page which is not working.
    // Original user agent = "Mozilla/5.0 (Symbian; U; N8-00; en-TH) AppleWebKit/534.3 (KHTML, like Gecko) FilesPlus Mobile Safari/534.3".
    // { name: "Safari iOS 4.3.3 iPhone"; value: "Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5" }
    // { name: "Safari iOS 5 iPhone"; value: "Mozilla/5.0 (iPhone; CPU iPhone OS 5_0 like Mac OS X) AppleWebKit/534.46 (KHTML, like Gecko) Version/5.1 Mobile/9A334 Safari/7534.48.3" }
    QString userAgent = request.rawHeader("User-Agent");
    QString propertyName = QString("CustomQNetworkAccessManager.userAgent.%1").arg(request.url().host());
    if (m_settings.contains(propertyName)) {
        qDebug() << "CustomQNetworkAccessManager::createRequest host" << request.url().host() << "request User-Agent" << request.rawHeader("User-Agent");
        userAgent = m_settings.value(propertyName).toString();
        newReq.setRawHeader("User-Agent", userAgent.toAscii());
        qDebug() << "CustomQNetworkAccessManager::createRequest host" << request.url().host() << "newReq User-Agent" << newReq.rawHeader("User-Agent");
    }

    return QNetworkAccessManager::createRequest(op, newReq, outgoingData);
}

void CustomQNetworkAccessManager::sslErrorsReplyFilter(QNetworkReply *reply, QList<QSslError> sslErrors)
{
    QString nonce = reply->request().attribute(QNetworkRequest::User).toString();
    qDebug() << "CustomQNetworkAccessManager::sslErrorsReplyFilter nonce" << nonce << "reply" << reply << QString("Error=%1").arg(reply->error()) << "sslErrors" << sslErrors;

    // NOTE Ignore all errors to support Not yet valid certificates because of SSLv3 poodle security hole detected and Qt4.7 doesn't support newer TLS.
    // sslError = ("The certificate is not yet valid", "The certificate is not yet valid")
    if (m_settings.value("CustomQNetworkAccessManager.ignoreSSLCertificateErrors", QVariant(false)).toBool()) {
        QList<QSslError> expectedSslErrors;
        foreach (QSslError sslError, sslErrors) {
//            qDebug() << "CustomQNetworkAccessManager::sslErrorsReplyFilter sslError error" << sslError.error() << "certificate" << sslError.certificate();
            expectedSslErrors.append(sslError);
        }

        qDebug() << "CustomQNetworkAccessManager::sslErrorsReplyFilter nonce" << nonce << "ignore expectedSslErrors" << expectedSslErrors;
        reply->ignoreSslErrors(expectedSslErrors);
    }
}
