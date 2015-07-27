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
        // TODO: Create db structures.
        QSqlQuery query(m_db);
        bool res = false;

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

        qDebug() << "DatabaseManager::initializeDB initialize default database successfully.";
    } else {
        qDebug() << "DatabaseManager::initializeDB initialize default database failed." << m_db.lastError().type() << m_db.lastError().text();
    }
}

QSqlDatabase DatabaseManager::getDB()
{
    return m_db;
}
