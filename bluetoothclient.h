#ifndef BLUETOOTHCLIENT_H
#define BLUETOOTHCLIENT_H

#include <QDeclarativeItem>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <qbluetoothuuid.h>
#include <qbluetoothserviceinfo.h>
#include <qbluetoothservicediscoveryagent.h>
#include <qbluetoothdeviceinfo.h>
#include <qbluetoothlocaldevice.h>
#include <qbluetoothtransfermanager.h>
#include <qbluetoothtransferrequest.h>
#include <qbluetoothtransferreply.h>

QTM_USE_NAMESPACE

class BluetoothClient : public QDeclarativeItem
{
    Q_OBJECT
    Q_ENUMS(ReplyErrorCode)
    Q_ENUMS(HostModeState)
    Q_PROPERTY(int hostMode READ getHostMode NOTIFY hostModeStateChanged)
    Q_PROPERTY(bool isPowerOn READ isPowerOn NOTIFY hostModeStateChanged)
    Q_PROPERTY(bool isFinished READ isFinished NOTIFY uploadFinished)
    Q_PROPERTY(bool isRunning READ isRunning NOTIFY uploadFinished)
    Q_PROPERTY(bool discovery READ isDiscovery WRITE setDiscovery NOTIFY discoveryChanged)
public:
    static const QString DATA_FILE_PATH;
    static const int MAX_DISCOVERY_RETRY;

    enum ReplyErrorCode {
        UploadAbort = -1
    };

    enum HostModeState {
        HostPoweredOff = 0,
        HostConnectable,
        HostDiscoverable,
        HostDiscoverableLimitedInquiry
    };

    explicit BluetoothClient(QDeclarativeItem *parent = 0);
    ~BluetoothClient();

    Q_INVOKABLE bool sendFile(const QString localPath, const QString deviceAddressStr);
    Q_INVOKABLE void uploadAbort();
    Q_INVOKABLE void powerOn();
    Q_INVOKABLE void powerOff();

    Q_INVOKABLE void requestDiscoveredDevices();
    Q_INVOKABLE void startDiscovery(bool full = false, bool clear = false);
    Q_INVOKABLE void stopDiscovery();
    bool isDiscovery();
    void setDiscovery(const bool discovery);

    bool isPowerOn();
    QBluetoothLocalDevice::HostMode getHostMode();

    bool isFinished();
    bool isRunning();
    Q_INVOKABLE bool isTrusted(const QString deviceAddressStr);
    Q_INVOKABLE bool isPaired(const QString deviceAddressStr);
signals:
    void uploadStarted(const QString localPath, const QString deviceAddressStr);
    void uploadProgress(const QString localPath, const QString deviceAddressStr, qint64 bytesSent, qint64 bytesTotal);
    void uploadFinished(const QString localPath, const QString deviceAddressStr, const QString msg, const int err);
    void abort();
    void hostModeStateChanged(QBluetoothLocalDevice::HostMode hostMode);
    void requestPowerOn();
    void discoveryChanged();
    void serviceDiscovered(const QString deviceName, const QString deviceAddress, const bool isTrusted, const bool isPaired, const bool isNear);
public slots:
    void uploadProgressFilter(qint64 bytesSent, qint64 bytesTotal);
    void uploadFinishedFilter(QBluetoothTransferReply *reply);
    void serviceDiscoveredFilter(const QBluetoothServiceInfo &info);
    void errorFilter(QBluetoothServiceDiscoveryAgent::Error error);
    void finishedFilter();
    void canceledFilter();
private:
    int m_discoveryRetry;
    QBluetoothLocalDevice *m_localDevice;
    QBluetoothServiceDiscoveryAgent *m_serviceDiscoveryAgent;
    QHash<QString, QString> m_btServiceHash;
    QFile *m_file;
    QString m_filePath;
    QString m_deviceAddressStr;
    qint64 m_lastBytesSent;
    QBluetoothTransferReply *m_reply;
    QList<QBluetoothUuid> m_uuidFilterList;

    void loadBtServiceHash();
    void saveBtServiceHash();
};

#endif // BLUETOOTHCLIENT_H
