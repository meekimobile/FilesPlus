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

void AppInfo::manageMonitoring()
{
#ifdef Q_OS_SYMBIAN
    qDebug() << "AppInfo m_settings isMonitoring()" << isMonitoring();
    if (isMonitoring()) {
        mon = new Monitoring();
        mon->start();
    } else {
        mon->deleteLater();
    }
#endif
}

void AppInfo::componentComplete()
{
    // Check settings if monitoring is enabled.
    m_settings = new QSettings(m_domainName, m_appName);
    qDebug() << "AppInfo m_settings fileName()" << m_settings->fileName();

    // Start monitoring if it's enabled.
    manageMonitoring();
}

