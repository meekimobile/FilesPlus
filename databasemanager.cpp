#include "databasemanager.h"

void DatabaseManager::initializeDB()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
#if defined(Q_WS_HARMATTAN)
    // Create cache database path if it's not exist.
    QDir dbPath("/home/user/.filesplus");
    if (!dbPath.exists()) {
        qDebug() << "DatabaseManager::initializeDB default database path" << dbPath.absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(dbPath.absolutePath());
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB default database path" << dbPath.absolutePath() << "can't be created.";
        } else {
            qDebug() << "DatabaseManager::initializeDB default database path" << dbPath.absolutePath() << "is created.";
        }
    }
    m_db.setDatabaseName(dbPath.absoluteFilePath("default.db"));
#else
    m_db.setDatabaseName("default.db");
#endif
    if (m_db.open()) {
        QSqlQuery query(m_db);
        bool res = false;

        // TODO: Create db structures according to common design (iOS, Android).
        res = query.exec("CREATE TABLE User(uid TEXT PRIMARY KEY, accountType TEXT, token TEXT, secret TEXT, enabled INTEGER, lastEventId TEXT, userData TEXT)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        res = query.exec("CREATE TABLE StorageItemMetadata(storageType INTEGER, accountType TEXT, uid TEXT, localPath TEXT, remotePath TEXT, timestamp TEXT, storageItemData TEXT, url TEXT)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS StorageItemMetadataPK ON StorageItemMetadata (storageType, accountType, uid, localPath, remotePath)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        res = query.exec("CREATE TABLE StorageConnection(sourcePortableKey TEXT, targetPortableKey TEXT, connectionType INTEGER, connectionData TEXT, enabled INTEGER)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS StorageConnectionPK ON StorageConnection (sourcePortableKey, targetPortableKey)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        res = query.exec("CREATE TABLE Bookmark(portableKey TEXT PRIMARY KEY, name TEXT)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        // Original FolderPie's tables.
        res = query.exec("CREATE TABLE folderpie_cache(id TEXT PRIMARY_KEY, name TEXT, absolute_path TEXT, last_modified TEXT, size INTEGER, is_dir INTEGER, sub_dir_count INTEGER, sub_file_count INTEGER, file_type TEXT, sort_flag INTEGER)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS folderpie_cache_pk ON folderpie_cache (id)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        // Original CloudDriveModel's tables.
        res = query.exec("CREATE TABLE cloud_drive_item(type INTEGER PRIMARY_KEY, uid TEXT PRIMARY_KEY, local_path TEXT PRIMARY_KEY, remote_path TEXT, hash TEXT, last_modified TEXT, cron_exp TEXT, sync_direction INTEGER)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }
        res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS cloud_drive_item_pk ON cloud_drive_item (type, uid, local_path)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }
        res = query.exec("CREATE TABLE cloud_drive_item_property(type INTEGER PRIMARY_KEY, uid TEXT PRIMARY_KEY, remote_path TEXT PRIMARY_KEY, property_name TEXT PRIMARY_KEY, property_value TEXT)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }
        res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS cloud_drive_item_property_pk ON cloud_drive_item_property (type, uid, remote_path, property_name)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        // Original Bookmark's tables.
        res = query.exec("CREATE TABLE bookmarks(type INTEGER PRIMARY_KEY, uid TEXT PRIMARY_KEY, path TEXT PRIMARY_KEY, title TEXT)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS bookmarks_pk ON bookmarks (type, uid, path)");
        if (!res) {
            qDebug() << "DatabaseManager::initializeDB Error" << query.lastError();
        }

        qDebug() << "DatabaseManager::initializeDB initialize default database successfully. Tables" << m_db.tables();
    } else {
        qDebug() << "DatabaseManager::initializeDB initialize default database failed." << m_db.lastError().type() << m_db.lastError().text();
    }
}

QSqlDatabase DatabaseManager::getDB()
{
    return m_db;
}
