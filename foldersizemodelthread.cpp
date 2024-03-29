#include "foldersizemodelthread.h"
#include "foldersizeitem.h"
#include "foldersizeitemlistmodel.h"
#include "databasemanager.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDesktopServices>
#ifdef Q_OS_SYMBIAN
#include <f32file.h>
#include <e32base.h>
#include <e32debug.h>
#endif

bool nameLessThan(const FolderSizeItem &o1, const FolderSizeItem &o2)
{
//    qDebug() << "nameLessThan" << o1.name << o1.fileType << o2.name << o2.fileType;
    return o1.name.toLower() < o2.name.toLower();
}

bool nameLessThanWithDirectoryFirst(const FolderSizeItem &o1, const FolderSizeItem &o2)
{
    if (o1.isDir && !o2.isDir) {
        // If dir before file, return true.
        return true;
    } else if (!o1.isDir && o2.isDir) {
        // If file before dir, return false;
        return false;
    } else {
        // If both are dir/file, compare name.
        return o1.name.toLower() < o2.name.toLower();
    }
}

bool timeGreaterThan(const FolderSizeItem &o1, const FolderSizeItem &o2)
{
    return o1.lastModified > o2.lastModified;
}

bool typeLessThan(const FolderSizeItem &o1, const FolderSizeItem &o2)
{
    if (o1.isDir && !o2.isDir) {
        // If dir before file, return true.
        return true;
    } else if (!o1.isDir && o2.isDir) {
        // If file before dir, return false;
        return false;
    } else {
        // If both are dir/file, compare type and name.
        if (o1.fileType.toLower() == o2.fileType.toLower()) {
            return o1.name.toLower() < o2.name.toLower();
        } else if (o1.fileType.toLower() < o2.fileType.toLower()) {
            return true;
        } else {
            return false;
        }
    }
}

bool sizeGreaterThan(const FolderSizeItem &o1, const FolderSizeItem &o2)
{
    if (o1.isDir && !o2.isDir) {
        // If dir before file, return true.
        return true;
    } else if (!o1.isDir && o2.isDir) {
        // If file before dir, return false;
        return false;
    } else {
        // If both the same, compare size.
        return o1.size > o2.size;
    }
}
// Harmattan is a linux
#if defined(Q_WS_HARMATTAN)
const QString FolderSizeModelThread::DEFAULT_CURRENT_DIR = "/home/user/MyDocs";
const int FolderSizeModelThread::FILE_READ_BUFFER = 1048576;
const int FolderSizeModelThread::FILE_COPY_DELAY = 50;
const int FolderSizeModelThread::FILE_DELETE_DELAY = 200;
#else
const QString FolderSizeModelThread::DEFAULT_CURRENT_DIR = "E:/";
const int FolderSizeModelThread::FILE_READ_BUFFER = 32768;
const int FolderSizeModelThread::FILE_COPY_DELAY = 50;
const int FolderSizeModelThread::FILE_DELETE_DELAY = 50;
#endif

FolderSizeModelThread::FolderSizeModelThread(QObject *parent) : QThread(parent)
{
    // TODO For testing only.
//#if defined(Q_WS_HARMATTAN)
//    QFile dbFile(CACHE_DB_PATH);
//    if (dbFile.remove()) {
//        qDebug() << "FolderSizeModelThread constructor removed" << CACHE_DB_PATH;
//    } else {
//        qDebug() << "FolderSizeModelThread constructor can't remove" << CACHE_DB_PATH;
//    }
//#elif defined(Q_OS_SYMBIAN)
//    QString privatePathQt(QApplication::applicationDirPath());
//    qDebug() << "FolderSizeModelThread constructor private" << privatePathQt;

//    QFileInfo dbFileInfo(CACHE_DB_PATH);
//    qDebug() << "FolderSizeModelThread constructor dbFileInfo" << dbFileInfo.absoluteFilePath() << dbFileInfo.exists();

//    m_db = QSqlDatabase::addDatabase("QSQLITE", CACHE_DB_CONNECTION_NAME);
//    m_db.setDatabaseName(CACHE_DB_PATH);
//    bool ok = m_db.open();
//    QSqlQuery deleteQuery("DELETE FROM folderpie_cache");
//    if (deleteQuery.exec()) {
//        m_db.commit();
//        qDebug() << "FolderSizeModelThread constructor deleted folderpie_cache." << deleteQuery.size();
//        qDebug() << "FolderSizeModelThread constructor count folderpie_cache." << countDirSizeCacheDB();
//    } else {
//        m_db.rollback();
//        qDebug() << "FolderSizeModelThread constructor can't delete folderpie_cache." << deleteQuery.lastError();
//    }
//    m_db.close();
//#endif

    // Initialize DB queries.
    initializeDB();

//    m_currentDir = DEFAULT_CURRENT_DIR;
//    m_sortFlag = getSortFlagFromDB(m_currentDir, SortByType);
    dirSizeCache = new QHash<QString, FolderSizeItem>();
    m_itemCache = new QHash<QString, FolderSizeItem>();

    // TODO To be implemented which file type to be indexed.
    m_isFileCached = false;
}

FolderSizeModelThread::~FolderSizeModelThread()
{
}

void FolderSizeModelThread::initializeDB()
{
    // Prepare queries.
    m_selectPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_selectPS.prepare("SELECT * FROM folderpie_cache WHERE id = :id");

    m_insertPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_insertPS.prepare("INSERT INTO folderpie_cache(id, name, absolute_path, last_modified, size, is_dir, sub_dir_count, sub_file_count, file_type)"
                       " VALUES (:id, :name, :absolute_path, :last_modified, :size, :is_dir, :sub_dir_count, :sub_file_count, :file_type)");

    m_updatePS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_updatePS.prepare("UPDATE folderpie_cache SET"
                       " name = :name, absolute_path = :absolute_path, last_modified = :last_modified, size = :size,"
                       " is_dir = :is_dir, sub_dir_count = :sub_dir_count, sub_file_count = :sub_file_count, file_type = :file_type"
                       " WHERE id = :id");

    m_deletePS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_deletePS.prepare("DELETE FROM folderpie_cache WHERE id = :id");

    m_countPS = QSqlQuery(DatabaseManager::defaultManager().getDB());
    m_countPS.prepare("SELECT count(*) count FROM folderpie_cache");
}

