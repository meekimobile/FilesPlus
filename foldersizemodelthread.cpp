#include "foldersizemodelthread.h"
#include "foldersizeitem.h"
#include "foldersizeitemlistmodel.h"
#include <QDebug>
#include <QCoreApplication>

bool nameLessThan(const FolderSizeItem &o1, const FolderSizeItem &o2)
{
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

const QString FolderSizeModelThread::CACHE_FILE_PATH = "C:/FolderPieCache.dat";
const QString FolderSizeModelThread::DEFAULT_CURRENT_DIR = "C:/";
const int FolderSizeModelThread::FILE_READ_BUFFER = 32768;

FolderSizeModelThread::FolderSizeModelThread(QObject *parent) : QThread(parent)
{
    dirSizeCache = new QHash<QString, FolderSizeItem>();
}

QString FolderSizeModelThread::currentDir() const
{
    return m_currentDir;
}

void FolderSizeModelThread::loadDirSizeCache() {
    // TODO load caches from file.
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
        QFile file(CACHE_FILE_PATH);
        if (file.open(QIODevice::WriteOnly)) {
            QDataStream out(&file);   // we will serialize the data into the file
            out << *dirSizeCache;

            qDebug() << "FolderSizeModelThread::saveDirSizeCache " << dirSizeCache->count();
        }
    }
}

bool FolderSizeModelThread::copy(int method, const QString sourcePath, const QString targetPath)
{
    // targetPath is always actual file/folder name, not parent folder.
    qDebug() << "FolderSizeModelThread::copy method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath;

    if (m_abortFlag) {
        emit copyFinished(method, sourcePath, targetPath, "Copy " + sourcePath + " to " + targetPath + " is aborted.", -2, 0, -1);
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
            emit copyFinished(method, sourcePath, targetPath, "Copy " + sourcePath + " to " + targetPath + " is done successfully.", 0, 0, itemSize);
        } else {
            qDebug() << "FolderSizeModelThread::copy method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath << "is failed. err = -1";
            emit copyFinished(method, sourcePath, targetPath, "Copy " + sourcePath + " to " + targetPath + " is failed.", -1, 0, itemSize);
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

    qDebug() << "FolderSizeModelThread::copyFile method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath;

    // Return false if aborted.
    if (m_abortFlag) {
        emit copyFinished(method, sourcePath, targetPath, "Copy " + sourcePath + " to " + targetPath + " is aborted.", -2, 0, -1);
        return false;
    }

    QFileInfo sourceFileInfo(sourcePath);
    QFileInfo targetFileInfo(targetPath);
    QString sourceAbsFilePath = sourcePath;
    QString targetAbsFilePath;
    qint64 itemSize = sourceFileInfo.size();

    // Verify targetPath is file or folder. It will be prepared to file path.
    if (targetFileInfo.isDir()) {
        targetAbsFilePath = QDir(targetFileInfo.absoluteFilePath()).absoluteFilePath(sourceFileInfo.fileName());
    } else {
        targetAbsFilePath = targetPath;
    }

    qDebug() << "FolderSizeModelThread::copyFile method" << method << "sourceAbsFilePath" << sourceAbsFilePath << "targetAbsFilePath" << targetAbsFilePath;

    if (sourceAbsFilePath == targetAbsFilePath) {
        qDebug() << "FolderSizeModelThread::copyFile Error sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath;
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Both source/target path can't be the same file.", -2, 0, itemSize);
        return false;
    }

    QFile sourceFile(sourceAbsFilePath);
    QFile targetFile(targetAbsFilePath);

    // TODO copy with feedback progress to QML.
    qint64 totalBytes = 0;
    bool res = false;
    if (sourceFile.open(QFile::ReadOnly) && targetFile.open(QFile::WriteOnly)) {
        char buf[FolderSizeModelThread::FILE_READ_BUFFER];
        qint64 c = sourceFile.read(buf, sizeof(buf));
        while (c > 0 && !m_abortFlag) {
            targetFile.write(buf, c);
            totalBytes += c;

            // Emit copy progress.
            emit copyProgress(method, sourceAbsFilePath, targetAbsFilePath, totalBytes, sourceFile.size());

            // TODO Commented below line can speedup copying.
//            msleep(50);

            // Read next buffer.
//            qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << "FolderSizeModelThread::copyFile before read";
            c = sourceFile.read(buf, sizeof(buf));
//            qDebug() << QTime::currentTime().toString("hh:mm:ss.zzz") << "FolderSizeModelThread::copyFile after read" << c;
        }

        qDebug() << "FolderSizeModelThread::copyFile done method" << method << "sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath << "totalBytes" << totalBytes;
        res = !m_abortFlag;
    } else {
        qDebug() << "FolderSizeModelThread::copyFile Error method" << method << "sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath;
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Both source " + sourceAbsFilePath + " and target " + targetAbsFilePath + " can't be read/written.", -3, totalBytes, itemSize);
        return false;
    }

    // Close files.
    sourceFile.close();
    targetFile.close();

    // Return false if aborted.
    if (m_abortFlag) {
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Copy " + sourceAbsFilePath + " is aborted.", -4, totalBytes, itemSize);
        return false;
    }

    // Emit signal as finish.
    if (res) {
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Copy " + sourceAbsFilePath + " to " + targetAbsFilePath + " is done successfully.", 0, totalBytes, itemSize);
    } else {
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Copy " + sourceAbsFilePath + " to " + targetAbsFilePath + " is failed.", -1, totalBytes, itemSize);
    }

    // Sleep after emit signal.
    msleep(50);

    return res;
}

bool FolderSizeModelThread::deleteDir(const QString sourcePath)
{
    qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath;

    emit deleteStarted(m_runMethod, sourcePath);

    msleep(50);

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

        // Delete dir.
        res = res && sourceFileInfo.dir().rmdir(sourceFileInfo.fileName());
    } else {
        // Delete file.
        QFile sourceFile(sourcePath);
        res = sourceFile.remove();
    }

    if (!res) {
        qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath << "failed.";
        emit deleteFinished(m_runMethod, sourcePath, "Deleting " + sourcePath + " is failed.", -1);
    } else {
        qDebug() << "FolderSizeModelThread::deleteDir sourcePath" << sourcePath << "done.";
        emit deleteFinished(m_runMethod, sourcePath, "Deleting " + sourcePath + " is done.", 0);

        // Move to deleteFinishedFilter
//        // Remove cache up to parent.
//        removeDirSizeCache(sourcePath);
    }

    // This sleep is a must.
    // Sleep for process deleteFinished signal.
    msleep(50);

    return res;
}

