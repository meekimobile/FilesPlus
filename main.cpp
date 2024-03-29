#include <QtGui/QApplication>
#include <QDeclarativeEngine>
#include <QDebug>
#include <QtSql>
#include <QAbstractListModel>
#include "qmlapplicationviewer.h"
#include "piechart.h"
#include "foldersizeitemlistmodel.h"
#include "systeminfohelper.h"
#include "gcpclient.h"
#include "clouddrivemodel.h"
#include "localfileimageprovider.h"
#include "remoteimageprovider.h"
#include "appinfo.h"
#include "clipboardmodel.h"
#include "bookmarksmodel.h"
#ifdef Q_OS_SYMBIAN
//#include "bluetoothclient.h"
#include "messageclient.h"
#endif
#include "customqnetworkaccessmanagerfactory.h"
#include "compressedfoldermodel.h"
#include "cacheimagehelper.h"
#include "databasemanager.h"

static const QString OrgName = "MeekiMobile";
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
    // NOTE Log file with .txt as extension will be shown on Document app on N9.
    QString filePath = QString("/home/user/MyDocs/%1_Debug_%2.log.txt").arg(AppName).arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
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

void testReset()
{
#ifdef Q_OS_SYMBIAN
    // Rollback to DAT bak for testing.
    qDebug() << "main testReset TEST -------------------------------------------------------";
    bool ok = false;

//    ok = QFile("C:/Data/.config/MeekiMobile/FilesPlus.conf").remove();
//    qDebug() << "main testReset remove C:/Data/.config/MeekiMobile/FilesPlus.conf" << ok;

    QString privateDrivePath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
    qDebug() << "main testReset privateDrive" << privateDrivePath;
    QDir privateDir(privateDrivePath);
    QStringList nameFilters;
    nameFilters << "*.dat" << "*.db";
    privateDir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    privateDir.setNameFilters(nameFilters);
    foreach (QFileInfo privateItem, privateDir.entryInfoList()) {
        QFile(privateItem.absoluteFilePath()).copy("E:/" + privateItem.fileName());
        qDebug() << "main testReset copy" << privateItem.absoluteFilePath() << "to E:/";
        privateDir.remove(privateItem.fileName());
        qDebug() << "main testReset remove" << privateItem.absoluteFilePath();
    }

//    QSqlDatabase f_db = QSqlDatabase::addDatabase("QSQLITE", "folderpie_cache");
//    f_db.setDatabaseName("FolderPieCache.db");
//    ok = f_db.open();
//    if (ok) {
//        QSqlQuery qry(f_db);
//        qry.exec("DROP INDEX folderpie_cache_pk");
//        qDebug() << "main" << qry.lastError() << qry.lastQuery();
//        qry.exec("DROP TABLE folderpie_cache");
//        qDebug() << "main" << qry.lastError() << qry.lastQuery();
//    }
//    f_db.close();

//    ok = QFile("CloudDriveModel.dat").remove();
//    qDebug() << "main testReset remove CloudDriveModel.dat" << ok;

    // For testing existing user case.
//    ok = QFile("CloudDriveModel.dat.bak").copy("CloudDriveModel.dat");
//    qDebug() << "main testReset copy CloudDriveModel.dat.bak to CloudDriveModel.dat" << ok;

    // For testing newly user case.
//    ok = QFile("DropboxClient.dat").remove();
//    qDebug() << "main testReset remove DropboxClient.dat" << ok;

//    QSqlDatabase c_db = QSqlDatabase::addDatabase("QSQLITE", "cloud_drive_model");
//    c_db.setDatabaseName("CloudDriveModel.db");
//    ok = c_db.open();
//    if (ok) {
//        QSqlQuery qry(c_db);
//        qry.exec("DROP INDEX cloud_drive_item_pk");
//        qDebug() << "main" << qry.lastError() << qry.lastQuery();
//        qry.exec("DROP TABLE cloud_drive_item");
//        qDebug() << "main" << qry.lastError() << qry.lastQuery();
//    }
//    c_db.close();

//    m_settings->setValue("dropbox.fullaccess.enabled", false);
//    qDebug() << "main testReset setting dropbox.fullaccess.enabled" << m_settings->value("dropbox.fullaccess.enabled");

    qDebug() << "main testReset TEST -------------------------------------------------------";
#elif defined(Q_WS_HARMATTAN)
    // Rollback to DAT bak for testing.
    qDebug() << "main testReset TEST -------------------------------------------------------";
    bool ok = false;

//    ok = QFile("/home/developer/.config/MeekiMobile/FilesPlus.conf").remove();
//    qDebug() << "main testReset remove /home/developer/.config/MeekiMobile/FilesPlus.conf" << ok;

//    ok = QFile("/home/user/.config/MeekiMobile/FilesPlus.conf").remove();
//    qDebug() << "main testReset remove /home/user/.config/MeekiMobile/FilesPlus.conf" << ok;

    ok = QFile("/home/user/.folderpie/FolderPieCache.db").remove();
    qDebug() << "main testReset remove /home/user/.folderpie/FolderPieCache.db" << ok;

    ok = QFile("/home/user/.filesplus/CloudDriveModel.dat").remove();
    qDebug() << "main testReset remove /home/user/.filesplus/CloudDriveModel.dat" << ok;

    // For testing existing user case.
//    ok = QFile("/home/user/.filesplus/CloudDriveModel.dat.bak").copy("/home/user/.filesplus/CloudDriveModel.dat");
//    qDebug() << "main testReset copy /home/user/.filesplus/CloudDriveModel.dat.bak to CloudDriveModel.dat" << ok;

    // For testing newly user case.
//    ok = QFile("/home/user/.filesplus/DropboxClient.dat").remove();
//    qDebug() << "main testReset remove /home/user/.filesplus/DropboxClient.dat" << ok;

    ok = QFile("/home/user/.filesplus/CloudDriveModel.db").remove();
    qDebug() << "main testReset remove CloudDriveModel.db" << ok;

//    m_settings->setValue("dropbox.fullaccess.enabled", false);
//    qDebug() << "main testReset setting dropbox.fullaccess.enabled" << m_settings->value("dropbox.fullaccess.enabled");

    qDebug() << "main testReset TEST -------------------------------------------------------";
#endif
}

