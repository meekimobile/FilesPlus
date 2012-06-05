#include "qnetworkreplywrapper.h"

QNetworkReplyWrapper::QNetworkReplyWrapper(QNetworkReply *reply, QObject *parent) :
    QObject(parent)
{
    m_reply = reply;
    connect(m_reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)) );
    connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressFilter(qint64,qint64)) );
}

void QNetworkReplyWrapper::uploadProgressFilter(qint64 bytesSent, qint64 bytesTotal)
{
    QString nonce = m_reply->request().attribute(QNetworkRequest::User).toString();

    emit uploadProgress(nonce, bytesSent, bytesTotal);
}

void QNetworkReplyWrapper::downloadProgressFilter(qint64 bytesReceived, qint64 bytesTotal)
{
    QString nonce = m_reply->request().attribute(QNetworkRequest::User).toString();

    emit downloadProgress(nonce, bytesReceived, bytesTotal);
}