bool FolderSizeModelThread::isDirSizeCacheExisting()
{
    // Return true if cache is not empty or cache is loading.
    return ( (isRunning() && m_runMethod == LoadDirSizeCache) || dirSizeCache->count() > 0);
}

void FolderSizeModelThread::removeDirSizeCache(const QString key)
{
//    qDebug() << "FolderSizeModelThread::removeDirSizeCache started from" << key;

    // Remove only specified key.
    dirSizeCache->remove(key);

//    QFileInfo fileInfo(key);
//    QDir dir;
//    if (!fileInfo.isDir()) {
//        dir = fileInfo.absoluteDir();
//    } else {
//        dir = QDir(key);
//    }

//    // Return if cache doesn't contain key.
//    if (!dirSizeCache->contains(dir.absolutePath())) {
//        qDebug() << "FolderSizeModelThread::removeDirSizeCache key" << dir.absolutePath() << "is not found. It may have been removed.";
//        return;
//    }

//    bool canCdup = true;
//    while (canCdup) {
//        dirSizeCache->remove(dir.absolutePath());
////        qDebug() << "FolderSizeModelThread::removeDirSizeCache remove cache for " << dir.absolutePath();

//        if (dir.isRoot()) {
//            canCdup = false;
//        } else {
//            canCdup = dir.cdUp();
//        }
//    }
}

FolderSizeItem FolderSizeModelThread::getCachedDir(const QFileInfo dir, const bool clearCache) {
    double dirSize = 0;
    double subDirCount = 0;
    double subFileCount = 0;
    FolderSizeItem cachedDir;
    bool isFound = false;

    // Get cached dirSize if any and clearCache==false
    if (!clearCache && dirSizeCache->contains(dir.absoluteFilePath())) {
//        qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir found " + dir.absoluteFilePath();

        cachedDir = dirSizeCache->value(dir.absoluteFilePath());
        if (cachedDir.lastModified >= dir.lastModified()) {
            // cachedDir is still valid, just return it.
//            qDebug() << QTime::currentTime() << QString("FolderSizeModelThread::getCachedDir %1 >= %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
            isFound = true;
        } else {
//            qDebug() << QTime::currentTime() << QString("FolderSizeModelThread::getCachedDir %1 < %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
        }
    }

    // If (cache is invalidated or not found) and FolderSizeModelThread is ready, get cache recursively.
    // Otherwise return dummy dir item with 0 byte.
    if (!isFound) {
//        qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir NOT found " + dir.absoluteFilePath();

        msleep(50);

        QDir d = QDir(dir.absoluteFilePath());
        d.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        QFileInfoList subList = d.entryInfoList();

        for (int i = 0; i < subList.size(); ++i) {
            QFileInfo fileInfo = subList.at(i);
            QString fileName = fileInfo.fileName();

            if (fileName == "." || fileName == "..") {
                // do nothing.
            } else if (fileInfo.isDir()) {
                // drilldown to get dir tree size.
                FolderSizeItem item = getCachedDir(fileInfo, clearCache);
                double subDirSize = item.size;
                dirSize += subDirSize;
                subDirCount++;
                // Count its sub dirs/files too.
                subDirCount += item.subDirCount;
                subFileCount += item.subFileCount;
            } else {
                dirSize += fileInfo.size();
                subFileCount++;

                // Emit signal on file is count.
                emit fetchDirSizeUpdated(fileInfo.absoluteFilePath());
            }

//            qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir path" << fileInfo.absoluteFilePath() << "dirSize" << dirSize << "subDirCount" << subDirCount << "subFileCount" << subFileCount;
        }

        // Put calculated dirSize to cache.
        cachedDir = FolderSizeItem(dir.fileName(), dir.absoluteFilePath(), dir.lastModified(), dirSize, true, subDirCount, subFileCount);
        dirSizeCache->insert(dir.absoluteFilePath(), cachedDir);
    }

//    // Emit signal on dir is cached.
//    emit fetchDirSizeUpdated(dir.absoluteFilePath());

//    qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir done " + cachedDir.name + ", " + QString("%1").arg(cachedDir.size);

    return cachedDir;
}

