#include <QtGui/QApplication>
#include <QDeclarativeEngine>
#include "qmlapplicationviewer.h"
#include "piechart.h"
#include "foldersizeitemlistmodel.h"
#include "systeminfohelper.h"
#include "gcpclient.h"
#include "clouddrivemodel.h"
#include "dropboxclient.h"
#include <QAbstractListModel>
#include "localfileimageprovider.h"
#include "appinfo.h"
#include "bluetoothclient.h"
#include "messageclient.h"
#include <QDebug>

void customMessageHandler(QtMsgType type, const char *msg)
{
    QString txt;
    QString timestampString = QDateTime::currentDateTime().toString(Qt::ISODate);
    switch (type) {
    case QtDebugMsg:
        txt = QString("%1 Debug: %2").arg(timestampString).arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("%1 Warning: %2").arg(timestampString).arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("%1 Critical: %2").arg(timestampString).arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("%1 Fatal: %2").arg(timestampString).arg(msg);
        abort();
    }

#ifdef Q_OS_SYMBIAN
    // Append to file.
    QString filePath = QString("E:/FilesPlus_Debug_%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    QFile outFile(filePath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
#elif defined(Q_WS_HARMATTAN)
    // Append to file.
    QString filePath = QString("/home/user/FilesPlus_Debug_%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    QFile outFile(filePath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
#endif
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    qDebug() << QTime::currentTime() << "main started.";

    qmlRegisterType<PieChart>("Charts", 1, 0, "PieChart");
    qmlRegisterType<FolderSizeItemListModel>("FolderSizeItemListModel", 1, 0, "FolderSizeItemListModel");
    qmlRegisterType<SystemInfoHelper>("SystemInfoHelper", 1, 0, "SystemInfoHelper");
    qmlRegisterType<GCPClient>("GCPClient", 1, 0, "GCPClient");
    qmlRegisterType<CloudDriveModel>("CloudDriveModel", 1, 0, "CloudDriveModel");
    qmlRegisterType<DropboxClient>("DropboxClient", 1, 0, "DropboxClient");
    qmlRegisterType<QAbstractListModel>();
    qmlRegisterType<AppInfo>("AppInfo", 1, 0, "AppInfo");
    qmlRegisterType<BluetoothClient>("BluetoothClient", 1, 0, "BluetoothClient");
    qmlRegisterType<MessageClient>("MessageClient", 1, 0, "MessageClient");

    // Set properties for QSettings.
    QCoreApplication::setOrganizationName("MeekiMobile");
    QCoreApplication::setApplicationName("FilesPlus");

#ifdef Q_OS_SYMBIAN
    // Check settings if logging is enabled.
    QSettings *m_settings = new QSettings();
    qDebug() << "main m_settings fileName()" << m_settings->fileName() << "m_settings->status()" << m_settings->status();
    if (m_settings->value("Logging.enabled", false).toBool()) {
        qDebug() << "main m_settings Logging.enabled=true";
        qInstallMsgHandler(customMessageHandler);
    } else {
        qDebug() << "main m_settings Logging.enabled=false";
    }
#elif defined(Q_WS_HARMATTAN)
    // Check settings if logging is enabled.
    QSettings *m_settings = new QSettings();
    qDebug() << "main m_settings fileName()" << m_settings->fileName() << "m_settings->status()" << m_settings->status();
    if (m_settings->value("Logging.enabled", false).toBool()) {
        qDebug() << "main m_settings Logging.enabled=true";
        qInstallMsgHandler(customMessageHandler);
    } else {
        qDebug() << "main m_settings Logging.enabled=false";
    }
#endif

    QScopedPointer<QApplication> app(createApplication(argc, argv));

    // Get defined locale in settings or system locale if it's not defined.
    QString localeName = m_settings->value("locale", QLocale::system().name()).toString();
    qDebug() << "main localeName" << localeName << "translationPath" << QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    // Install bundled Qt translator.
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + localeName, QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app.data()->installTranslator(&qtTranslator);

    // ISSUE: If enable code below, causes unable to remove translator and switch back to english. Requires English translation to make it works.
    // Install default app translator.
    QTranslator myappTranslator;
    bool res = myappTranslator.load(m_settings->applicationName() + "_" + localeName, ":/");
    if (res) {
        qDebug() << "main myappTranslation is loaded. isEmpty" << myappTranslator.isEmpty();
        app.data()->installTranslator(&myappTranslator);
    } else {
        qDebug() << "main myappTranslation loading failed. isEmpty" << myappTranslator.isEmpty();
    }

    QmlApplicationViewer viewer;
    viewer.setSource(QUrl("qrc:/qml/FilesPlus/main.qml"));
    viewer.showExpanded();
    // *** viewer.rootContext() can't support setContextProperty in Qt Creator
    // *** So I decide to implement PieChart with QDeclarativeItem.

    QDeclarativeEngine *engine = viewer.engine();
    engine->addImageProvider(QLatin1String("local"), new LocalFileImageProvider());

    return app->exec();
}