void FolderSizeModelThread::fixDamagedDB()
{
    QSqlQuery query(DatabaseManager::defaultManager().getDB());
    bool res;

    // Test database table.
    // NOTE QSqlError 10, 11, 19 requires reindex.
    res = query.exec("SELECT count(*) FROM folderpie_cache WHERE id = ''");
    if (res) {
        qDebug() << "FolderSizeModelThread::fixDamagedDB folderpie_cache is valid. Operation is aborted.";
        return;
    } else {
        qDebug() << "FolderSizeModelThread::fixDamagedDB folderpie_cache is malformed. Error" << query.lastError();
    }

    // Drop index to fix QSqlError 11 database disk image is malformed.
    res = query.exec("DROP INDEX folderpie_cache_pk;");
    if (res) {
        qDebug() << "FolderSizeModelThread::fixDamagedDB DROP INDEX is done.";
    } else {
        qDebug() << "FolderSizeModelThread::fixDamagedDB DROP INDEX is failed. Error" << query.lastError();
    }

    res = query.exec("DELETE FROM folderpie_cache WHERE id = ''");
    if (res) {
        qDebug() << "FolderSizeModelThread::fixDamagedDB DELETE item with empty id is done." << query.numRowsAffected();
    } else {
        qDebug() << "FolderSizeModelThread::fixDamagedDB DELETE is failed. Error" << query.lastError() << query.lastQuery();
    }

    // TODO Workaround. Delete duplicated unique item to fix QSqlError 19 indexed columns are not unique.
    QSqlQuery deleteDuplicatedPS(DatabaseManager::defaultManager().getDB());
    deleteDuplicatedPS.prepare("DELETE FROM folderpie_cache WHERE id = :id AND rowid < :rowid;");

    res = query.exec("SELECT id, count(*) c, max(rowid) max_rowid FROM folderpie_cache GROUP BY id HAVING count(*) > 1;");
    if (res) {
        qDebug() << "FolderSizeModelThread::fixDamagedDB find duplicated unique key. numRowsAffected" << query.numRowsAffected();
        QSqlRecord rec = query.record();
        while (query.next()) {
            // Process pending events.
            QApplication::processEvents();

            if (query.isValid()) {
                QString id = query.value(rec.indexOf("id")).toString();
                int c = query.value(rec.indexOf("c")).toInt();
                int maxRowId = query.value(rec.indexOf("max_rowid")).toInt();
                qDebug() << "FolderSizeModelThread::fixDamagedDB find duplicated unique key record" << id << c << maxRowId;

                // Delete duplicated unique key record.
                deleteDuplicatedPS.bindValue(":id", id);
                deleteDuplicatedPS.bindValue(":rowid", maxRowId);
                res = deleteDuplicatedPS.exec();
                if (res) {
                    qDebug() << "FolderSizeModelThread::fixDamagedDB DELETE duplicated unique key record is done." << deleteDuplicatedPS.numRowsAffected();
                } else {
                    qDebug() << "FolderSizeModelThread::fixDamagedDB DELETE duplicated unique key record is failed. Error" << deleteDuplicatedPS.lastError() << deleteDuplicatedPS.lastQuery();
                }
            } else {
                qDebug() << "FolderSizeModelThread::fixDamagedDB find duplicated unique key record position is invalid. ps.lastError()" << query.lastError();
            }
        }
    } else {
        qDebug() << "FolderSizeModelThread::fixDamagedDB find duplicated unique key failed. Error" << query.lastError() << query.lastQuery();
    }

    // Process pending events.
    QApplication::processEvents();
}

FolderSizeItem FolderSizeModelThread::selectDirSizeCacheFromDB(const QString id)
{
    FolderSizeItem item;
    bool res;

    // Find in itemCache, return if found.
    if (m_itemCache->contains(id)) {
        item = m_itemCache->value(id);
//        qDebug() << "FolderSizeModelThread::selectDirSizeCacheFromDB cached id" << id << "item" << item;
        return item;
    }

    m_selectPS.bindValue(":id", id);
    res = m_selectPS.exec();
    if (res && m_selectPS.first()) {
//        qDebug() << "FolderSizeModelThread::selectDirSizeCacheFromDB id" << id << "is found. id" << m_selectPS.value(rec.indexOf("id")).toString() << "size" << m_selectPS.value(rec.indexOf("size")).toLongLong();
        QSqlRecord rec = m_selectPS.record();
        item.name = m_selectPS.value(rec.indexOf("name")).toString();
        item.absolutePath = m_selectPS.value(rec.indexOf("absolute_path")).toString();
        item.lastModified = m_selectPS.value(rec.indexOf("last_modified")).toDateTime();
        item.size = m_selectPS.value(rec.indexOf("size")).toLongLong();
        item.isDir = m_selectPS.value(rec.indexOf("is_dir")).toBool();
        item.subDirCount = m_selectPS.value(rec.indexOf("sub_dir_count")).toInt();
        item.subFileCount = m_selectPS.value(rec.indexOf("sub_file_count")).toInt();
        item.fileType = m_selectPS.value(rec.indexOf("file_type")).toString();
        qDebug() << "FolderSizeModelThread::selectDirSizeCacheFromDB id" << id << "item" << item;

        // Insert item to itemCache.
        if (item.absolutePath == id) {
            m_itemCache->insert(id, item);
        }
    } else {
        qDebug() << "FolderSizeModelThread::selectDirSizeCacheFromDB id" << id << " is not found. error" << m_selectPS.lastError();
    }

    return item;
}

int FolderSizeModelThread::insertDirSizeCacheToDB(const FolderSizeItem item)
{
    bool res;

    m_insertPS.bindValue(":id", item.absolutePath);
    m_insertPS.bindValue(":name", item.name);
    m_insertPS.bindValue(":absolute_path", item.absolutePath);
    m_insertPS.bindValue(":last_modified", item.lastModified);
    m_insertPS.bindValue(":size", item.size);
    m_insertPS.bindValue(":is_dir", (item.isDir ? 1 : 0));
    m_insertPS.bindValue(":sub_dir_count", item.subDirCount);
    m_insertPS.bindValue(":sub_file_count", item.subFileCount);
    m_insertPS.bindValue(":file_type", item.fileType);
    res = m_insertPS.exec();
    if (res) {
        qDebug() << "FolderSizeModelThread::insertDirSizeCacheToDB insert done" << item << "res" << res << "numRowsAffected" << m_insertPS.numRowsAffected();
        DatabaseManager::defaultManager().getDB().commit();

        // Insert item to itemCache.
        m_itemCache->insert(item.absolutePath, item);
    } else {
        qDebug() << "FolderSizeModelThread::insertDirSizeCacheToDB insert failed" << item << "res" << res << m_insertPS.lastError();
    }

    return m_insertPS.numRowsAffected();
}

int FolderSizeModelThread::updateDirSizeCacheToDB(const FolderSizeItem item)
{
    bool res;

    m_updatePS.bindValue(":id", item.absolutePath);
    m_updatePS.bindValue(":name", item.name);
    m_updatePS.bindValue(":absolute_path", item.absolutePath);
    m_updatePS.bindValue(":last_modified", item.lastModified);
    m_updatePS.bindValue(":size", item.size);
    m_updatePS.bindValue(":is_dir", (item.isDir ? 1 : 0));
    m_updatePS.bindValue(":sub_dir_count", item.subDirCount);
    m_updatePS.bindValue(":sub_file_count", item.subFileCount);
    m_updatePS.bindValue(":file_type", item.fileType);
    res = m_updatePS.exec();
    if (res) {
        qDebug() << "FolderSizeModelThread::updateDirSizeCacheToDB update done" << item << "res" << res << "numRowsAffected" << m_updatePS.numRowsAffected();
        DatabaseManager::defaultManager().getDB().commit();

        // Insert item to itemCache.
        m_itemCache->insert(item.absolutePath, item);
    } else {
        qDebug() << "FolderSizeModelThread::updateDirSizeCacheToDB update failed" << item << "res" << res << m_updatePS.lastError();
    }

    return m_updatePS.numRowsAffected();
}

