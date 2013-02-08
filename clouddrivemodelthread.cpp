#include "clouddrivemodelthread.h"

CloudDriveModelThread::CloudDriveModelThread(QObject *parent) :
    QRunnable()
{
    m_parent = parent;
    m_isDirectInvokation = false;
}

void CloudDriveModelThread::setNonce(QString nonce)
{
    m_nonce = nonce;
}

void CloudDriveModelThread::setDirectInvokation(bool flag)
{
    m_isDirectInvokation = flag;
}

void CloudDriveModelThread::run()
{
    if (m_isDirectInvokation) {
        qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << QThread::currentThread() << "started. Invoking dispatchJob method on parent" << m_parent << " with direct connection.";
        // Direct invokation to target object's eventloop.
        QMetaObject::invokeMethod(m_parent, "dispatchJob", Qt::DirectConnection, Q_ARG(QString, m_nonce));
    } else {
        qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << QThread::currentThread() << "started. Invoking dispatchJob method on parent" << m_parent << " with queued connection.";
        // Queue invokation to target object's eventloop.
        QMetaObject::invokeMethod(m_parent, "dispatchJob", Qt::QueuedConnection, Q_ARG(QString, m_nonce));
    }
    qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << QThread::currentThread() << "done.";
}
