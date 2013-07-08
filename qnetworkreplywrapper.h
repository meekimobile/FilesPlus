#ifndef QNETWORKREPLYWRAPPER_H
#define QNETWORKREPLYWRAPPER_H

#include <QtNetwork>
#include <QObject>
#include <QTimer>
#include <QSettings>

class QNetworkReplyWrapper : public QObject
{
    Q_OBJECT
public:
    static const int DefaultTimeoutMSec;

    explicit QNetworkReplyWrapper(QNetworkReply *reply, bool noTimeout = false, QObject *parent = 0);

    QNetworkReply * data();
signals:
    void uploadProgress(QString nonce, qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(QString nonce, qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void uploadProgressFilter(qint64 bytesSent, qint64 bytesTotal);
    void downloadProgressFilter(qint64 bytesReceived, qint64 bytesTotal);
    void destroyedFilter();

    void idleTimerTimeoutSlot();
private:
    QNetworkReply *m_reply;
    QTimer *m_idleTimer;
    QSettings m_settings;
};

#endif // QNETWORKREPLYWRAPPER_H