void FolderSizeModelThread::sortItemList(QList<FolderSizeItem> &itemList) {
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
        qSort(itemList.begin(), itemList.end(), sizeGreaterThan);
        break;
    }
}

void FolderSizeModelThread::getDirContent(const QString dirPath, QList<FolderSizeItem> &itemList)
{
    // TODO Issue: so many files caused application crash.

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
            msleep(50);
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
            itemList.append(getDirItem(fileInfo));
        } else {
            itemList.append(getFileItem(fileInfo));
        }
    }

    // Sort itemList if sortFlag == SortBySize. To sort folder size.
    if (m_sortFlag == SortBySize) {
        sortItemList(itemList);
    }
}

void FolderSizeModelThread::fetchDirSize(const bool clearCache) {
    // Fetch to cache only. No need to populate to itemList.
    qDebug() << QTime::currentTime() << "FolderSizeModelThread::fetchDirSize started " << currentDir() << " clearCache " << clearCache;

    emit fetchDirSizeStarted();

    QFileInfo dirInfo(m_currentDir);
    getCachedDir(dirInfo, clearCache);

    // Reset after fetch.
    m_clearCache = false;

    emit fetchDirSizeFinished();

    qDebug() << QTime::currentTime() << "FolderSizeModelThread::fetchDirSize is done " << currentDir();
}

FolderSizeItem FolderSizeModelThread::getItem(const QFileInfo fileInfo) const {
    if (fileInfo.isDir()) {
        return getDirItem(fileInfo);
    } else if (fileInfo.isFile()){
        return getFileItem(fileInfo);
    } else {
        return FolderSizeItem();
    }
}

FolderSizeItem FolderSizeModelThread::getFileItem(const QFileInfo fileInfo) const {
    // FileItem always get isDir=false, subDirCount=0 and subFileCount=0
    FolderSizeItem item(fileInfo.fileName(),
                        fileInfo.absoluteFilePath(),
                        fileInfo.lastModified(),
                        fileInfo.size(),
                        false, 0, 0);

    return item;
}

FolderSizeItem FolderSizeModelThread::getDirItem(const QFileInfo dirInfo) const {
    FolderSizeItem item;
    if (dirSizeCache->contains(dirInfo.absoluteFilePath())) {
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
    }

    return item;
}

void FolderSizeModelThread::setCurrentDir(const QString currentDir)
{
    m_currentDir = currentDir;
}

int FolderSizeModelThread::sortFlag() const
{
    return m_sortFlag;
}

bool FolderSizeModelThread::setSortFlag(int sortFlag)
{
    qDebug() << "FolderSizeModelThread::setSortFlag m_sortFlag " << m_sortFlag << " sortFlag " << sortFlag;
    if (m_sortFlag != sortFlag) {
        m_sortFlag = sortFlag;
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

bool FolderSizeModelThread::changeDir(const QString dirName)
{
    QDir dir = QDir(m_currentDir);
    if (dir.isRoot() && dirName == "..") {
        qDebug() << "FolderSizeModelThread::changeDir " << m_currentDir << " is root. Changing to parent folder is aborted.";
        return false;
    } else {
        dir.cd(dirName);
        setCurrentDir(dir.absolutePath());
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

    // Reset flags.
    setAbortFlag(false);
    setRollbackFlag(true);

    switch (m_runMethod) {
    case FetchDirSize:
        fetchDirSize(m_clearCache);
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
        if (copy(m_runMethod, m_sourcePath, m_targetPath)) {
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
        if (deleteDir(m_sourcePath)) {
            qDebug() << "FolderSizeModelThread::run delete sourcePath" << m_sourcePath << "is done.";
            // Reset method parameters
            m_sourcePath = "";
            m_targetPath = "";
        }
        break;
    }
}
