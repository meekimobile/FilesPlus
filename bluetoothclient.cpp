#include "bluetoothclient.h"

BluetoothClient::BluetoothClient(QDeclarativeItem *parent) :
    QDeclarativeItem(parent), m_localDevice(new QBluetoothLocalDevice(this)), m_serviceDiscoveryAgent(new QBluetoothServiceDiscoveryAgent(this))
{
    connect(m_localDevice, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)), this, SIGNAL(hostModeStateChanged(QBluetoothLocalDevice::HostMode)) );
    connect(m_serviceDiscoveryAgent, SIGNAL(serviceDiscovered(QBluetoothServiceInfo)), this, SLOT(serviceDiscoveredFilter(QBluetoothServiceInfo)) );
    connect(m_serviceDiscoveryAgent, SIGNAL(error(QBluetoothServiceDiscoveryAgent::Error)), this, SLOT(errorFilter(QBluetoothServiceDiscoveryAgent::Error)) );
    connect(m_serviceDiscoveryAgent, SIGNAL(canceled()), this, SLOT(canceledFilter()) );
    connect(m_serviceDiscoveryAgent, SIGNAL(finished()), this, SLOT(finishedFilter()) );
}

bool BluetoothClient::sendFile(const QString localPath, const QString deviceAddressStr)
{
    if (!m_localDevice->isValid() || m_localDevice->hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        emit requestPowerOn();
        return false;
    }

    QBluetoothAddress deviceAddress(deviceAddressStr);
    QBluetoothTransferManager mgr;
    QBluetoothTransferRequest req(deviceAddress);

    m_filePath = localPath;
    m_deviceAddressStr = deviceAddressStr;
    m_file = new QFile(localPath);
    m_lastBytesSent = 0;

    emit uploadStarted(localPath, deviceAddressStr);

    m_reply = mgr.put(req, m_file);
    connect(m_reply, SIGNAL(uploadProgress(qint64,qint64)), this, SLOT(uploadProgressFilter(qint64,qint64)));
    connect(m_reply, SIGNAL(finished(QBluetoothTransferReply*)), this, SLOT(uploadFinishedFilter(QBluetoothTransferReply*)));

    if(m_reply->error() != QBluetoothTransferReply::NoError){
        qDebug() << "BluetoothClient::sendFile sending is failed." << m_reply->error() << m_reply->errorString();

        uploadFinishedFilter(m_reply);
        return false;
    }

    return true;
}

void BluetoothClient::uploadAbort()
{
    m_reply->abort();
}

void BluetoothClient::powerOn()
{
    if (m_localDevice->isValid()) {
        m_localDevice->powerOn();
    } else {
        qDebug() << "BluetoothClient::powerOn local device is not available.";
    }
}

void BluetoothClient::powerOff()
{
    if (m_localDevice->isValid()) {
        m_localDevice->setHostMode(QBluetoothLocalDevice::HostPoweredOff);
    } else {
        qDebug() << "BluetoothClient::powerOn local device is not available.";
    }
}

void BluetoothClient::startDiscovery(bool full, bool clear)
{
    qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "BluetoothClient::startDiscovery full" << full
             << "discoveredServices" << m_serviceDiscoveryAgent->discoveredServices().count() << "uuidFilter" << QBluetoothUuid(QBluetoothUuid::ObexObjectPush).toString();

    if (clear) {
        m_btServiceHash.clear();
        m_serviceDiscoveryAgent->clear();
    }
    m_serviceDiscoveryAgent->setUuidFilter(QBluetoothUuid(QBluetoothUuid::ObexObjectPush));
    m_serviceDiscoveryAgent->start( (full ? QBluetoothServiceDiscoveryAgent::FullDiscovery : QBluetoothServiceDiscoveryAgent::MinimalDiscovery) );

    emit discoveryChanged();
}

void BluetoothClient::stopDiscovery()
{
    qDebug() << "BluetoothClient::stopDiscovery";
    m_serviceDiscoveryAgent->stop();
}

bool BluetoothClient::isDiscovery()
{
    return (m_serviceDiscoveryAgent == 0) ? false : m_serviceDiscoveryAgent->isActive();
}

