#include "appinfo.h"

AppInfo::AppInfo(QDeclarativeItem *parent) :
    QDeclarativeItem(parent)
{
    // Start monitoring if it's enabled.
    toggleMonitoring();
}

QString AppInfo::getVersion() const
{
    qDebug() << "AppInfo::getVersion" << VER;
    return VER;
}

bool AppInfo::isLogging()
{
    return getSettingBoolValue("Logging.enabled", false);
}

void AppInfo::stopLogging()
{
    if (!isLogging()) {
        qInstallMsgHandler(0);
        setSettingValue("Logging.enabled", false);
        qDebug() << "AppInfo::stopLogging is done.";
    } else {
        qDebug() << "AppInfo::stopLogging is ignored. Logging is enabled.";
    }
}

QString AppInfo::getLogPath() const
{
#ifdef Q_OS_SYMBIAN
    return "E:/";
#elif defined(Q_WS_HARMATTAN)
    return "/home/user/MyDocs/";
#else
    return "";
#endif
}

QString AppInfo::getSystemLocale() const
{
    return QLocale::system().name();
}

QString AppInfo::getLocale()
{
    return getSettingValue("locale", "").toString();
}

void AppInfo::setLocale(const QString locale)
{
    if (locale == "") {
        // Load TS.
        loadTS(getSystemLocale());

        // Remove existing setting value.
        removeSettingValue("locale");

        // Emit signal
        emit localeChanged(getSystemLocale());
    } else if (locale != getLocale()) {
        // Load TS.
        loadTS(locale);

        // Save locale to settings.
        setSettingValue("locale", locale);

        // Emit signal
        emit localeChanged(locale);
    }
}

QString AppInfo::getEmptyStr()
{
    return "";
}

QString AppInfo::getEmptySetting()
{
    return "";
}

void AppInfo::addToClipboard(const QString text)
{
    qDebug() << "AppInfo::addToClipboard text" << text;
    QApplication::clipboard()->setText(text);
}

QString AppInfo::getFromClipboard()
{
    QString text = QApplication::clipboard()->text();
    qDebug() << "AppInfo::getFromClipboard text" << text;
    return text;
}

void AppInfo::clearClipboard()
{
    QApplication::clipboard()->clear();
    qDebug() << "AppInfo::clearClipboard";
}

QString AppInfo::getConfigPath()
{
#if defined(Q_WS_HARMATTAN)
    QString sourceFilePath = QDir(QApplication::applicationDirPath()).absoluteFilePath("../config");
#else
    QString sourceFilePath = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).absoluteFilePath("config");
#endif
    return sourceFilePath;
}

bool AppInfo::isMonitoring()
{
    return getSettingBoolValue("Monitoring.enabled", false);
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

    QVariant v = m_settings->value(key, defaultValue);

    qDebug() << "AppInfo::getSettingValue key" << key << "v" << v;

    return v;
}

bool AppInfo::getSettingBoolValue(const QString key, const bool defaultValue)
{
    return getSettingValue(key, defaultValue).toBool();
}

int AppInfo::getSettingIntValue(const QString key, const int defaultValue)
{
    return getSettingValue(key, defaultValue).toInt();
}

bool AppInfo::setSettingValue(const QString key, const QVariant v, const bool forceUpdate)
{
    if (forceUpdate || m_settings->value(key) != v.toString()) {
        qDebug() << "AppInfo::setSettingValue key" << key << "settingValue" << m_settings->value(key) << "v" << v;

        m_settings->setValue(key, v);

        // Sync to backend.
        m_settings->sync();

        // Emit setting changed.
        emit settingChanged();

        return true;
    } else {
        return false;
    }
}

bool AppInfo::hasSettingValue(const QString key)
{
    bool res =  m_settings->contains(key);
    qDebug() << "AppInfo::hasSettingValue key" << key << "contains" << res;
    return res;
}

void AppInfo::removeSettingValue(const QString key)
{
    m_settings->remove(key);
}

void AppInfo::toggleMonitoring()
{
    qDebug() << "AppInfo m_settings isMonitoring()" << isMonitoring();
    if (isMonitoring()) {
        mon = new Monitoring();
        mon->start();
        qDebug() << "AppInfo m_settings mon" << mon << "is started.";
    } else {
        qDebug() << "AppInfo m_settings mon" << mon;
        if (mon != 0) {
            mon->stop();
            mon->deleteLater();
            qDebug() << "AppInfo m_settings mon" << mon << "is stopped.";
        }
    }
}

void AppInfo::init()
{
    if (m_settings != 0) return;

    // Check settings if monitoring is enabled.
    m_settings = new QSettings();
    qDebug() << "AppInfo m_settings fileName()" << m_settings->fileName();

    if (isLogging()) {
#ifdef Q_OS_SYMBIAN
        emit notifyLoggingSignal("E:/");
#elif defined(Q_WS_HARMATTAN)
        emit notifyLoggingSignal("/home/user/MyDocs/");
#endif
    }

    if (isMonitoring()) {
#ifdef Q_OS_SYMBIAN
        emit notifyMonitoringSignal("E:/");
#elif defined(Q_WS_HARMATTAN)
        emit notifyMonitoringSignal("/home/user/MyDocs/");
#endif
    }

    // Initialize translator.
    initTS();
}

void AppInfo::initTS()
{
    m_ts = new QTranslator();
    connect(this, SIGNAL(destroyed()), m_ts, SIGNAL(destroyed()));

    QString localeName = getLocale();
    qDebug() << "AppInfo::initTS settings localeName" << localeName << "translationPath" << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    // Install app translator.
    loadTS(localeName);
}

bool AppInfo::loadTS(const QString localeName)
{
    qDebug() << "AppInfo::loadTS" << getLocale() << "is changing to" << localeName;

    qApp->removeTranslator(m_ts);
    bool res = false;
//    res = m_ts->load(m_settings->applicationName() + "_" + localeName, ":/"); // Load qm file from qrc.
#ifdef Q_OS_SYMBIAN
    res = m_ts->load(m_settings->applicationName() + "_" + localeName, "i18n"); // Load qm file from application default path.
#elif defined(Q_WS_HARMATTAN)
    res = m_ts->load(m_settings->applicationName() + "_" + localeName, QDir(QApplication::applicationDirPath()).absoluteFilePath("../i18n") ); // Load qm file from application binary path.
#endif
    if (res) {
        qDebug() << "AppInfo::loadTS m_ts is loaded. isEmpty" << m_ts->isEmpty();
        qApp->installTranslator(m_ts);
    } else {
        qDebug() << "AppInfo::loadTS m_ts loading failed. isEmpty" << m_ts->isEmpty();
    }

    return res;
}

void AppInfo::componentComplete()
{
    init();
}

