#include <QtGui/QApplication>
#include <QDeclarativeEngine>
#include "qmlapplicationviewer.h"
#include "piechart.h"
#include "foldersizeitemlistmodel.h"
#include "systeminfohelper.h"
#include "gcpclient.h"
#include "clouddrivemodel.h"
#include <QAbstractListModel>
#include "localfileimageprovider.h"
#include "remoteimageprovider.h"
#include "appinfo.h"
#include "bluetoothclient.h"
#include "messageclient.h"
#include "clipboardmodel.h"
#include <QDebug>
#include <QtSql>
#ifdef Q_OS_SYMBIAN
#include "customqnetworkaccessmanagerfactory.h"
#endif

static const QString AppName = "FilesPlus";

bool suppressWarningMsg;

void customMessageHandler(QtMsgType type, const char *msg)
{
    QString txt;
    QString timestampString = QDateTime::currentDateTime().toString(Qt::ISODate);
    switch (type) {
    case QtDebugMsg:
        txt = QString("%1 Debug: %2").arg(timestampString).arg(msg);
        break;
    case QtWarningMsg:
        if (suppressWarningMsg) {
            return;
        }
        txt = QString("%1 Warning: %2").arg(timestampString).arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("%1 Critical: %2").arg(timestampString).arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("%1 Fatal: %2").arg(timestampString).arg(msg);
        break;
    }

#ifdef Q_OS_SYMBIAN
    // Append to file.
    QString filePath = QString("E:/%1_Debug_%2.log").arg(AppName).arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    QFile outFile(filePath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
#elif defined(Q_WS_HARMATTAN)
    // Append to file.
    QString filePath = QString("/home/user/MyDocs/%1_Debug_%2.log").arg(AppName).arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
    QFile outFile(filePath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << endl;
#endif

    // Abort application if fatal msg occurs.
    if (type == QtFatalMsg) {
        abort();
    }
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    qDebug() << QTime::currentTime() << "main started.";
    for (int i = 0; i < argc; i++) {
        qDebug() << "main argv i" << i << argv[i];
    }

    qmlRegisterType<PieChart>("Charts", 1, 0, "PieChart");
    qmlRegisterType<FolderSizeItemListModel>("FolderSizeItemListModel", 1, 0, "FolderSizeItemListModel");
    qmlRegisterType<SystemInfoHelper>("SystemInfoHelper", 1, 0, "SystemInfoHelper");
    qmlRegisterType<GCPClient>("GCPClient", 1, 0, "GCPClient");
    qmlRegisterType<CloudDriveModel>("CloudDriveModel", 1, 0, "CloudDriveModel");
    qmlRegisterType<QAbstractListModel>();
    qmlRegisterType<AppInfo>("AppInfo", 1, 0, "AppInfo");
    qmlRegisterType<BluetoothClient>("BluetoothClient", 1, 0, "BluetoothClient");
    qmlRegisterType<MessageClient>("MessageClient", 1, 0, "MessageClient");
    qmlRegisterType<ClipboardModel>("ClipboardModel", 1, 0, "ClipboardModel");

    // Set properties for QSettings.
    QCoreApplication::setOrganizationName("MeekiMobile");
    QCoreApplication::setApplicationName(AppName);

/*
    // TODO ***** test only.
#ifdef Q_OS_SYMBIAN
    // Rollback to DAT bak for testing.
    qDebug() << "main TEST -------------------------------------------------------";
    bool ok = false;

//    ok = QFile("C:/Data/.config/MeekiMobile/FilesPlus.conf").remove();
//    qDebug() << "main remove C:/Data/.config/MeekiMobile/FilesPlus.conf" << ok;

    QSqlDatabase f_db = QSqlDatabase::addDatabase("QSQLITE", "folderpie_cache");
    f_db.setDatabaseName("FolderPieCache.db");
    ok = f_db.open();
    if (ok) {
        QSqlQuery qry(f_db);
        qry.exec("DROP INDEX folderpie_cache_pk");
        qDebug() << "main" << qry.lastError() << qry.lastQuery();
        qry.exec("DROP TABLE folderpie_cache");
        qDebug() << "main" << qry.lastError() << qry.lastQuery();
    }
    f_db.close();

    ok = QFile("CloudDriveModel.dat").remove();
    qDebug() << "main remove CloudDriveModel.dat" << ok;

    // For testing existing user case.
    ok = QFile("CloudDriveModel.dat.bak").copy("CloudDriveModel.dat");
    qDebug() << "main copy CloudDriveModel.dat.bak to CloudDriveModel.dat" << ok;

    // For testing newly user case.
//    ok = QFile("DropboxClient.dat").remove();
//    qDebug() << "main remove DropboxClient.dat" << ok;

    QSqlDatabase c_db = QSqlDatabase::addDatabase("QSQLITE", "cloud_drive_model");
    c_db.setDatabaseName("CloudDriveModel.db");
    ok = c_db.open();
    if (ok) {
        QSqlQuery qry(c_db);
        qry.exec("DROP INDEX cloud_drive_item_pk");
        qDebug() << "main" << qry.lastError() << qry.lastQuery();
        qry.exec("DROP TABLE cloud_drive_item");
        qDebug() << "main" << qry.lastError() << qry.lastQuery();
    }
    c_db.close();

//    m_settings->setValue("dropbox.fullaccess.enabled", false);
//    qDebug() << "main setting dropbox.fullaccess.enabled" << m_settings->value("dropbox.fullaccess.enabled");

    qDebug() << "main TEST -------------------------------------------------------";
#elif defined(Q_WS_HARMATTAN)
    // Rollback to DAT bak for testing.
    qDebug() << "main TEST -------------------------------------------------------";
    bool ok = false;

//    ok = QFile("/home/developer/.config/MeekiMobile/FilesPlus.conf").remove();
//    qDebug() << "main remove /home/developer/.config/MeekiMobile/FilesPlus.conf" << ok;

//    ok = QFile("/home/user/.config/MeekiMobile/FilesPlus.conf").remove();
//    qDebug() << "main remove /home/user/.config/MeekiMobile/FilesPlus.conf" << ok;

    ok = QFile("/home/user/.folderpie/FolderPieCache.db").remove();
    qDebug() << "main remove /home/user/.folderpie/FolderPieCache.db" << ok;

    ok = QFile("/home/user/.filesplus/CloudDriveModel.dat").remove();
    qDebug() << "main remove /home/user/.filesplus/CloudDriveModel.dat" << ok;

    // For testing existing user case.
//    ok = QFile("/home/user/.filesplus/CloudDriveModel.dat.bak").copy("/home/user/.filesplus/CloudDriveModel.dat");
//    qDebug() << "main copy /home/user/.filesplus/CloudDriveModel.dat.bak to CloudDriveModel.dat" << ok;

    // For testing newly user case.
//    ok = QFile("/home/user/.filesplus/DropboxClient.dat").remove();
//    qDebug() << "main remove /home/user/.filesplus/DropboxClient.dat" << ok;

    ok = QFile("/home/user/.filesplus/CloudDriveModel.db").remove();
    qDebug() << "main remove CloudDriveModel.db" << ok;

//    m_settings->setValue("dropbox.fullaccess.enabled", false);
//    qDebug() << "main setting dropbox.fullaccess.enabled" << m_settings->value("dropbox.fullaccess.enabled");

    qDebug() << "main TEST -------------------------------------------------------";
#endif
*/

    // For testing only.
    qDebug() << "main ApplicationBinaryLocation argv[0]" << argv[0];
    qDebug() << "main ApplicationsLocation" << QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation) << "currentDir" << QDir().absolutePath();
    qDebug() << "main DataLocation" << QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    qDebug() << "main HomeLocation" << QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
    qDebug() << "main DocumentsLocation" << QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    qDebug() << "main DesktopLocation" << QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    qDebug() << "main PicturesLocation" << QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
    qDebug() << "main MusicLocation" << QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
    qDebug() << "main MoviesLocation" << QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);

    // Migrate DAT from C:/ to private folder.
#ifdef Q_OS_SYMBIAN
    QDir sourceDir("C:/");
    foreach (QFileInfo fileInfo, sourceDir.entryInfoList(QStringList("*.dat"))) {
        QString targetFilePath = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).absoluteFilePath(fileInfo.fileName());
        if (QFile::rename(fileInfo.absoluteFilePath(), targetFilePath)) {
            qDebug() << "main moved" << fileInfo.absoluteFilePath() << "to" << targetFilePath;
        } else if (QFile::copy(fileInfo.absoluteFilePath(), targetFilePath)) {
            qDebug() << "main copied" << fileInfo.absoluteFilePath() << "to" << targetFilePath;
            QFile(fileInfo.absoluteFilePath()).remove();
            qDebug() << "main removed" << fileInfo.absoluteFilePath();
        } else {
            qDebug() << "main moving" << fileInfo.absoluteFilePath() << "to" << targetFilePath << "failed.";
        }
    }