int FolderSizeModelThread::updateDirSizeCacheTreeToDB(const QString oldAbsPath, const QString newAbsPath)
{
    bool res;

    QSqlQuery m_updateTreePS(DatabaseManager::defaultManager().getDB());
    m_updateTreePS.prepare("UPDATE folderpie_cache SET"
                       " absolute_path = REPLACE(absolute_path, :old_absolute_path, :new_absolute_path),"
                       " id = REPLACE(id, :old_id, :new_id)"
                       " WHERE id = :old_id1 OR id LIKE :old_id2||'/%'");
    m_updateTreePS.bindValue(":old_absolute_path", oldAbsPath);
    m_updateTreePS.bindValue(":new_absolute_path", newAbsPath);
    m_updateTreePS.bindValue(":old_id", oldAbsPath);
    m_updateTreePS.bindValue(":new_id", newAbsPath);
    m_updateTreePS.bindValue(":old_id1", oldAbsPath);
    m_updateTreePS.bindValue(":old_id2", oldAbsPath);
    res = m_updateTreePS.exec();
    if (res) {
        qDebug() << "FolderSizeModelThread::updateDirSizeCacheTreeToDB update done" << oldAbsPath << "res" << res << "numRowsAffected" << m_updateTreePS.numRowsAffected();
        DatabaseManager::defaultManager().getDB().commit();

        // Remove source item from itemCache.
        m_itemCache->remove(oldAbsPath);
    } else {
        qDebug() << "FolderSizeModelThread::updateDirSizeCacheTreeToDB update failed" << oldAbsPath << "res" << res << m_updateTreePS.lastError();
    }

    return m_updateTreePS.numRowsAffected();
}

int FolderSizeModelThread::deleteDirSizeCacheToDB(const QString id)
{
    bool res;

    m_deletePS.bindValue(":id", id);
    res = m_deletePS.exec();
    if (res) {
        qDebug() << "FolderSizeModelThread::deleteDirSizeCacheToDB delete done" << id << "res" << res << "numRowsAffected" << m_deletePS.numRowsAffected();
        DatabaseManager::defaultManager().getDB().commit();

        // Remove item from itemCache.
        m_itemCache->remove(id);
    } else {
        qDebug() << "FolderSizeModelThread::deleteDirSizeCacheToDB delete failed" << id << "res" << res << m_deletePS.lastError();
    }

    return m_deletePS.numRowsAffected();
}

int FolderSizeModelThread::deleteDirSizeCacheTreeToDB(const QString id)
{
    bool res;

    QSqlQuery m_deleteTreePS(DatabaseManager::defaultManager().getDB());
    m_deleteTreePS.prepare("DELETE FROM folderpie_cache WHERE id like :id||'/%'");
    m_deleteTreePS.bindValue(":id", id);
    res = m_deleteTreePS.exec();
    if (res) {
        qDebug() << "FolderSizeModelThread::deleteDirSizeCacheTreeToDB delete done" << id << "res" << res << "numRowsAffected" << m_deleteTreePS.numRowsAffected();
        DatabaseManager::defaultManager().getDB().commit();

        // Remove item to itemCache.
        m_itemCache->remove(id);
    } else {
        qDebug() << "FolderSizeModelThread::deleteDirSizeCacheTreeToDB delete failed" << id << "res" << res << m_deleteTreePS.lastError();
    }

    return m_deleteTreePS.numRowsAffected();
}

int FolderSizeModelThread::countDirSizeCacheDB()
{
    int c = 0;
    bool res = m_countPS.exec();
    if (res) {
        if (m_countPS.next()) {
            c = m_countPS.value(m_countPS.record().indexOf("count")).toInt();
            qDebug() << "FolderSizeModelThread::countDirSizeCacheDB res" << res << "count" << c;
        }
    } else {
        qDebug() << "FolderSizeModelThread::countDirSizeCacheDB res" << res << "error" << m_countPS.lastError();
    }

    return c;
}

QString FolderSizeModelThread::currentDir() const
{
    return m_currentDir;
}

void FolderSizeModelThread::cleanItems()
{
    foreach (FolderSizeItem item, dirSizeCache->values()) {
        cleanItem(item);
    }
}

bool FolderSizeModelThread::cleanItem(const FolderSizeItem &item)
{
    //    qDebug() << "FolderSizeModelThread::cleanItem item localPath" << item.localPath << "remotePath" << item.remotePath << "type" << item.type << "uid" << item.uid << "hash" << item.hash;

    QFileInfo info(item.absolutePath);
    if (!info.exists()) {
        qDebug() << "FolderSizeModelThread::cleanItem remove item absolutePath" << item.absolutePath << "isDir" << item.isDir << "size" << item.size << "subFileCount" << item.subFileCount << "subDirCount" << item.subDirCount << "lastModified" << item.lastModified.toString(Qt::ISODate);
        dirSizeCache->remove(item.absolutePath);
        return true;
    } else if (item.absolutePath.startsWith(":") || item.absolutePath == "") {
        // TODO override remove for unwanted item.
        qDebug() << "FolderSizeModelThread::cleanItem remove item absolutePath" << item.absolutePath << "isDir" << item.isDir << "size" << item.size << "subFileCount" << item.subFileCount << "subDirCount" << item.subDirCount << "lastModified" << item.lastModified.toString(Qt::ISODate);
        dirSizeCache->remove(item.absolutePath);
        return true;
    }
    return false;
}

bool FolderSizeModelThread::move(const QString sourcePath, const QString targetPath) {
    // Try rename(move) first, then copy/delete if it failed.
    QFileInfo sourceFileInfo(sourcePath);
    qDebug() << "FolderSizeModelThread::move sourceFileInfo" << sourceFileInfo.absoluteFilePath() << "isDir" << sourceFileInfo.isDir() << "isFile" << sourceFileInfo.isFile() << "exists" << sourceFileInfo.exists();
    FolderSizeItem sourceItem = getItem(sourceFileInfo);

    if (sourceFileInfo.absoluteDir().rename(sourcePath, targetPath)) {
        qDebug() << "FolderSizeModelThread::move Moving is done. sourceFile" << sourcePath << "targetFile" << targetPath;

        // TODO include total size in signal.
        emit copyStarted(MoveFile, sourcePath, targetPath, "", 0);

        // TODO getItem return default object with incorrect size, count.
        qDebug() << "FolderSizeModelThread::move sourceItem" << sourceItem;

        // TODO Update cache tree from sourcePath to targetPath.
        updateDirSizeCacheTreeToDB(sourcePath, targetPath);

        // Emit copy progress.
        emit copyProgress(MoveFile, sourcePath, targetPath, sourceItem.size, sourceItem.size);

        // Emit copy finish with count.
        bool isSourceRoot = (sourcePath == m_sourcePath);
        emit copyFinished(MoveFile, sourcePath, targetPath, tr("Move %1 to %2 is done successfully.").arg(sourcePath).arg(targetPath),
                          0, sourceItem.size, sourceItem.size, (1 + sourceItem.subDirCount + sourceItem.subFileCount), isSourceRoot);

        return true;
    } else {
        return false;
    }
}