void BluetoothClient::setDiscovery(const bool discovery)
{
    if (discovery) startDiscovery();
}

bool BluetoothClient::isPowerOn()
{
    return getHostMode() != QBluetoothLocalDevice::HostPoweredOff;
}

QBluetoothLocalDevice::HostMode BluetoothClient::getHostMode()
{
    return (m_localDevice == 0) ? QBluetoothLocalDevice::HostPoweredOff : m_localDevice->hostMode();
}

bool BluetoothClient::isFinished()
{
    return (m_reply == 0) ? false : m_reply->isFinished();
}

bool BluetoothClient::isRunning()
{
    return (m_reply == 0) ? false : m_reply->isRunning();
}

bool BluetoothClient::isPaired(const QString deviceAddressStr)
{
    return (m_localDevice == 0) ? false : (m_localDevice->pairingStatus(QBluetoothAddress(deviceAddressStr)) != QBluetoothLocalDevice::Unpaired);
}

bool BluetoothClient::isTrusted(const QString deviceAddressStr)
{
    return (m_localDevice == 0) ? false : (m_localDevice->pairingStatus(QBluetoothAddress(deviceAddressStr)) == QBluetoothLocalDevice::AuthorizedPaired);
}

void BluetoothClient::uploadProgressFilter(qint64 bytesSent, qint64 bytesTotal)
{
    qDebug() << "BluetoothClient::uploadProgressFilter" << bytesSent << bytesTotal;

    m_lastBytesSent = bytesSent;

    emit uploadProgress(m_filePath, m_deviceAddressStr, bytesSent, bytesTotal);
}

void BluetoothClient::uploadFinishedFilter(QBluetoothTransferReply *reply)
{
    qDebug() << "BluetoothClient::uploadFinishedFilter" << m_filePath << m_deviceAddressStr << reply;
    qDebug() << "BluetoothClient::uploadFinishedFilter" << m_filePath << m_deviceAddressStr << reply->error() << reply->errorString() << reply->isFinished() << m_lastBytesSent;

    if (reply->error() == QBluetoothTransferReply::NoError) {
        if (reply->isFinished()) {
            emit uploadProgress(m_filePath, m_deviceAddressStr, m_file->size(), m_file->size());
            emit uploadFinished(m_filePath, m_deviceAddressStr, tr("Transfering is done."), reply->error());
        } else {
            emit uploadFinished(m_filePath, m_deviceAddressStr, tr("Transfering is failed."), reply->error());
        }
    } else {
        emit uploadFinished(m_filePath, m_deviceAddressStr, reply->errorString(), reply->error());
    }

    m_reply->deleteLater();
}

void BluetoothClient::serviceDiscoveredFilter(const QBluetoothServiceInfo &info)
{
    QString deviceAddressStr = info.device().address().toString();

    qDebug() << "BluetoothClient::serviceDiscoveredFilter" << info.device().name() << deviceAddressStr << info.serviceName() << info.serviceUuid().toString();

    if (!m_btServiceHash.contains(deviceAddressStr)) {
        m_btServiceHash[deviceAddressStr] = info.device().name();

        emit serviceDiscovered(info.device().name(), deviceAddressStr, isTrusted(deviceAddressStr), isPaired(deviceAddressStr));
    } else {
        qDebug() << "BluetoothClient::serviceDiscoveredFilter" << info.device().name() << deviceAddressStr << "is suppressed.";
    }
}

void BluetoothClient::errorFilter(QBluetoothServiceDiscoveryAgent::Error error)
{
    qDebug() << "BluetoothClient::errorFilter" << error;
}

void BluetoothClient::finishedFilter()
{
    qDebug() << QDateTime::currentDateTime().toString(Qt::ISODate) << "BluetoothClient::finishedFilter" << m_serviceDiscoveryAgent->error() << m_serviceDiscoveryAgent->errorString();

    emit discoveryChanged();

    // TODO force re-discovery if bluetooth is still on.
    if (!isDiscovery() && isPowerOn() && m_btServiceHash.empty()) {
        startDiscovery(false, false);
    }
}

void BluetoothClient::canceledFilter()
{
    qDebug() << "BluetoothClient::canceledFilter";

    emit discoveryChanged();
}
