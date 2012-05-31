#include "foldersizemodel.h"
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
        // If both the same, compare size.
        return o1.fileType < o2.fileType && o1.name < o2.name;
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

const QString FolderSizeModel::CACHE_FILE_PATH = "C:/FolderPieCache.dat";
const QString FolderSizeModel::DEFAULT_CURRENT_DIR = "C:/";

FolderSizeModel::FolderSizeModel() : QRunnable()
{
    m_isReady = true;
}

FolderSizeModel::FolderSizeModel(QString currentDir) : QRunnable()
{
    m_currentDir = currentDir;
    m_isReady = true;
}

QString FolderSizeModel::currentDir() const
{
    return m_currentDir;
}

QStringList FolderSizeModel::getDriveList() {
    QFileInfoList drives = QDir::drives();
    QStringList driveList;

    for (int i=0; i<drives.length(); i++) {
        driveList.append(drives.at(i).absolutePath());
    }

    return driveList;
}

void FolderSizeModel::loadDirSizeCache() {
    // TODO load caches from file.
    QFile file(CACHE_FILE_PATH);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);    // read the data serialized from the file
        in >> dirSizeCache;

        qDebug() << "FolderSizeModel::loadDirSizeCache " << dirSizeCache.count();
    }
}

void FolderSizeModel::saveDirSizeCache() {
    // TODO save caches from file.
    QFile file(CACHE_FILE_PATH);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);   // we will serialize the data into the file
        out << dirSizeCache;

        qDebug() << "FolderSizeModel::saveDirSizeCache " << dirSizeCache.count();
    }
}

bool FolderSizeModel::isDirSizeCacheExisting()
{
    return (dirSizeCache.count() > 0);
}

void FolderSizeModel::removeDirSizeCache(const QString key)
{
    dirSizeCache.remove(key);
}

FolderSizeItem FolderSizeModel::getCachedDir(const QFileInfo dir, const bool clearCache) {
    double dirSize = 0;
    double subDirCount = 0;
    double subFileCount = 0;
    FolderSizeItem cachedDir;
    bool isFound = false;

    // Get cached dirSize if any and clearCache==false
    if (!clearCache && dirSizeCache.contains(dir.absoluteFilePath())) {
//        qDebug() << QTime::currentTime() << "FolderSizeModel::getCachedDir found " + dir.absoluteFilePath();

        cachedDir = dirSizeCache.value(dir.absoluteFilePath());
        if (cachedDir.lastModified >= dir.lastModified()) {
            // cachedDir is still valid, just return it.
//            qDebug() << QTime::currentTime() << QString("FolderSizeModel::getCachedDir %1 >= %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
            isFound = true;
        } else {
//            qDebug() << QTime::currentTime() << QString("FolderSizeModel::getCachedDir %1 < %2").arg(cachedDir.lastModified.toString()).arg(dir.lastModified().toString());
        }
    }

    if (!isFound) {
//        qDebug() << QTime::currentTime() << "FolderSizeModel::getCachedDir NOT found " + dir.absoluteFilePath();

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

//    qDebug() << QTime::currentTime() << "FolderSizeModel::getCachedDir done " + cachedDir.name + ", " + QString("%1").arg(cachedDir.size);

    return cachedDir;
}

QString FolderSizeModel::formatFileSize(double size) {
    const uint KB = 1024;
    const uint MB = 1048576;
    const uint GB = 1073741824;

    QString fileSize;

    if (size >= GB) {
        fileSize = QString("%1 GB").arg( int(size/GB) );
    } else if (size >= MB) {
        fileSize = QString("%1 MB").arg( int(size/MB) );
    } else if (size >= 1024) {
        fileSize = QString("%1 KB").arg( int(size/KB) );
    } else {
        fileSize = QString("%1").arg( size );
    }

    return fileSize;
}

void FolderSizeModel::sortItemList() {
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

QList<FolderSizeItem> FolderSizeModel::getDirContent() const
{
    return itemList;
}

void FolderSizeModel::fetchDirSize(const bool clearCache) {
//    qDebug() << QTime::currentTime() << "FolderSizeModel::fetchDirSize started " << currentDir;
    m_isReady = false;

    QDir dir;
    dir = QDir(m_currentDir);
    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoSymLinks | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);

    itemList.clear();

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
            // drilldown to get dir tree size.
            itemList.append(getCachedDir(fileInfo, clearCache));
        } else {
            // do nothing
            itemList.append(getFileItem(fileInfo));
        }
    }

    // Sort itemList as current sortFlag.
    sortItemList();

    // Refresh Cache for the currentDir.
    QFileInfo dirInfo(m_currentDir);
//    qDebug() << "Refresh cache dirInfo.absoluteFilePath() " << dirInfo.absoluteFilePath();
//    dirSizeCache.remove(dirInfo.absoluteFilePath());
    getCachedDir(dirInfo, clearCache);

    m_isReady = true;
}

FolderSizeItem FolderSizeModel::getFileItem(const QFileInfo fileInfo) {
    // FileItem always get isDir=false, subDirCount=0 and subFileCount=0
    FolderSizeItem item = FolderSizeItem(fileInfo.fileName(),
                                         fileInfo.absoluteFilePath(),
                                         fileInfo.lastModified(),
                                         fileInfo.size(),
                                         false, 0, 0);
    return item;
}

void FolderSizeModel::setCurrentDir(const QString currentDir)
{
    m_currentDir = currentDir;
}

int FolderSizeModel::sortFlag() const
{
    return m_sortFlag;
}

bool FolderSizeModel::setSortFlag(int sortFlag)
{
    qDebug() << "FolderSizeModel::setSortFlag m_sortFlag " << m_sortFlag << " sortFlag " << sortFlag;
    if (m_sortFlag != sortFlag) {
        m_sortFlag = sortFlag;

        sortItemList();
        return true;
    } else {
        return false;
    }
}

bool FolderSizeModel::changeDir(const QString dirName)
{
    QDir dir = QDir(m_currentDir);
    if (dir.isRoot() && dirName == "..") {
        qDebug() << "FolderSizeModel::changeDir " << m_currentDir << " is root. Changing to parent folder is aborted.";
        return false;
    } else {
        dir.cd(dirName);
        setCurrentDir(dir.absolutePath());
        return true;
    }
}

void FolderSizeModel::removeItem(const int index)
{
    itemList.removeAt(index);
}

bool FolderSizeModel::isReady()
{
    return m_isReady;
}

bool FolderSizeModel::isRoot()
{
    QDir dir(m_currentDir);
    return (dir.isRoot());
}

bool FolderSizeModel::clearCache()
{
    return m_clearCache;
}

void FolderSizeModel::setClearCache(bool clearCache)
{
    m_clearCache = clearCache;
}

void FolderSizeModel::run()
{
    fetchDirSize(m_clearCache);
}
