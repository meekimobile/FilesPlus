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
#include <QDebug>

void customMessageHandler(QtMsgType type, const char *msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        abort();
    }

#ifdef Q_OS_SYMBIAN
    // Append to file.
    QString filePath = QString("E:/FilesPlus_Debug_%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    QFile outFile(filePath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
#elif defined(Q_OS_LINUX)
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
#elif defined(Q_OS_LINUX)
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

    QmlApplicationViewer viewer;
    viewer.setSource(QUrl("qrc:/qml/FilesPlus/main.qml"));
    viewer.showExpanded();
    // *** viewer.rootContext() can't support setContextProperty in Qt Creator
    // *** So I decide to implement PieChart with QDeclarativeItem.

    QDeclarativeEngine *engine = viewer.engine();
    engine->addImageProvider(QLatin1String("local"), new LocalFileImageProvider());

    return app->exec();
}
