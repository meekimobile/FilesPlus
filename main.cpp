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
#include "monitoring.h"
#include <QSettings>

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

    QScopedPointer<QApplication> app(createApplication(argc, argv));

    QmlApplicationViewer viewer;
    viewer.setSource(QUrl("qrc:/qml/FilesPlus/main.qml"));
    viewer.showExpanded();
    // *** viewer.rootContext() can't support setContextProperty in Qt Creator
    // *** So I decide to implement PieChart with QDeclarativeItem.

    QDeclarativeEngine *engine = viewer.engine();
    engine->addImageProvider(QLatin1String("local"), new LocalFileImageProvider());

    // Check settings if monitoring is enabled.
    QSettings m_Settings(QApplication::applicationDirPath() + "/" + QApplication::applicationName() + ".conf");
    bool monitoringEnabled = m_Settings.value("Monitoring.enabled", false).toBool();

#ifdef Q_OS_SYMBIAN
    if (monitoringEnabled) {
        Monitoring mon;
        mon.start();
    }
#endif

    return app->exec();
}
