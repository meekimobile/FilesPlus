#include "appinfo.h"

AppInfo::AppInfo(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
}

QString AppInfo::getDomainName() const
{
    return m_domainName;
}

void AppInfo::setDomainName(const QString domainName)
{
    m_domainName = domainName;
}

QString AppInfo::getAppName() const
{
    return m_appName;
}

void AppInfo::setAppName(const QString appName)
{
    m_appName = appName;
}

bool AppInfo::isMonitoring() const
{
    return m_settings->value("Monitoring.enabled", false).toBool();
}

void AppInfo::setMonitoring(const bool flag)
{
    m_settings->setValue("Monitoring.enabled", flag);
    m_settings->sync();
}

QString AppInfo::getMonitoringFilePath() const
{
    if (mon != 0) {
        return mon->getMonitoringFilePath();
    }
    return "";
}

QVariant AppInfo::getSettingValue(const QString key, const QVariant defaultValue)
{
    // Initialize if it's not done.
    init();

    // Sync to backend.
    m_settings->sync();

    return m_settings->value(key, defaultValue);
}

void AppInfo::setSettingValue(const QString key, const QVariant v)
{
    m_settings->setValue(key, v);

    // Sync to backend.
    m_settings->sync();

    // Call startMonitoring.
    startMonitoring();
}

void AppInfo::startMonitoring()
{
#ifdef Q_OS_SYMBIAN
    qDebug() << "AppInfo m_settings isMonitoring()" << isMonitoring();
    if (isMonitoring()) {
        mon = new Monitoring();
        mon->start();
    } else {
        qDebug() << "AppInfo m_settings mon" << mon;
        if (mon != 0) {
            mon->stop();
            mon->deleteLater();
            qDebug() << "AppInfo m_settings mon" << mon << "is stopped.";
        }
    }
#endif
}

void AppInfo::init()
{
    if (m_settings != 0) return;

    // Check settings if monitoring is enabled.
    m_settings = new QSettings(m_domainName, m_appName);
    qDebug() << "AppInfo m_settings fileName()" << m_settings->fileName();
}

void AppInfo::componentComplete()
{
    init();
}