bool FolderSizeModelThread::copy(int method, const QString sourcePath, const QString targetPath)
{
    // targetPath is always actual file/folder name, not parent folder.
    qDebug() << "FolderSizeModelThread::copy method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath;

    bool isSourceRoot = (sourcePath == m_sourcePath);

    if (m_abortFlag) {
        emit copyFinished(method, sourcePath, targetPath,
                          (method == CopyFile) ? tr("Copy %1 to %2 is aborted.").arg(sourcePath).arg(targetPath) : tr("Move %1 to %2 is aborted.").arg(sourcePath).arg(targetPath),
                          -2, 0, -1, 0, isSourceRoot);
        return false;
    }

    QFileInfo sourceFileInfo(sourcePath);
    QFileInfo targetFileInfo(targetPath);
    qint64 itemSize = getItem(sourceFileInfo).size;
    bool res = false;

    // Copy dir.
    if (sourceFileInfo.isDir()) {
        // TODO include total size in signal.
        emit copyStarted(method, sourcePath, targetPath, "", 0);

        // Create dir on targetPath.
        if (!targetFileInfo.exists()) {
            if (!targetFileInfo.dir().mkdir(targetFileInfo.fileName())) {
                qDebug() << "FolderSizeModelThread::copy can't create folder" << targetFileInfo.absoluteFilePath() << "It already exists.";
            } else {
                // Create folder successfully, result = true;
                res = true;

                // Emit copy progress.
                emit copyProgress(method, sourcePath, targetPath, targetFileInfo.size(), itemSize);
            }
        }

        // List sourceDir's contents. Then invoke copy recursively.
        QDir dir(sourceFileInfo.absoluteFilePath());
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        foreach (QFileInfo item, dir.entryInfoList()) {
            // Break if aborted.
            if (m_abortFlag) {
                return false;
            }

            res = copy(method, item.absoluteFilePath(), QDir(targetFileInfo.absoluteFilePath()).absoluteFilePath(item.fileName()) );

            // TODO commented below line to always resume even there is any failure.
            //            // Break if it returns false.
            //            if (!res) break;
        }

        // Emit signal as dir copying is finished.
        if (res) {
            qDebug() << "FolderSizeModelThread::copy method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath << "is done.";

            // TODO Copy cache (both DAT and DB) for targetPath.
            // Postpone caching to be done by refreshDir().
//            FolderSizeItem cachedItem = getCachedDir(sourceFileInfo);
//            cachedItem.name = targetFileInfo.fileName();
//            cachedItem.absolutePath = targetFileInfo.absoluteFilePath();
//            insertDirSizeCacheToDB(cachedItem);
//            m_itemCache->insert(cachedItem.absolutePath, cachedItem);

            emit copyFinished(method, sourcePath, targetPath,
                              (method == CopyFile) ? tr("Copy %1 to %2 is done successfully.").arg(sourcePath).arg(targetPath) : tr("Move %1 to %2 is done successfully.").arg(sourcePath).arg(targetPath),
                              0, 0, itemSize, 1, isSourceRoot);
        } else {
            qDebug() << "FolderSizeModelThread::copy method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath << "is failed. err = -1";
            emit copyFinished(method, sourcePath, targetPath,
                              (method == CopyFile) ? tr("Copy %1 to %2 is failed.").arg(sourcePath).arg(targetPath) : tr("Move %1 to %2 is failed.").arg(sourcePath).arg(targetPath),
                              -1, 0, itemSize, 0, isSourceRoot);
        }
    } else {
        // Method copyFile. Method copyFile will emit copyFinished once it's done.
        res = copyFile(method, sourcePath, targetPath);
    }

    // TODO move to copyFinishedFilter
    //    if (res) {
    //        // Remove cache.
    //        removeDirSizeCache(targetPath);
    //    }

    return res;
}

bool FolderSizeModelThread::copyFile(int method, const QString sourcePath, const QString targetPath)
{
    emit copyStarted(method, sourcePath, targetPath, "", 0);

//    qDebug() << "FolderSizeModelThread::copyFile method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath;

    bool isSourceRoot = (sourcePath == m_sourcePath);

    // Return false if aborted.
    if (m_abortFlag) {
        emit copyFinished(method, sourcePath, targetPath,
                          (method == CopyFile) ? tr("Copy %1 to %2 is aborted.").arg(sourcePath).arg(targetPath) : tr("Move %1 to %2 is aborted.").arg(sourcePath).arg(targetPath),
                          -2, 0, -1, 0, isSourceRoot);
        return false;
    }

    QFileInfo sourceFileInfo(sourcePath);
    QFileInfo targetFileInfo(targetPath);
    QString sourceAbsFilePath = sourcePath;
    QString targetAbsFilePath;
    qint64 itemSize = sourceFileInfo.size();
    qint64 totalBytes = 0;
    bool res = false;

    // Verify targetPath is file or folder. It will be prepared to file path.
    if (targetFileInfo.isDir()) {
        targetAbsFilePath = QDir(targetFileInfo.absoluteFilePath()).absoluteFilePath(sourceFileInfo.fileName());
    } else {
        targetAbsFilePath = targetPath;
    }

    qDebug() << "FolderSizeModelThread::copyFile method" << method << "sourceAbsFilePath" << sourceAbsFilePath << "targetAbsFilePath" << targetAbsFilePath;

    if (sourceAbsFilePath == targetAbsFilePath) {
        qDebug() << "FolderSizeModelThread::copyFile Error sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath;
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, tr("Both source/target path can't be the same file."), -2, 0, itemSize, 0, isSourceRoot);
        return false;
    }

#ifdef USE_SYSTEM_COPY
    // Copy with bundled method QFile::copy() may consume less CPU power.

    res = QFile::copy(sourcePath, targetPath);
    if (res) {
        totalBytes = (res ? itemSize : 0);

        // Emit copy progress.
        emit copyProgress(method, sourcePath, targetPath, totalBytes, itemSize);
    }
    res = res && !m_abortFlag;
#else
    // Copy with feedback progress to QML.

    QFile sourceFile(sourceAbsFilePath);
    QFile targetFile(targetAbsFilePath);

    if (sourceFile.open(QFile::ReadOnly) && targetFile.open(QFile::WriteOnly)) {
        char buf[FolderSizeModelThread::FILE_READ_BUFFER];
        qint64 c = sourceFile.read(buf, sizeof(buf));
        while (c > 0 && !m_abortFlag) {
            targetFile.write(buf, c);
            totalBytes += c;

            // Emit copy progress.
            emit copyProgress(method, sourceAbsFilePath, targetAbsFilePath, totalBytes, sourceFile.size());

            // Delay to update UI.
            delay(FILE_COPY_DELAY);

            // Read next buffer.
//            qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << "FolderSizeModelThread::copyFile before read";
            c = sourceFile.read(buf, sizeof(buf));
//            qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << "FolderSizeModelThread::copyFile after read" << c;
        }

        qDebug() << "FolderSizeModelThread::copyFile done method" << method << "sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath << "totalBytes" << totalBytes;
        res = !m_abortFlag;
    } else {
        qDebug() << "FolderSizeModelThread::copyFile Error method" << method << "sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath;
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, tr("Both source %1 and target %2 can't be read/written.").arg(sourceAbsFilePath).arg(targetAbsFilePath), -3, totalBytes, itemSize, 0, isSourceRoot);
        return false;
    }

    // Close files.
    sourceFile.close();
    targetFile.close();
