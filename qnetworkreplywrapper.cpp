#include "qnetworkreplywrapper.h"

const int QNetworkReplyWrapper::DefaultTimeoutMSec = 60000;

QNetworkReplyWrapper::QNetworkReplyWrapper(QNetworkReply *reply, QObject *parent) :
    QObject(parent)
{
    m_reply = reply;
    connect(m_reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)) );
    connect(m_reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgressFilter(qint64,qint64)) );
    connect(m_reply, SIGNAL(destroyed()), this, SLOT(deleteLater()) );

    // Initialize idle timer.
    m_idleTimer = new QTimer(this);
    connect(m_idleTimer, SIGNAL(timeout()), this, SLOT(idleTimerTimeoutSlot()) );
    connect(m_reply, SIGNAL(destroyed()), m_idleTimer, SLOT(stop()) );
    connect(m_reply, SIGNAL(destroyed()), m_idleTimer, SLOT(deleteLater()) );
    m_idleTimer->setInterval(m_settings.value("QNetworkReplyWrapper.timeout.interval", QVariant(DefaultTimeoutMSec)).toInt());
    m_idleTimer->setSingleShot(true);
    m_idleTimer->start();
}

void QNetworkReplyWrapper::uploadProgressFilter(qint64 bytesSent, qint64 bytesTotal)
{
    QString nonce = m_reply->request().attribute(QNetworkRequest::User).toString();

    emit uploadProgress(nonce, bytesSent, bytesTotal);

    // Restart idle timer if there is any progress.
    m_idleTimer->start();
}

void QNetworkReplyWrapper::downloadProgressFilter(qint64 bytesReceived, qint64 bytesTotal)
{
    QString nonce = m_reply->request().attribute(QNetworkRequest::User).toString();

    emit downloadProgress(nonce, bytesReceived, bytesTotal);

    // Restart idle timer if there is any progress.
    m_idleTimer->start();
}

void QNetworkReplyWrapper::idleTimerTimeoutSlot()
{
    // Abort reply if it's timeout.
    if (m_reply != 0) {
        QString nonce = m_reply->request().attribute(QNetworkRequest::User).toString();
        if (m_reply->isFinished()) {
            qDebug() << "QNetworkReplyWrapper::idleTimerTimeoutSlot nonce" << nonce << "reply" << m_reply << "is finished. Operation is ignored.";
        } else {
            m_reply->abort();
            qDebug() << "QNetworkReplyWrapper::idleTimerTimeoutSlot nonce" << nonce << "reply" << m_reply << "is aborted.";
        }
    } else {
        qDebug() << "QNetworkReplyWrapper::idleTimerTimeoutSlot reply = 0. Operation is ignored.";
    }
}
