#include "clouddrivemodelthread.h"

CloudDriveModelThread::CloudDriveModelThread(QObject *parent) :
    QObject(parent), QRunnable()
{
    m_isDirectInvokation = false;
}

void CloudDriveModelThread::setNonce(QString nonce)
{
    m_nonce = nonce;
}

QString CloudDriveModelThread::getNonce()
{
    return m_nonce;
}

void CloudDriveModelThread::setDirectInvokation(bool flag)
{
    m_isDirectInvokation = flag;
}

bool CloudDriveModelThread::isDirectInvokation()
{
    return m_isDirectInvokation;
}

void CloudDriveModelThread::run()
{
    if (m_isDirectInvokation) {
        qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << QThread::currentThread() << "started. Invoking dispatchJob method on parent" << parent() << " with direct connection.";
        // Direct invokation to target object's eventloop.
        QMetaObject::invokeMethod(parent(), "dispatchJob", Qt::DirectConnection, Q_ARG(QString, m_nonce));
    } else {
        qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << QThread::currentThread() << "started. Invoking dispatchJob method on parent" << parent() << " with queued connection.";
        // Queue invokation to target object's eventloop.
        QMetaObject::invokeMethod(parent(), "dispatchJob", Qt::QueuedConnection, Q_ARG(QString, m_nonce));
    }
    qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << QThread::currentThread() << "done.";
}

void CloudDriveModelThread::start()
{
    run();
}
