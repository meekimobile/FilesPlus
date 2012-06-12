#include "clouddrivemodelthread.h"

CloudDriveModelThread::CloudDriveModelThread(QObject *parent) :
    QThread(parent)
{
}

QString CloudDriveModelThread::getHashFilePath() const
{
    return m_hashFilePath;
}

void CloudDriveModelThread::setHashFilePath(const QString hashFilePath)
{
    m_hashFilePath = hashFilePath;
}

QMultiMap<QString, CloudDriveItem> CloudDriveModelThread::getCloudDriveItems()
{
    return m_cloudDriveItems;
}

void CloudDriveModelThread::run()
{
    loadCloudDriveItems();
}

void CloudDriveModelThread::loadCloudDriveItems() {
    QFile file(m_hashFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> m_cloudDriveItems;

        qDebug() << QTime::currentTime() << "CloudDriveModel::loadCloudDriveItems " << m_cloudDriveItems;

        emit dataLoadedSignal();
    }
}
