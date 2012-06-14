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

FolderSizeModelThread::FolderSizeModelThread(QObject *parent) : QThread(parent)
{
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
        in >> dirSizeCache;

        qDebug() << QTime::currentTime() << "FolderSizeModelThread::loadDirSizeCache " << dirSizeCache.count();

        emit loadDirSizeCacheFinished();
    }
}

void FolderSizeModelThread::saveDirSizeCache() {
    if (!dirSizeCache.isEmpty()) {
        // TODO save caches from file.
        QFile file(CACHE_FILE_PATH);
        if (file.open(QIODevice::WriteOnly)) {
            QDataStream out(&file);   // we will serialize the data into the file
            out << dirSizeCache;

            qDebug() << "FolderSizeModelThread::saveDirSizeCache " << dirSizeCache.count();
        }
    }
}

bool FolderSizeModelThread::copyFile(int method, const QString sourcePath, const QString targetPath)
{
    qDebug() << "FolderSizeModelThread::copyFile method" << method << "sourceFile" << sourcePath << "targetFile" << targetPath;

    QFileInfo sourceFileInfo(sourcePath);
    QFileInfo targetFileInfo(targetPath);
    QString sourceAbsFilePath = sourcePath;
    QString targetAbsFilePath;

    if (targetFileInfo.isDir()) {
        targetAbsFilePath = QDir(targetFileInfo.absoluteFilePath()).absoluteFilePath(sourceFileInfo.fileName());
    } else {
        targetAbsFilePath = targetPath;
    }

    qDebug() << "FolderSizeModelThread::copyFile method" << method << "sourceAbsFilePath" << sourceAbsFilePath << "targetAbsFilePath" << targetAbsFilePath;

    if (sourceAbsFilePath == targetAbsFilePath) {
        qDebug() << "FolderSizeModelThread::copyFile Error method" << method << "sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath;
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Both source/target files can't be the same file.");
        return false;
    }

    QFile sourceFile(sourceAbsFilePath);
    QFile targetFile(targetAbsFilePath);

    // TODO copy with feedback progress to QML.
    bool res = false;
    if (sourceFile.open(QFile::ReadOnly) && targetFile.open(QFile::WriteOnly)) {
        qint64 totalBytes = 0;
        char buf[4096];
        qint64 c = sourceFile.read(buf, sizeof(buf));
        while (c > 0) {
            targetFile.write(buf, c);
            totalBytes += c;

            // Emit copy progress.
            emit copyProgress(method, sourceAbsFilePath, targetAbsFilePath, totalBytes, sourceFile.size());

            // Tell event loop to process event before it will process time consuming task.
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

            // Read next buffer.
            c = sourceFile.read(buf, sizeof(buf));
        }

        res = true;
    } else {
        qDebug() << "FolderSizeModelThread::copyFile Error method" << method << "sourceFile" << sourceAbsFilePath << "targetFile" << targetAbsFilePath;
        emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Both source/target files can't be read/written.");
        return false;
    }

    if (res) {
        // Remove target path cache.
        removeDirSizeCache(targetPath);

        // Delete source file if method is MoveFile.
        if (method == MoveFile) {
            // Remove file from file system.
            res = sourceFile.remove();

            if (res) {
                // Remove deleted file path cache.
                removeDirSizeCache(sourceFileInfo.absolutePath());
            }
        }
    }

    // Reset copy method parameters
    m_sourcePath = "";
    m_targetPath = "";

    // TODO should emit signal as finish.
    emit copyFinished(method, sourceAbsFilePath, targetAbsFilePath, "Action is done successfully.");

    return res;
}

bool FolderSizeModelThread::isDirSizeCacheExisting()
{
    // Return true if cache is not empty or cache is loading.
    return ( (isRunning() && m_runMethod == LoadDirSizeCache) || dirSizeCache.count() > 0);
}

void FolderSizeModelThread::removeDirSizeCache(const QString key)
{
    qDebug() << "FolderSizeModelThread::removeDirSizeCache started from" << key;

    QDir dir(key);
    bool canCdup = true;
    while (canCdup) {
        dirSizeCache.remove(dir.absolutePath());
        qDebug() << "FolderSizeModelThread::removeDirSizeCache remove cache for " << dir.absolutePath();

        canCdup = dir.cdUp();
    }
}

FolderSizeItem FolderSizeModelThread::getCachedDir(const QFileInfo dir, const bool clearCache) {
    double dirSize = 0;
    double subDirCount = 0;
    double subFileCount = 0;
    FolderSizeItem cachedDir;
    bool isFound = false;

    // Get cached dirSize if any and clearCache==false
    if (!clearCache && dirSizeCache.contains(dir.absoluteFilePath())) {
//        qDebug() << QTime::currentTime() << "FolderSizeModelThread::getCachedDir found " + dir.absoluteFilePath();

        cachedDir = dirSizeCache.value(dir.absoluteFilePath());
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

        // Tell event loop to process event before it will process time consuming task.
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

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
            }
        }

        // Put calculated dirSize to cache.
        cachedDir = FolderSizeItem(dir.fileName(), dir.absoluteFilePath(), dir.lastModified(), dirSize, true, subDirCount, subFileCount);
        dirSizeCache.insert(dir.absoluteFilePath(), cachedDir);
    }

    emit fetchDirSizeUpdated(dir.absoluteFilePath());

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

