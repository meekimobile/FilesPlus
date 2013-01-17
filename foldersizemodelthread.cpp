#include "foldersizemodelthread.h"
#include "foldersizeitem.h"
#include "foldersizeitemlistmodel.h"
#include <QDebug>
#include <QCoreApplication>

bool nameLessThan(const FolderSizeItem &o1, const FolderSizeItem &o2)
{
//    qDebug() << "nameLessThan" << o1.name << o1.fileType << o2.name << o2.fileType;
    return o1.name < o2.name;
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
        // If both are dir, compare name.
        // If both are file, compare type and name.
        if (o1.isDir && o2.isDir) {
//            qDebug() << "typeLessThan" << o1.name << o1.fileType << o2.name << o2.fileType;
            return o1.name < o2.name;
        } else {
            if (o1.fileType == o2.fileType) {
                return o1.name < o2.name;
            } else if (o1.fileType < o2.fileType) {
                return true;
            } else {
                return false;
            }
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
const QString FolderSizeModelThread::CACHE_FILE_PATH = "/home/user/.folderpie/FolderPieCache.dat";
const QString FolderSizeModelThread::CACHE_DB_PATH = "/home/user/.folderpie/FolderPieCache.db";
const QString FolderSizeModelThread::CACHE_DB_CONNECTION_NAME = "folderpie_cache";
const QString FolderSizeModelThread::DEFAULT_CURRENT_DIR = "/home/user/MyDocs";
const int FolderSizeModelThread::FILE_READ_BUFFER = 32768;
const int FolderSizeModelThread::FILE_COPY_DELAY = 50;
const int FolderSizeModelThread::FILE_DELETE_DELAY = 200;
#else
const QString FolderSizeModelThread::CACHE_FILE_PATH = "C:/FolderPieCache.dat";
const QString FolderSizeModelThread::CACHE_DB_PATH = "FolderPieCache.db"; // Symbian supports only default database file location.
const QString FolderSizeModelThread::CACHE_DB_CONNECTION_NAME = "folderpie_cache";
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

    // TODO Initialize SQLLITE DB.
    // Moved to enqueue while constructing model class.
//    initializeDB();

//    m_currentDir = DEFAULT_CURRENT_DIR;
//    m_sortFlag = getSortFlagFromDB(m_currentDir, SortByType);
    dirSizeCache = new QHash<QString, FolderSizeItem>();
    m_itemCache = new QHash<QString, FolderSizeItem>();

    // TODO To be implemented which file type to be indexed.
    m_isFileCached = false;
}

FolderSizeModelThread::~FolderSizeModelThread()
{
    // Close database before destroyed.
    closeDB();
}

void FolderSizeModelThread::initializeDB()
{
    qDebug() << "FolderSizeModelThread::initializeDB started";
    emit initializeDBStarted();

    // Create cache database path if it's not exist.
    QFile file(CACHE_DB_PATH);
    QFileInfo info(file);
    if (!info.absoluteDir().exists()) {
        qDebug() << "FolderSizeModelThread::initializeDB dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
        bool res = QDir::home().mkpath(info.absolutePath());
        if (!res) {
            qDebug() << "FolderSizeModelThread::initializeDB can't make dir" << info.absolutePath();
        } else {
            qDebug() << "FolderSizeModelThread::initializeDB make dir" << info.absolutePath();
        }
    }

    // First initialization.
    m_db = QSqlDatabase::addDatabase("QSQLITE", CACHE_DB_CONNECTION_NAME);
    m_db.setDatabaseName(CACHE_DB_PATH);    
    bool ok = m_db.open();
    qDebug() << "FolderSizeModelThread::initializeDB" << ok << "connectionName" << m_db.connectionName() << "databaseName" << m_db.databaseName() << "driverName" << m_db.driverName();

    QSqlQuery query(m_db);
    bool res = false;

    res = query.exec("CREATE TABLE folderpie_cache(id TEXT PRIMARY_KEY, name TEXT, absolute_path TEXT, last_modified TEXT, size INTEGER, is_dir INTEGER, sub_dir_count INTEGER, sub_file_count INTEGER, file_type TEXT)");
    if (res) {
        qDebug() << "FolderSizeModelThread::initializeDB CREATE TABLE folderpie_cache is done.";
    } else {
        qDebug() << "FolderSizeModelThread::initializeDB CREATE TABLE folderpie_cache is failed. Error" << query.lastError();
    }

    res = query.exec("CREATE UNIQUE INDEX IF NOT EXISTS folderpie_cache_pk ON folderpie_cache (id)");
    if (res) {
        qDebug() << "FolderSizeModelThread::initializeDB CREATE INDEX folderpie_cache_pk is done.";
    } else {
        qDebug() << "FolderSizeModelThread::initializeDB CREATE INDEX folderpie_cache_pk is failed. Error" << query.lastError();
    }

    // TODO Additional initialization.
    res = query.exec("ALTER TABLE folderpie_cache ADD COLUMN sort_flag INTEGER");
    if (res) {
        qDebug() << "FolderSizeModelThread::initializeDB adding column folderpie_cache.sort_flag is done.";
    } else {
        qDebug() << "FolderSizeModelThread::initializeDB adding column folderpie_cache.sort_flag is failed. Error" << query.lastError();
    }

    // Prepare queries.
    m_selectPS = QSqlQuery(m_db);
    m_selectPS.prepare("SELECT * FROM folderpie_cache WHERE id = :id");

    m_insertPS = QSqlQuery(m_db);
    m_insertPS.prepare("INSERT INTO folderpie_cache(id, name, absolute_path, last_modified, size, is_dir, sub_dir_count, sub_file_count, file_type)"
                       " VALUES (:id, :name, :absolute_path, :last_modified, :size, :is_dir, :sub_dir_count, :sub_file_count, :file_type)");

    m_updatePS = QSqlQuery(m_db);
    m_updatePS.prepare("UPDATE folderpie_cache SET"
                       " name = :name, absolute_path = :absolute_path, last_modified = :last_modified, size = :size,"
                       " is_dir = :is_dir, sub_dir_count = :sub_dir_count, sub_file_count = :sub_file_count, file_type = :file_type"
                       " WHERE id = :id");

    m_deletePS = QSqlQuery(m_db);
    m_deletePS.prepare("DELETE FROM folderpie_cache WHERE id = :id");

    m_countPS = QSqlQuery(m_db);
    m_countPS.prepare("SELECT count(*) count FROM folderpie_cache");

    emit initializeDBFinished();
}

void FolderSizeModelThread::closeDB()
{
    qDebug() << "FolderSizeModelThread::closeDB" << countDirSizeCacheDB();
    m_db.close();
}

FolderSizeItem FolderSizeModelThread::selectDirSizeCacheFromDB(const QString id)
{
    FolderSizeItem item;
    bool res;
    QSqlRecord rec = m_selectPS.record();

    // Find in itemCache, return if found.
    if (m_itemCache->contains(id)) {
        item = m_itemCache->value(id);
//        qDebug() << "FolderSizeModelThread::selectDirSizeCacheFromDB cached id" << id << "item" << item;
        return item;
    }

    m_selectPS.bindValue(":id", id);
    res = m_selectPS.exec();
    if (res && m_selectPS.next()) {
//        qDebug() << "FolderSizeModelThread::selectDirSizeCacheFromDB id" << id << "is found. id" << m_selectPS.value(rec.indexOf("id")).toString() << "size" << m_selectPS.value(rec.indexOf("size")).toLongLong();
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
        qDebug() << "FolderSizeModelThread::selectDirSizeCacheFromDB id" << id << " is not found.";
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
        m_db.commit();

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
        m_db.commit();

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

    QSqlQuery m_updateTreePS(m_db);
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
        m_db.commit();

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
        m_db.commit();

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

    QSqlQuery m_deleteTreePS(m_db);
    m_deleteTreePS.prepare("DELETE FROM folderpie_cache WHERE id like :id||'/%'");
    m_deleteTreePS.bindValue(":id", id);
    res = m_deleteTreePS.exec();
    if (res) {
        qDebug() << "FolderSizeModelThread::deleteDirSizeCacheTreeToDB delete done" << id << "res" << res << "numRowsAffected" << m_deleteTreePS.numRowsAffected();
        m_db.commit();

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

void FolderSizeModelThread::loadDirSizeCache() {
    // TODO load caches from file.
//    qDebug() << "FolderSizeModelThread::loadDirSizeCache CACHE_FILE_PATH" << CACHE_FILE_PATH << "DEFAULT_CURRENT_DIR" << DEFAULT_CURRENT_DIR;
    QFile file(CACHE_FILE_PATH);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> *dirSizeCache;

        qDebug() << QTime::currentTime() << "FolderSizeModelThread::loadDirSizeCache " << dirSizeCache->count();

        emit loadDirSizeCacheFinished();
    }
}

void FolderSizeModelThread::saveDirSizeCache() {
    if (!dirSizeCache->isEmpty()) {
        // TODO save caches from file.

        // TODO Clean up invalid items.
        cleanItems();

        QFile file(CACHE_FILE_PATH);
        QFileInfo info(file);
        if (!info.absoluteDir().exists()) {
            qDebug() << "FolderSizeModelThread::saveDirSizeCache dir" << info.absoluteDir().absolutePath() << "doesn't exists.";
            bool res = QDir::home().mkpath(info.absolutePath());
            if (!res) {
                qDebug() << "FolderSizeModelThread::saveDirSizeCache can't make dir" << info.absolutePath();
            } else {
                qDebug() << "FolderSizeModelThread::saveDirSizeCache make dir" << info.absolutePath();
            }
        }
        if (file.open(QIODevice::WriteOnly)) {
            QDataStream out(&file);   // we will serialize the data into the file
            out << *dirSizeCache;

            qDebug() << "FolderSizeModelThread::saveDirSizeCache " << dirSizeCache->count();
        }
    }
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

bool FolderSizeModelThread::deleteDir(const QString sourcePath)
{
    qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath;

    emit deleteStarted(m_runMethod, sourcePath);

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
            res = res && deleteDir(item.absoluteFilePath());

            if (!res) break;
        }

        // Return false if aborted.
        if (m_abortFlag) {
            return false;
        }

        // Delete dir.
        if (res) {
            res = res && sourceFileInfo.dir().rmdir(sourceFileInfo.fileName());
        }
    } else {
        // Delete file.
        QFile sourceFile(sourcePath);
        res = sourceFile.remove();
    }

    if (!res) {
        qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath << "failed.";
        emit deleteProgress(m_runMethod, sourcePath, tr("Deleting %1 is failed.").arg(sourcePath), -1);
    } else {
        // TODO Move to emit only once per thread call.
        qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath << "done.";
        emit deleteProgress(m_runMethod, sourcePath, tr("Deleting %1 is done.").arg(sourcePath), 0);

        // Move to deleteFinishedFilter
        //        // Remove cache up to parent.
        //        removeDirSizeCache(sourcePath);
    }

    // This sleep is a must.
    // Sleep for process deleteFinished signal.
    delay(FILE_DELETE_DELAY);

    return res;
}

bool FolderSizeModelThread::isDirSizeCacheExisting()
{
    // Return true if cache is not empty or cache is loading.
//    qDebug() << "FolderSizeModelThread::isDirSizeCacheExisting isRunning(LoadDirSizeCache)" << (isRunning() && m_runMethod == LoadDirSizeCache) << "dirSizeCache" << (dirSizeCache->count()) << "countDirSizeCacheDB" << (countDirSizeCacheDB());
    return ( (isRunning() && m_runMethod == LoadDirSizeCache) || dirSizeCache->count() > 0 || countDirSizeCacheDB() > 0);
}

void FolderSizeModelThread::removeDirSizeCache(const QString key)
{
    //    qDebug() << "FolderSizeModelThread::removeDirSizeCache started from" << key;

    // Remove only specified key.
    dirSizeCache->remove(key);

    // Dirty cache.
    FolderSizeItem item = selectDirSizeCacheFromDB(key);
    if (item.absolutePath == key) {
        item.isDir = true;
        item.lastModified = QDateTime::fromMSecsSinceEpoch(0);
        updateDirSizeCacheToDB(item);
    }
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
    if (!isFound || !isValid) {
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
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    dir.setNameFilters(m_nameFilters);
    switch (m_sortFlag) {
    case SortByName:
        dir.setSorting(QDir::Name);
        break;
    case SortBySize:
        dir.setSorting(QDir::Size | QDir::DirsFirst);
        break;
    case SortByTime:
        dir.setSorting(QDir::Time);
        break;
    case SortByType:
        dir.setSorting(QDir::Type);
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
                itemList.append(getDirItem(fileInfo));
            } else {
                itemList.append(getCachedDir(fileInfo));
            }
        } else {
            itemList.append(getCachedDir(fileInfo));
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

    emit fetchDirSizeStarted();

    QFileInfo dirInfo(sourcePath);
    getCachedDir(dirInfo, clearCache);

    // Reset after fetch.
    m_clearCache = false;

    emit fetchDirSizeFinished();

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
    QSqlQuery query(m_db);
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
            QSqlQuery query(m_db);
            query.prepare("UPDATE folderpie_cache SET sort_flag = :sort_flag WHERE id = :id");
            query.bindValue(":sort_flag", m_sortFlag);
            query.bindValue(":id", m_currentDir);
            if (query.exec()) {
                m_db.commit();
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
    case LoadDirSizeCache:
        loadDirSizeCache();
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

        if (deleteDir(m_sourcePath)) {
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
    case InitializeDB:
        initializeDB();
        break;
    }

    // TODO Process events before thread is finished.
    QApplication::processEvents(QEventLoop::AllEvents, 50);
}