#endif

    // Return false if aborted.
    if (m_abortFlag) {
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath,
                          (method == CopyFile) ? tr("Copy %1 to %2 is aborted.").arg(sourceAbsFilePath).arg(targetAbsFilePath) : tr("Move %1 to %2 is aborted.").arg(sourceAbsFilePath).arg(targetAbsFilePath),
                          -4, totalBytes, itemSize, 0, isSourceRoot);
        return false;
    }

    // Emit signal as finish.
    if (res) {
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath,
                          (method == CopyFile) ? tr("Copy %1 to %2 is done successfully.").arg(sourceAbsFilePath).arg(targetAbsFilePath) : tr("Move %1 to %2 is done successfully.").arg(sourceAbsFilePath).arg(targetAbsFilePath),
                          0, totalBytes, itemSize, 1, isSourceRoot);
    } else {
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath,
                          (method == CopyFile) ? tr("Copy %1 to %2 is failed.").arg(sourceAbsFilePath).arg(targetAbsFilePath) : tr("Move %1 to %2 is failed.").arg(sourceAbsFilePath).arg(targetAbsFilePath),
                          -1, totalBytes, itemSize, 0, isSourceRoot);
    }

    // Sleep after emit signal.
    QApplication::processEvents();

    return res;
}

void FolderSizeModelThread::delay(const int interval)
{
#ifdef Q_WS_HARMATTAN
//    QApplication::sendPostedEvents();
//    msleep(interval);
    QApplication::processEvents(QEventLoop::AllEvents, interval);
#else
//    QApplication::sendPostedEvents();
//    msleep(interval);
    QApplication::processEvents(QEventLoop::AllEvents, interval);
#endif
}

bool FolderSizeModelThread::deleteDir(const QString sourcePath, bool suppressSignal, bool forceDeleteReadOnly)
{
    qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath;

    if (!suppressSignal) emit deleteStarted(m_runMethod, sourcePath);

    // Return false if aborted.
    if (m_abortFlag) {
        return false;
    }

    bool res = true;
    QFileInfo sourceFileInfo(sourcePath);

    // Delete dir.
    if (sourceFileInfo.isDir()) {
        // List sourceDir's contents. Then delete recursively.
        QDir dir(sourcePath);
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        foreach (QFileInfo item, dir.entryInfoList()) {
            res = res && deleteDir(item.absoluteFilePath(), suppressSignal, forceDeleteReadOnly);

            if (!res) break;
        }

        // Return false if aborted.
        if (m_abortFlag) {
            return false;
        }

        // Delete dir.
        if (res) {
            res = res && sourceFileInfo.dir().rmdir(sourceFileInfo.fileName());
            // Force delete if enabled.
            if (forceDeleteReadOnly && !res) {
                if (setFileAttribute(sourcePath, ReadOnly, false))
                    res = sourceFileInfo.dir().rmdir(sourceFileInfo.fileName());
            }
        }
    } else {
        // Delete file.
        QFile sourceFile(sourcePath);
        res = sourceFile.remove();
        // Force delete if enabled.
        if (forceDeleteReadOnly && !res) {
            if (setFileAttribute(sourcePath, ReadOnly, false))
                res = sourceFile.remove();
        }
    }

    if (!res) {
        qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath << "failed.";
        if (!suppressSignal) emit deleteProgress(m_runMethod, sourcePath, tr("Deleting %1 is failed.").arg(sourceFileInfo.fileName()), -1);
    } else {
        // TODO Move to emit only once per thread call.
        qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath << "done.";
        if (!suppressSignal) emit deleteProgress(m_runMethod, sourcePath, tr("Deleting %1 is done.").arg(sourceFileInfo.fileName()), 0);

        // Move to deleteFinishedFilter
        //        // Remove cache up to parent.
        //        removeDirSizeCache(sourcePath);
    }

    // This sleep is a must.
    // Sleep for process deleteFinished signal.
    delay(FILE_DELETE_DELAY);

    return res;
}

bool FolderSizeModelThread::trash(const QString sourcePath, const QString targetPath)
{
    // Move to trash only.
    // TODO Does it need to support copy? Ex. trash C: file/folder.
    if (!QFileInfo(sourcePath).exists()) {
        qDebug() << "FolderSizeModelThread::trash not found sourcePath" << sourcePath << "Operation is ignored.";
        return true;
    }

    if (QFileInfo(targetPath).exists()) {
        qDebug() << "FolderSizeModelThread::trash found existing targetPath" << targetPath << "It's being deleted.";
        deleteDir(targetPath, true);
    }

    if (QFileInfo(sourcePath).absoluteDir().rename(sourcePath, targetPath)) {
        return true;
    } else {
        return false;
    }
}

bool FolderSizeModelThread::emptyDir(const QString sourcePath)
{
    QDir dir(sourcePath);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    bool res = true;
    foreach (QString childName, dir.entryList()) {
        QApplication::processEvents();

        QString childPath = dir.absoluteFilePath(childName);
        qDebug() << "FolderSizeModelThread::emptyDir delete childPath" << childPath;

        if (!deleteDir(childPath, true, m_settings.value("FolderSizeModelThread.forceDeleteReadOnly.enabled", false).toBool())) {
            res = false;
        }
    }

    return res;
}

bool FolderSizeModelThread::isDirSizeCacheExisting()
{
    // Return true if cache is not empty or cache is loading.
//    qDebug() << "FolderSizeModelThread::isDirSizeCacheExisting isRunning(LoadDirSizeCache)" << (isRunning() && m_runMethod == LoadDirSizeCache) << "dirSizeCache" << (dirSizeCache->count()) << "countDirSizeCacheDB" << (countDirSizeCacheDB());
    return (dirSizeCache->count() > 0 || countDirSizeCacheDB() > 0);
}

int FolderSizeModelThread::removeDirSizeCache(const QString key)
{
    //    qDebug() << "FolderSizeModelThread::removeDirSizeCache started from" << key;

    int res = 0;

    // Remove only specified key.
    res = dirSizeCache->remove(key);

    // Dirty cache.
    FolderSizeItem item = selectDirSizeCacheFromDB(key);
    if (item.absolutePath == key && item.lastModified != QDateTime::fromMSecsSinceEpoch(0)) {
        item.isDir = true;
        item.lastModified = QDateTime::fromMSecsSinceEpoch(0);
        res += updateDirSizeCacheToDB(item);
    }

    return res;
}