void initializeDefaultSettings(QSettings *m_settings)
{
    if (!m_settings->contains("drivepage.clouddrive.enabled")) {
        m_settings->setValue("drivepage.clouddrive.enabled", QVariant(true));
    }
    if (!m_settings->contains("dropbox.fullaccess.enabled")) {
        m_settings->setValue("dropbox.fullaccess.enabled", QVariant(true));
    }
    if (!m_settings->contains("CloudDriveModel.syncDirtyItems.enabled")) {
        m_settings->setValue("CloudDriveModel.syncDirtyItems.enabled", QVariant(true));
    }
    if (!m_settings->contains("FolderSizeModelThread.getDirContent.showHiddenSystem.enabled")) {
        m_settings->setValue("FolderSizeModelThread.getDirContent.showHiddenSystem.enabled", QVariant(true));
    }
    if (!m_settings->contains("GCDClient.dirtyBeforeSync.enabled")) {
        m_settings->setValue("GCDClient.dirtyBeforeSync.enabled", QVariant(true));
    }
    if (!m_settings->contains("GCDClient.patchFile.setModifiedDate.enabled")) {
        m_settings->setValue("GCDClient.patchFile.setModifiedDate.enabled", QVariant(true));
    }
    if (!m_settings->contains("GCDClient.deleteFile.permanently.enabled")) {
        m_settings->setValue("GCDClient.deleteFile.permanently.enabled", QVariant(true));
    }
    if (!m_settings->contains("RemoteImageProvider.CacheImageWorker.enabled")) { // NOTE To avoid crash while loading remote image asynchronously.
        m_settings->setValue("RemoteImageProvider.CacheImageWorker.enabled", QVariant(true));
    }
    if (!m_settings->contains("CloudDriveClient.stringifyScriptValue.useCustomLogic")) {
        m_settings->setValue("CloudDriveClient.stringifyScriptValue.useCustomLogic", QVariant(true));
    }
    if (!m_settings->contains("temp.path")) {
#ifdef Q_OS_SYMBIAN
        m_settings->setValue("temp.path", QVariant("E:/temp/.filesplus"));
#elif defined(Q_WS_HARMATTAN)
        m_settings->setValue("temp.path", QVariant("/home/user/MyDocs/temp/.filesplus"));
#endif
    }
    if (!m_settings->contains("image.cache.path")) {
#ifdef Q_OS_SYMBIAN
        m_settings->setValue("image.cache.path", QVariant("E:/temp/.filesplus"));
#elif defined(Q_WS_HARMATTAN)
        m_settings->setValue("image.cache.path", QVariant("/home/user/MyDocs/temp/.filesplus"));
#endif
    }
#ifdef Q_OS_SYMBIAN
    if (!m_settings->contains("CustomQNetworkAccessManager.userAgent.www.dropbox.com")) {
        m_settings->setValue("CustomQNetworkAccessManager.userAgent.www.dropbox.com",
                             "Mozilla/5.0 (Nokia Belle; U; N8-00; en-TH) AppleWebKit/534.3 (KHTML, like Gecko) FilesPlus Mobile Safari/534.3");
    }
#endif
    if (!m_settings->contains("CustomQNetworkAccessManager.userAgent.www.box.com")) {
        m_settings->setValue("CustomQNetworkAccessManager.userAgent.www.box.com",
                             "Mozilla/5.0 (iPhone; U; CPU iPhone OS 4_3_3 like Mac OS X; en-us) AppleWebKit/533.17.9 (KHTML, like Gecko) Version/5.0.2 Mobile/8J2 Safari/6533.18.5");
    }
    m_settings->sync();
}