QList<FolderSizeItem> FolderSizeModelThread::getItemList() const
{
    return itemList;
}

QList<FolderSizeItem> FolderSizeModelThread::getDirContent(const QString dirPath)
{
    // Get only contents in directory (not recursive).
    QList<FolderSizeItem> tempList;

    QDir dir;
    dir = QDir(dirPath);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        QString fileName = fileInfo.fileName();

        if (fileName == ".") {
            // skip current directory '.'
            continue;
        } else if (fileName == "..") {
            // skip current directory '.'
            continue;
        } else if (fileInfo.isDir()) {
            // If running, return cache or dummy item.
            // If not running, drilldown to get dir content.
            tempList.append(getDirItem(fileInfo));
        } else {
            tempList.append(getFileItem(fileInfo));
        }
    }

    // Sort itemList as current sortFlag.
    sortItemList(tempList);

    return tempList;
}

void FolderSizeModelThread::fetchDirSize(const bool clearCache) {
    // Fetch to cache only. No need to populate to itemList.
    qDebug() << QTime::currentTime() << "FolderSizeModelThread::fetchDirSize started " << currentDir() << " clearCache " << clearCache;

    emit fetchDirSizeStarted();

    QFileInfo dirInfo(m_currentDir);
    getCachedDir(dirInfo, clearCache);

    // Reset after fetch.
    m_clearCache = false;

    // Populate and sort directory content to itemList.
    refreshItemList();

    emit fetchDirSizeFinished();

    qDebug() << QTime::currentTime() << "FolderSizeModelThread::fetchDirSize is done " << currentDir();
}

void FolderSizeModelThread::refreshItemList()
{
    qDebug() << "FolderSizeModelThread::refreshDirSize";

    // Populate dir content to itemList.
    itemList = getDirContent(m_currentDir);

    // Sort itemList as current sortFlag.
    sortItemList(itemList);
}

FolderSizeItem FolderSizeModelThread::getFileItem(const QFileInfo fileInfo) {
    // FileItem always get isDir=false, subDirCount=0 and subFileCount=0
    FolderSizeItem item = FolderSizeItem(fileInfo.fileName(),
                                         fileInfo.absoluteFilePath(),
                                         fileInfo.lastModified(),
                                         fileInfo.size(),
                                         false, 0, 0);
    return item;
}

FolderSizeItem FolderSizeModelThread::getDirItem(const QFileInfo dirInfo) {
    FolderSizeItem item;
    if (dirSizeCache.contains(dirInfo.absoluteFilePath())) {
        if (isRunning()) {
            item = dirSizeCache.value(dirInfo.absoluteFilePath());
        } else {
            // TODO This call may cause blocking. To be moved to thread.
            item = getCachedDir(dirInfo, false);
        }
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

        sortItemList(itemList);
        return true;
    } else {
        return false;
    }
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

void FolderSizeModelThread::removeItem(const int index)
{
    itemList.removeAt(index);
}

FolderSizeItem FolderSizeModelThread::getItem(const int index)
{
    return itemList.at(index);
}

void FolderSizeModelThread::setItem(const int index, FolderSizeItem &item)
{
    itemList.replace(index, item);
}

bool FolderSizeModelThread::clearCache()
{
    return m_clearCache;
}

void FolderSizeModelThread::setClearCache(bool clearCache)
{
    if (!isRunning()) m_clearCache = clearCache;
}

void FolderSizeModelThread::setRunMethod(int method)
{
    qDebug() << "FolderSizeModelThread::setRunMethod" << method << "isRunning()" << isRunning();
    if (!isRunning()) m_runMethod = method;
}

void FolderSizeModelThread::setCopyPath(const QString sourcePath, const QString targetPath)
{
    if (!isRunning()) {
        m_sourcePath = sourcePath;
        m_targetPath = targetPath;
    }
}

void FolderSizeModelThread::run()
{
    qDebug() << "FolderSizeModelThread::run m_runMethod" << m_runMethod;

    switch (m_runMethod) {
    case FetchDirSize:
        fetchDirSize(m_clearCache);
        break;
    case LoadDirSizeCache:
        loadDirSizeCache();
        break;
    case CopyFile:
        copyFile(m_runMethod, m_sourcePath, m_targetPath);
        break;
    case MoveFile:
        copyFile(m_runMethod, m_sourcePath, m_targetPath);
        break;
    }
}