FolderSizeItem FolderSizeModelThread::getCachedDir(const QFileInfo dir, const bool clearCache) {
    double dirSize = 0;
    double subDirCount = 0;
    double subFileCount = 0;
    FolderSizeItem cachedDir;
    bool isFound = false;
    bool isValid = true;
    bool isInDB = false;

    // Get cached dirSize if any and clearCache==false
    if (!clearCache && (dir.isDir() || m_isFileCached) ) {
        // Get cachedDir from DB.
        cachedDir = selectDirSizeCacheFromDB(dir.absoluteFilePath());
        if (!isFound && cachedDir.absolutePath == dir.absoluteFilePath()) {
            isFound = true;
            if (cachedDir.lastModified >= dir.lastModified()) {
                // cachedDir is still valid, just return it.
//                qDebug() << QTime::currentTime() << QString("FolderSizeModelThread::getCachedDir DB %1 >= %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
                isInDB = true;
            } else {
                qDebug() << QTime::currentTime() << QString("FolderSizeModelThread::getCachedDir DB %1 < %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
                isValid = false;
            }
        } else if (cachedDir.absolutePath == "") {
            // TODO Clean invalid entry.
            isFound = false;
        }

        // Get cachedDir from DAT.
        if (!isFound && dirSizeCache->contains(dir.absoluteFilePath())) {
            //        qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir found " + dir.absoluteFilePath();
            cachedDir = dirSizeCache->value(dir.absoluteFilePath());
            if (cachedDir.lastModified >= dir.lastModified()) {
                // cachedDir is still valid, just return it.
//                qDebug() << QTime::currentTime() << QString("FolderSizeModelThread::getCachedDir %1 >= %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
                isFound = true;
            } else {
                qDebug() << QTime::currentTime() << QString("FolderSizeModelThread::getCachedDir %1 < %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
                isValid = false;
            }
        }
    }

    // If (cache is invalidated or not found) and FolderSizeModelThread is ready, get cache recursively.
    // Otherwise return dummy dir item with 0 byte.
    if (!isFound || !isValid || dir.isRoot()) {
//        qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir NOT found " + dir.absoluteFilePath();

        // Avoid UI freezing by processEvents().
        QApplication::processEvents();

        if (dir.isDir()) {
            QDir d = QDir(dir.absoluteFilePath());
            d.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
            QFileInfoList subList = d.entryInfoList();

            for (int i = 0; i < subList.size(); ++i) {
                QFileInfo fileInfo = subList.at(i);
                QString fileName = fileInfo.fileName();

                // Avoid UI freezing by processEvents().
                QApplication::processEvents();

                FolderSizeItem item;
                if (fileName == "." || fileName == "..") {
                    // do nothing.
                } else if (fileInfo.isDir()) {
                    // drilldown to get dir tree size.
                    item = getCachedDir(fileInfo, clearCache);
                    double subDirSize = item.size;
                    dirSize += subDirSize;
                    subDirCount++;
                    // Count its sub dirs/files too.
                    subDirCount += item.subDirCount;
                    subFileCount += item.subFileCount;
                } else {
                    item = getCachedDir(fileInfo, clearCache);
                    dirSize += fileInfo.size();
                    subFileCount++;

                    // Emit signal on file is count.
                    emit fetchDirSizeUpdated(fileInfo.absoluteFilePath());
                }

                //            qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir path" << fileInfo.absoluteFilePath() << "dirSize" << dirSize << "subDirCount" << subDirCount << "subFileCount" << subFileCount;
            }

            // Put calculated dirSize to cache.
            cachedDir = FolderSizeItem(dir.fileName(), dir.absoluteFilePath(), dir.lastModified(), dirSize + dir.size(), true, subDirCount, subFileCount);
    //        dirSizeCache->insert(dir.absoluteFilePath(), cachedDir);
        } else {
            // Cache file.
            cachedDir = FolderSizeItem(dir.fileName(), dir.absoluteFilePath(), dir.lastModified(), dir.size(), false, 0, 0);
        }
    }

    //    // Emit signal on dir is cached.
    //    emit fetchDirSizeUpdated(dir.absoluteFilePath());

    //    qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir done " + cachedDir.name + ", " + QString("%1").arg(cachedDir.size);

    // TODO Persists to DB.
    if (dir.isDir() || m_isFileCached) {
        if (!isFound || !isValid || !isInDB) {
            if (!isFound) {
                if (insertDirSizeCacheToDB(cachedDir) <= 0) {
                    // Avoid UI freezing by processEvents().
                    QApplication::processEvents();

                    // Workaround. In case it can't be inserted, try update.
                    updateDirSizeCacheToDB(cachedDir);
                }
            } else {
                updateDirSizeCacheToDB(cachedDir);
            }

            // TODO Remove from dirSizeCache after migrate to DB.
            dirSizeCache->remove(cachedDir.absolutePath);
            qDebug() << "FolderSizeModelThread::getCachedDir remove from dirSizeCache" << cachedDir << "dirSizeCache->count()" << dirSizeCache->count();
        }
    } else {
//        qDebug() << "FolderSizeModelThread::getCachedDir m_isFileCached" << m_isFileCached << "skip update cache" << cachedDir;
    }

    return cachedDir;
}

void FolderSizeModelThread::sortItemList(QList<FolderSizeItem> &itemList, bool sortAll) {
    switch (m_sortFlag) {
    case SortByName:
        qSort(itemList.begin(), itemList.end(), nameLessThan);
        break;
    case SortByNameWithDirectoryFirst:
        qSort(itemList.begin(), itemList.end(), nameLessThanWithDirectoryFirst);
        break;
    case SortByTime:
        qSort(itemList.begin(), itemList.end(), timeGreaterThan);
        break;
    case SortByType:
        qSort(itemList.begin(), itemList.end(), typeLessThan);
        break;
    case SortBySize:
        int i;
        for (i = 0; i < itemList.size(); i++) {
            if (!itemList.at(i).isDir) {
                break;
            }
        }

        qDebug() << "FolderSizeModelThread::sortItemList sortBySize file start from" << i << "itemList.size" << itemList.size();
        if (i == 0 || sortAll) {
            qSort(itemList.begin(), itemList.end(), sizeGreaterThan);
        } else {
            QList<FolderSizeItem> dirList = itemList.mid(0, i);
            QList<FolderSizeItem> fileList = itemList.mid(i);
            qSort(dirList.begin(), dirList.end(), sizeGreaterThan);
            itemList.clear();
            itemList.append(dirList);
            itemList.append(fileList);
        }
        qDebug() << "FolderSizeModelThread::sortItemList sortBySize file start from" << i << "sorted itemList.size" << itemList.size();
        break;
    }
}

void FolderSizeModelThread::getDirContent(const QString dirPath, QList<FolderSizeItem> &itemList)
{
    // TODO Issue: so many files caused application crash.

    // TODO Ignore if dirPath = "";
    if (dirPath.trimmed() == "") {
        qDebug() << "FolderSizeModelThread::getDirContent dirPath" << dirPath << "Ignore and return.";
        return;
    }

    // Get only contents in directory (not recursive).
    QDir dir;
    dir = QDir(dirPath);
    if (m_settings.value("FolderSizeModelThread.getDirContent.showHiddenSystem.enabled", false).toBool()) {
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    } else {
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    }
    dir.setNameFilters(m_nameFilters);
    switch (m_sortFlag) {
    case SortByName:
        dir.setSorting(QDir::Name | QDir::IgnoreCase);
        break;
    case SortByNameWithDirectoryFirst:
        dir.setSorting(QDir::Name | QDir::IgnoreCase | QDir::DirsFirst);
        break;
    case SortBySize:
        dir.setSorting(QDir::Size | QDir::DirsFirst);
        break;
    case SortByTime:
        dir.setSorting(QDir::Time);
        break;
    case SortByType:
        dir.setSorting(QDir::Type | QDir::IgnoreCase | QDir::DirsFirst);
        break;
    }

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        if (i % 10 == 1) {
            // Process events i = 1,11,21,...
            QApplication::processEvents();
        }

        QFileInfo fileInfo = list.at(i);
        QString fileName = fileInfo.fileName();
        FolderSizeItem childItem;

        //        qDebug() << "FolderSizeModelThread::getDirContent fileInfo.absoluteFilePath()" << fileInfo.absoluteFilePath();

        if (fileName == ".") {
            // skip current directory '.'
            continue;
        } else if (fileName == "..") {
            // skip current directory '.'
            continue;
        } else if (fileInfo.isDir()) {
            // If running, return cache or dummy item.
            // If not running, drilldown to get dir content.
            if (isRunning()) {
                childItem = getDirItem(fileInfo);
                childItem.isHidden = fileInfo.isHidden();
                childItem.isReadOnly = !fileInfo.isWritable();
                itemList.append(childItem);
            } else {
                childItem = getCachedDir(fileInfo);
                childItem.isHidden = fileInfo.isHidden();
                childItem.isReadOnly = !fileInfo.isWritable();
                itemList.append(childItem);
            }
        } else {
            childItem = getCachedDir(fileInfo);
            childItem.isHidden = fileInfo.isHidden();
            childItem.isReadOnly = !fileInfo.isWritable();
            itemList.append(childItem);
        }
    }

    // Sort itemList if sortFlag == SortBySize. To sort folder size.
    if (m_sortFlag == SortBySize) {
        int i;
        for (i = 0; i < itemList.size(); i++) {
            if (!itemList.at(i).isDir) {
                break;
            }
        }
        if (i > 0) {
            // Sort only if there are mixed dir(s) and files.
            sortItemList(itemList, false);
        }
    }
}

void FolderSizeModelThread::fetchDirSize(const QString startPath, const bool clearCache) {
    // Fetch to cache only. No need to populate to itemList.
    QString sourcePath = (startPath.isEmpty()) ? currentDir() : startPath;
    qDebug() << QTime::currentTime() << "FolderSizeModelThread::fetchDirSize started sourcePath" << sourcePath << "clearCache" << clearCache;

    emit fetchDirSizeStarted(startPath);

    QFileInfo dirInfo(sourcePath);
    getCachedDir(dirInfo, clearCache);

    // Reset after fetch.
    m_clearCache = false;

    emit fetchDirSizeFinished(startPath);

    qDebug() << QTime::currentTime() << "FolderSizeModelThread::fetchDirSize done sourcePath" << sourcePath;
}

FolderSizeItem FolderSizeModelThread::getItem(const QFileInfo fileInfo) {
    if (fileInfo.isDir()) {
        return getDirItem(fileInfo);
    } else if (fileInfo.isFile()){
        return getFileItem(fileInfo);
    } else {
        return FolderSizeItem();
    }
}

QString FolderSizeModelThread::getSourcePath()
{
    return m_sourcePath;
}

FolderSizeItem FolderSizeModelThread::getFileItem(const QFileInfo fileInfo) {
    FolderSizeItem item;

    // If fileCached is enabled, then try get cache from DB.
    // Otherwise skip and return default item.
    if (m_isFileCached) {
        item = selectDirSizeCacheFromDB(fileInfo.absoluteFilePath());
    }

    if (item.absolutePath == fileInfo.absoluteFilePath()) {
        // Get cache from DB.
        // If found, do nothing and return found item.
    } else if (dirSizeCache->contains(fileInfo.absoluteFilePath())) {
        // Get cache from DAT.
        item = dirSizeCache->value(fileInfo.absoluteFilePath());
    } else {
        // FileItem always get isDir=false, subDirCount=0 and subFileCount=0
        item = FolderSizeItem(fileInfo.fileName(),
                            fileInfo.absoluteFilePath(),
                            fileInfo.lastModified(),
                            fileInfo.size(),
                            false, 0, 0);
        // TODO Emit signal to queue refreshDir job.
    }

    return item;
}

FolderSizeItem FolderSizeModelThread::getDirItem(const QFileInfo dirInfo) {
    FolderSizeItem item;

    item = selectDirSizeCacheFromDB(dirInfo.absoluteFilePath());
    if (item.absolutePath == dirInfo.absoluteFilePath()) {
        // Get cache from DB.
        // If found, do nothing and return found item.
    } else if (dirSizeCache->contains(dirInfo.absoluteFilePath())) {
        // Get cache from DAT.
        //        if (isRunning()) {
        item = dirSizeCache->value(dirInfo.absoluteFilePath());
        //        } else {
        //            // TODO This call may cause blocking. To be moved to thread.
        //            item = getCachedDir(dirInfo, false);
        //        }
    } else {
        // DirItem always get isDir=true, subDirCount=0 and subFileCount=0
        item = FolderSizeItem(dirInfo.fileName(),
                              dirInfo.absoluteFilePath(),
                              dirInfo.lastModified(),
                              dirInfo.size(),
                              true, 0, 0);
        // TODO Emit signal to queue refreshDir job.
    }

    return item;
}

void FolderSizeModelThread::setCurrentDir(const QString currentDir, const int sortFlag)
{
    m_currentDir = currentDir;
    if (sortFlag == -1) {
        m_sortFlag = getSortFlagFromDB(m_currentDir, m_sortFlag);
    } else {
        m_sortFlag = sortFlag;
    }
}

int FolderSizeModelThread::getSortFlagFromDB(const QString absolutePath, const int defaultSortFlag)
{
    // TODO Get sortFlag from DB
    int sortFlag;
    QSqlQuery query(DatabaseManager::defaultManager().getDB());
    query.prepare("SELECT sort_flag FROM folderpie_cache WHERE id = :id");
    query.bindValue(":id", absolutePath);
    if (query.exec()) {
        if (query.next()) {
            sortFlag = query.value(query.record().indexOf("sort_flag")).toInt();
            qDebug() << "FolderSizeModelThread::getSortFlagFromDB" << absolutePath << "sortFlag" << sortFlag;
            return sortFlag;
        }
    }

    return defaultSortFlag;
}

int FolderSizeModelThread::sortFlag() const
{
    return m_sortFlag;
}

bool FolderSizeModelThread::setSortFlag(int sortFlag, bool saveSortFlag)
{
    qDebug() << "FolderSizeModelThread::setSortFlag m_sortFlag " << m_sortFlag << " sortFlag " << sortFlag;
    if (m_sortFlag != sortFlag) {
        m_sortFlag = sortFlag;

        // TODO Save to DB
        if (saveSortFlag) {
            QSqlQuery query(DatabaseManager::defaultManager().getDB());
            query.prepare("UPDATE folderpie_cache SET sort_flag = :sort_flag WHERE id = :id");
            query.bindValue(":sort_flag", m_sortFlag);
            query.bindValue(":id", m_currentDir);
            if (query.exec()) {
                DatabaseManager::defaultManager().getDB().commit();
                qDebug() << "FolderSizeModelThread::setSortFlag update sortFlag" << m_currentDir << " sortFlag " << m_sortFlag;
            }
        }

        return true;
    } else {
        return false;
    }
}

QStringList FolderSizeModelThread::nameFilters() const
{
    return m_nameFilters;
}

void FolderSizeModelThread::setNameFilters(const QStringList nameFilters)
{    
    m_nameFilters = nameFilters;
}

bool FolderSizeModelThread::changeDir(const QString dirName, const int sortFlag)
{
    QDir dir = QDir(m_currentDir);
    if (dir.isRoot() && dirName == "..") {
        qDebug() << "FolderSizeModelThread::changeDir " << m_currentDir << " is root. Changing to parent folder is aborted.";
        return false;
    } else {
        dir.cd(dirName);
        setCurrentDir(dir.absolutePath(), sortFlag);
        return true;
    }
}

bool FolderSizeModelThread::clearCache() const
{
    return m_clearCache;
}

void FolderSizeModelThread::setClearCache(bool clearCache)
{
    qDebug() << "FolderSizeModelThread::setClearCache" << clearCache << "isRunning()" << isRunning();
    if (!isRunning()) m_clearCache = clearCache;
}

bool FolderSizeModelThread::abortFlag() const
{
    return m_abortFlag;
}

void FolderSizeModelThread::setAbortFlag(bool flag)
{
    m_abortFlag = flag;
}

bool FolderSizeModelThread::rollbackFlag() const
{
    return m_rollbackFlag;
}

void FolderSizeModelThread::setRollbackFlag(bool flag)
{
    m_rollbackFlag = flag;
}

void FolderSizeModelThread::setRunMethod(int method)
{
    qDebug() << "FolderSizeModelThread::setRunMethod" << method << "isRunning()" << isRunning();
    if (!isRunning()) m_runMethod = method;
}

void FolderSizeModelThread::setSourcePath(const QString sourcePath)
{
    if (!isRunning()) {
        m_sourcePath = sourcePath;
    }
}

void FolderSizeModelThread::setTargetPath(const QString targetPath)
{
    if (!isRunning()) {
        m_targetPath = targetPath;
    }
}

void FolderSizeModelThread::run()
{
    qDebug() << "FolderSizeModelThread::run m_runMethod" << m_runMethod << "m_clearCache" << m_clearCache << "m_sourcePath" << m_sourcePath << "m_targetPath" << m_targetPath;

    // TODO Process UI events before start.
    QApplication::processEvents(QEventLoop::AllEvents, 50);

    // Reset flags.
    setAbortFlag(false);
    setRollbackFlag(true);

    switch (m_runMethod) {
    case FetchDirSize:
        fetchDirSize(m_sourcePath, m_clearCache);
        break;
    case CopyFile:
        if (copy(m_runMethod, m_sourcePath, m_targetPath)) {
            // Reset copy method parameters
            m_sourcePath = "";
            m_targetPath = "";
        } else {
            // TODO rollback.
            if (abortFlag() && rollbackFlag()) {
                qDebug() << "FolderSizeModelThread::run rollback by delete targetPath" << m_targetPath;
                deleteDir(m_targetPath);
            }
        }
        break;
    case MoveFile:
        // TODO Try rename first, then copy/delete if it failed.
        if (move(m_sourcePath, m_targetPath)) {
            // Reset copy method parameters
            m_sourcePath = "";
            m_targetPath = "";
        } else if (copy(m_runMethod, m_sourcePath, m_targetPath)) {
            if (!abortFlag()) deleteDir(m_sourcePath);

            // Reset copy method parameters
            m_sourcePath = "";
            m_targetPath = "";
        } else {
            // TODO rollback.
            if (abortFlag() && rollbackFlag()) {
                qDebug() << "FolderSizeModelThread::run rollback by delete targetPath" << m_targetPath;
                deleteDir(m_targetPath);
            }
        }
        break;
    case DeleteFile:
        // Delay to let deleteProgressDialog show before progressing.
        msleep(100);

        if (deleteDir(m_sourcePath, false, m_settings.value("FolderSizeModelThread.forceDeleteReadOnly.enabled", false).toBool())) {
            qDebug() << "FolderSizeModelThread::run delete sourcePath" << m_sourcePath << "is done.";
            emit deleteFinished(m_runMethod, m_sourcePath, "Deleting " + m_sourcePath + " is done.", 0);

            // Reset method parameters
            m_sourcePath = "";
            m_targetPath = "";
        } else {
            if (abortFlag()) {
                emit deleteFinished(m_runMethod, m_sourcePath, tr("Deleting %1 is aborted.").arg(m_sourcePath), -2);
            }
        }
        break;
    case TrashFile:
        if (trash(m_sourcePath, m_targetPath)) {
            qDebug() << "FolderSizeModelThread::run trash sourcePath" << m_sourcePath << "to" << m_targetPath << "is done.";
            emit trashFinished(TrashFile, m_sourcePath, m_targetPath, tr("Move %1 to %2 is done successfully.").arg(m_sourcePath).arg(m_targetPath), 0);
        } else {
            qDebug() << "FolderSizeModelThread::run trash sourcePath" << m_sourcePath << "to" << m_targetPath << "is failed.";
            emit trashFinished(TrashFile, m_sourcePath, m_targetPath, tr("Move %1 to %2 is failed.").arg(m_sourcePath).arg(m_targetPath), -1);
        }
        // Reset method parameters
        m_sourcePath = "";
        m_targetPath = "";
        break;
    case EmptyDir:
        if (emptyDir(m_sourcePath)) {
            qDebug() << "FolderSizeModelThread::run emptyDir sourcePath" << m_sourcePath << "is done.";
            emit emptyDirFinished(EmptyDir, m_sourcePath, tr("Empty %1 is done successfully.").arg(m_sourcePath), 0);
        } else {
            qDebug() << "FolderSizeModelThread::run emptyDir sourcePath" << m_sourcePath << "is failed.";
            emit emptyDirFinished(EmptyDir, m_sourcePath, tr("Empty %1 is failed.").arg(m_sourcePath), -1);
        }
        // Reset method parameters
        m_sourcePath = "";
        m_targetPath = "";
        break;
    }

    // TODO Process events before thread is finished.
    QApplication::processEvents(QEventLoop::AllEvents, 50);
}

bool FolderSizeModelThread::setFileAttribute(QString localFilePath, FileAttribute attribute, bool value)
{
#ifdef Q_OS_SYMBIAN
    QString nativeLocalFilePath = QDir::toNativeSeparators(localFilePath);

    TPtrC pLocalFilePath (static_cast<const TUint16*>(nativeLocalFilePath.utf16()), nativeLocalFilePath.length());
    TBuf<100> buf;
    buf.Copy(pLocalFilePath);

    RFs fs;
    TInt res;
    TUint postAtts;
    fs.Connect();

    // NOTE RFs::SetAtt() must use path with native separator.
    if (attribute == ReadOnly) {
//        qDebug() << "FolderSizeModelThread::setFileAttribute ReadOnly" << localFilePath << value;
        if (value) {
            res = fs.SetAtt(buf, KEntryAttReadOnly | KEntryAttNormal, KEntryAttNormal);
        } else {
            res = fs.SetAtt(buf, KEntryAttNormal, KEntryAttReadOnly);
        }
    } else if (attribute == Hidden) {
//        qDebug() << "FolderSizeModelThread::setFileAttribute Hidden" << localFilePath << value;
        if (value) {
            res = fs.SetAtt(buf, KEntryAttHidden, KEntryAttNormal);
        } else {
            res = fs.SetAtt(buf, KEntryAttNormal, KEntryAttHidden);
        }
    }
    fs.Att(buf, postAtts);
    RDebug::Print(_L("FolderSizeModelThread::setFileAttribute %S %d %04x"), &buf, res, postAtts);
    fs.Close();

    return (res >= 0);
#endif
    return false;
}