void initializeDefaultDB()
{
    DatabaseManager::defaultManager().initializeDB();
}

void importCloudDriveClientDAT()
{
#ifdef Q_OS_SYMBIAN
    QDir sourceDir("C:/");
    foreach (QFileInfo fileInfo, sourceDir.entryInfoList(QStringList("*.dat"))) {
        QString targetFilePath = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).absoluteFilePath(fileInfo.fileName());
        if (QFile(targetFilePath).exists()) {
            QFile::rename(targetFilePath, targetFilePath + ".bak");
            qDebug() << "main importCloudDriveClientDAT backup" << targetFilePath << "to" << targetFilePath + ".bak";
        }
        if (QFile::rename(fileInfo.absoluteFilePath(), targetFilePath)) {
            // For file in same drive/partition.
            qDebug() << "main importCloudDriveClientDAT moved" << fileInfo.absoluteFilePath() << "to" << targetFilePath;
        } else if (QFile::copy(fileInfo.absoluteFilePath(), targetFilePath)) {
            // For file in different drive/partition.
            qDebug() << "main importCloudDriveClientDAT copied" << fileInfo.absoluteFilePath() << "to" << targetFilePath;
            QFile(fileInfo.absoluteFilePath()).remove();
            qDebug() << "main importCloudDriveClientDAT removed" << fileInfo.absoluteFilePath();
        } else {
            qDebug() << "main importCloudDriveClientDAT moving" << fileInfo.absoluteFilePath() << "to" << targetFilePath << "failed.";
        }
    }
#endif
}