#endif

#ifdef Q_OS_SYMBIAN
    // Default logging is enabled on first startup. It will be uninstalled and disabled once DrivePage is loaded successfully.
    QSettings *m_settings = new QSettings();
    qDebug() << "main m_settings fileName()" << m_settings->fileName() << "m_settings->status()" << m_settings->status();
    if (m_settings->value("Logging.enabled", true).toBool()) {
        qDebug() << "main m_settings Logging.enabled=true";
        qInstallMsgHandler(customMessageHandler);
        suppressWarningMsg = m_settings->value("Logging.suppressWarning", true).toBool();
    } else {
        qDebug() << "main m_settings Logging.enabled=false";
    }
#elif defined(Q_WS_HARMATTAN)
    // Default logging is enabled on first startup. It will be uninstalled and disabled once DrivePage is loaded successfully.
    QSettings *m_settings = new QSettings();
    qDebug() << "main m_settings fileName()" << m_settings->fileName() << "m_settings->status()" << m_settings->status();
    if (m_settings->value("Logging.enabled", true).toBool()) {
        qDebug() << "main m_settings Logging.enabled=true";
        qInstallMsgHandler(customMessageHandler);
        suppressWarningMsg = m_settings->value("Logging.suppressWarning", true).toBool();
    } else {
        qDebug() << "main m_settings Logging.enabled=false";
    }
