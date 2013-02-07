#include "clouddrivemodelthread.h"
#include "clouddrivemodel.h"

CloudDriveModelThread::CloudDriveModelThread(QObject *parent) :
    QThread(parent)
{
    m_isDirectInvokation = false;
}

QString CloudDriveModelThread::getHashFilePath() const
{
    return m_hashFilePath;
}

void CloudDriveModelThread::setHashFilePath(const QString hashFilePath)
{
    m_hashFilePath = hashFilePath;
}

void CloudDriveModelThread::setNonce(QString nonce)
{
    m_nonce = nonce;
}

void CloudDriveModelThread::setDirectInvokation(bool flag)
{
    m_isDirectInvokation = flag;
}

void CloudDriveModelThread::setRunMethod(int method)
{
    m_runMethod = method;
}

void CloudDriveModelThread::setCloudDriveItems(QMultiMap<QString, CloudDriveItem> *cloudDriveItems)
{
    m_cloudDriveItems = cloudDriveItems;
}

void CloudDriveModelThread::run()
{
    qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << currentThread() << "started. Invoking dispatchJob method on parent" << parent();
    if (m_isDirectInvokation) {
        // Direct invokation to target object's eventloop.
        QMetaObject::invokeMethod(parent(), "dispatchJob", Qt::DirectConnection, Q_ARG(QString, m_nonce));
    } else {
        // Queue invokation to target object's eventloop.
        QMetaObject::invokeMethod(parent(), "dispatchJob", Qt::QueuedConnection, Q_ARG(QString, m_nonce));
    }
    qDebug() << "CloudDriveModelThread::run job" << m_nonce << "thread" << currentThread() << "done";
}

void CloudDriveModelThread::loadCloudDriveItems() {
    QFile file(m_hashFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> *m_cloudDriveItems;
    }

    qDebug() << QTime::currentTime() << "CloudDriveModelThread::loadCloudDriveItems " << m_cloudDriveItems->size();

    emit loadCloudDriveItemsFinished(m_nonce);
}