Q_DECL_EXPORT int main(int argc, char *argv[])
{
    qDebug() << QTime::currentTime() << "main started.";

    // For testing only.
//    testReset();

    // Set properties for QSettings.
    QCoreApplication::setOrganizationName(OrgName);
    QCoreApplication::setApplicationName(AppName);

    // Initializes settings.
    QSettings *m_settings = new QSettings();
    qDebug() << "main m_settings fileName()" << m_settings->fileName() << "m_settings->status()" << m_settings->status();

    // Initialize logging.
#ifdef Q_OS_SYMBIAN
    // Default logging is enabled on first startup. It will be uninstalled and disabled once DrivePage is loaded successfully.
    if (m_settings->value("Logging.enabled", true).toBool()) {
        qDebug() << "main m_settings Logging.enabled=true";
        qInstallMsgHandler(customMessageHandler);
        suppressWarningMsg = m_settings->value("Logging.suppressWarning", true).toBool();
    } else {
        qDebug() << "main m_settings Logging.enabled=false";
    }
#elif defined(Q_WS_HARMATTAN)
    // Default logging is enabled on first startup. It will be uninstalled and disabled once DrivePage is loaded successfully.
    if (m_settings->value("Logging.enabled", true).toBool()) {
        qDebug() << "main m_settings Logging.enabled=true";
        qInstallMsgHandler(customMessageHandler);
        suppressWarningMsg = m_settings->value("Logging.suppressWarning", true).toBool();
    } else {
        qDebug() << "main m_settings Logging.enabled=false";
    }
#endif

    // For testing only.
    for (int i = 0; i < argc; i++) {
        qDebug() << "main argv i" << i << argv[i];
    }
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
    importCloudDriveClientDAT();

    // Default setting values.
    initializeDefaultSettings(m_settings);

    // Default database initialization.
    initializeDefaultDB();

    // Registers QML types.
    qmlRegisterType<PieChart>("Charts", 1, 0, "PieChart");
    qmlRegisterType<FolderSizeItemListModel>("FolderSizeItemListModel", 1, 0, "FolderSizeItemListModel");
    qmlRegisterType<SystemInfoHelper>("SystemInfoHelper", 1, 0, "SystemInfoHelper");
    qmlRegisterType<GCPClient>("GCPClient", 1, 0, "GCPClient");
    qmlRegisterType<CloudDriveModel>("CloudDriveModel", 1, 0, "CloudDriveModel");
    qmlRegisterType<QAbstractListModel>();
    qmlRegisterType<AppInfo>("AppInfo", 1, 0, "AppInfo");
    qmlRegisterType<ClipboardModel>("ClipboardModel", 1, 0, "ClipboardModel");
    qmlRegisterType<BookmarksModel>("BookmarksModel", 1, 0, "BookmarksModel");
#ifdef Q_OS_SYMBIAN
//    qmlRegisterType<BluetoothClient>("BluetoothClient", 1, 0, "BluetoothClient");
    qmlRegisterType<MessageClient>("MessageClient", 1, 0, "MessageClient");
#endif
    qmlRegisterType<CompressedFolderModel>("CompressedFolderModel", 1, 0, "CompressedFolderModel");
    qmlRegisterType<CacheImageHelper>("CacheImageHelper", 1, 0, "CacheImageHelper");

    // Application initialization.
    QScopedPointer<QApplication> app(createApplication(argc, argv));

#ifdef Q_OS_SYMBIAN
    QSplashScreen *splash;
    if (app.data()->desktop()->availableGeometry().width() > 360) {
        // Landscape
        splash = new QSplashScreen(QPixmap(":/qml/FilesPlus/splash_640x360.png"));
    } else {
        // Portrait
        splash = new QSplashScreen(QPixmap(":/qml/FilesPlus/splash_360x640.png"));
    }
    splash->showFullScreen();
#elif defined(Q_WS_HARMATTAN)
    QSplashScreen *splash = new QSplashScreen(QPixmap(":/qml/FilesPlus/splash_854x480.png")); // Workaround: It shows in landscape by default.
    splash->showFullScreen();
#endif

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
    QString i18nPath;
#ifdef Q_OS_SYMBIAN
    i18nPath = "i18n"; // Load qm file from application default path. Use ":/" if load from qrc.
#elif defined(Q_WS_HARMATTAN)
    i18nPath = QDir(app.data()->applicationDirPath()).absoluteFilePath("../i18n"); // Load qm file from application binary path.
#endif
    qDebug() << "main i18nPath" << i18nPath << "exists" << QDir(i18nPath).exists();
    bool res = myappTranslator.load(m_settings->applicationName() + "_" + localeName, i18nPath);
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
#ifdef Q_OS_SYMBIAN
    splash->finish(&viewer);
    splash->deleteLater();
#elif defined(Q_WS_HARMATTAN)
    splash->finish(&viewer);
    splash->deleteLater();
#endif

    QDeclarativeEngine *engine = viewer.engine();

    // Initialize image provider.
    if (!QDir(m_settings->value("image.cache.path").toString()).exists()) {
        QDir().mkpath(m_settings->value("image.cache.path").toString());
    }
    engine->addImageProvider(QLatin1String("local"), new LocalFileImageProvider(m_settings->value("image.cache.path").toString()));
    engine->addImageProvider(QLatin1String("remote"), new RemoteImageProvider(m_settings->value("image.cache.path").toString()));

    // Add custom NAMF to change User-Agent to fix problem with Dropbox, Box login page.
    viewer.engine()->setNetworkAccessManagerFactory(new CustomQNetworkAccessManagerFactory());

    return app->exec();
}
