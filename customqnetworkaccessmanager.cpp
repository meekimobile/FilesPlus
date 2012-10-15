#include "customqnetworkaccessmanager.h"
#include <QDebug>

CustomQNetworkAccessManager::CustomQNetworkAccessManager(QObject *parent)
{
}

QNetworkReply * CustomQNetworkAccessManager::createRequest(QNetworkAccessManager::Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
    qDebug() << "CustomQNetworkAccessManager::createRequest request User-Agent" << request.rawHeader("User-Agent");

    QNetworkRequest newReq = QNetworkRequest(request);
    // Issue: Symbian in user agent causes Dropbox to redirect to login page which is not working.
    // Original user agent = "Mozilla/5.0 (Symbian; U; N8-00; en-TH) AppleWebKit/534.3 (KHTML, like Gecko) FilesPlus Mobile Safari/534.3".
    QString userAgent = request.rawHeader("User-Agent");
    userAgent.replace("Symbian", "Nokia Belle");
    newReq.setRawHeader("User-Agent", userAgent.toAscii());

    qDebug() << "CustomQNetworkAccessManager::createRequest newReq User-Agent" << newReq.rawHeader("User-Agent");

    return QNetworkAccessManager::createRequest(op, newReq, outgoingData);
}