#else
    QSettings *m_settings = new QSettings();
    qDebug() << "main m_settings fileName()" << m_settings->fileName() << "m_settings->status()" << m_settings->status();
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
    bool res = false;
//    res = myappTranslator.load(m_settings->applicationName() + "_" + localeName, ":/"); // Load qm file from qrc.
#ifdef Q_OS_SYMBIAN
    res = myappTranslator.load(m_settings->applicationName() + "_" + localeName, "i18n"); // Load qm file from application default path.
#elif defined(Q_WS_HARMATTAN)
    res = myappTranslator.load(m_settings->applicationName() + "_" + localeName, QDir(app.data()->applicationDirPath()).absoluteFilePath("../i18n") ); // Load qm file from application binary path.
#endif
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
#ifdef Q_OS_SYMBIAN
    engine->addImageProvider(QLatin1String("local"), new LocalFileImageProvider("E:/temp/.fileplus"));
    engine->addImageProvider(QLatin1String("remote"), new RemoteImageProvider("E:/temp/.fileplus"));
#elif defined(Q_WS_HARMATTAN)
    engine->addImageProvider(QLatin1String("local"), new LocalFileImageProvider("/home/user/MyDocs/temp/.filesplus"));
    engine->addImageProvider(QLatin1String("remote"), new RemoteImageProvider("/home/user/MyDocs/temp/.filesplus"));
#endif

#ifdef Q_OS_SYMBIAN
    // Add custom NAMF to change User-Agent to fix problem with Dropbox login page.
    viewer.engine()->setNetworkAccessManagerFactory(new CustomQNetworkAccessManagerFactory());
#endif

    return app->exec();
}
