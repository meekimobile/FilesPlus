#include "foldersizeitemlistmodel.h"

const int FolderSizeItemListModel::TimerInterval = 100;

FolderSizeItemListModel::FolderSizeItemListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // This map will be used when populating property to delegate.
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[AbsolutePathRole] = "absolutePath";
    roles[SizeRole] = "size";
    roles[LastModifiedRole] = "lastModified";
    roles[IsDirRole] = "isDir";
    roles[SubDirCountRole] = "subDirCount";
    roles[SubFileCountRole] = "subFileCount";
    roles[FileTypeRole] = "fileType";
    roles[IsRunningRole] = "isRunning";
    roles[RunningValueRole] = "runningValue";
    roles[RunningMaxValueRole] = "runningMaxValue";
    setRoleNames(roles);

    // Connect model class with listModel.
    connect(&m, SIGNAL(loadDirSizeCacheFinished()), this, SLOT(postLoadSlot()) );
    connect(&m, SIGNAL(fetchDirSizeStarted()), this, SIGNAL(fetchDirSizeStarted()) );
    connect(&m, SIGNAL(fetchDirSizeFinished()), this, SLOT(postFetchSlot()) );
    connect(&m, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)), this, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)) );
    connect(&m, SIGNAL(copyFinished(int,QString,QString,QString)), this, SIGNAL(copyFinished(int,QString,QString,QString)) );
    connect(&m, SIGNAL(fetchDirSizeUpdated(QString)), this, SIGNAL(fetchDirSizeUpdated(QString)) );

    // Load cache
    m.setRunMethod(m.LoadDirSizeCache);
    m.start();
}

FolderSizeItemListModel::~FolderSizeItemListModel()
{
    // Save cache
    m.saveDirSizeCache();
}

int FolderSizeItemListModel::rowCount(const QModelIndex & parent) const {
//    qDebug() << "FolderSizeItemListModel::rowCount " + QString("%1").arg(m.getDirContent().count());

    return m.getItemList().count();
}

int FolderSizeItemListModel::columnCount(const QModelIndex &parent) const
{
    return roleNames().count();
}

QVariant FolderSizeItemListModel::data(const QModelIndex & index, int role) const {
//    qDebug() << "FolderSizeItemListModel::data " + QString("%1 %2").arg(index.row()).arg(role);

    if (index.row() < 0 || index.row() > m.getItemList().count())
        return QVariant();

    const FolderSizeItem &item = m.getItemList().at(index.row());
    if (role == NameRole)
        return item.name;
    else if (role == AbsolutePathRole)
        return item.absolutePath;
    else if (role == SizeRole)
        return item.size;
    else if (role == LastModifiedRole)
        return item.lastModified;
    else if (role == IsDirRole)
        return item.isDir;
    else if (role == SubDirCountRole)
        return item.subDirCount;
    else if (role == SubFileCountRole)
        return item.subFileCount;
    else if (role == FileTypeRole)
        return item.fileType;
    else if (role == IsRunningRole)
        return item.isRunning;
    else if (role == RunningValueRole)
        return item.runningValue;
    else if (role == RunningMaxValueRole)
        return item.runningMaxValue;
    return QVariant();
}

QString FolderSizeItemListModel::currentDir() const
{
    return m.currentDir();
}

void FolderSizeItemListModel::setCurrentDir(const QString &path)
{
    m.setCurrentDir(path);

    // Emit to update currentDir on UI.
    emit currentDirChanged();

    // Invoke background refresh
    refreshDir(false);

    // Invoke refreshItems to emit dataChanged.
    refreshItems();

    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::setCurrentDir";
}

QStringList FolderSizeItemListModel::getDriveList()
{
    QFileInfoList drives = QDir::drives();
    QStringList driveList;

    for (int i=0; i<drives.length(); i++) {
        driveList.append(drives.at(i).absolutePath());
    }

    return driveList;
}

bool FolderSizeItemListModel::isDirSizeCacheExisting()
{
    return m.isDirSizeCacheExisting();
}

bool FolderSizeItemListModel::isReady()
{
    return !m.isRunning();
}

QVariant FolderSizeItemListModel::getProperty(const int index, FolderSizeItemListModel::FolderSizeItemRoles role)
{
    if (index >= 0 && index < rowCount()) {
        return data(createIndex(index,0), role);
    }

    return QVariant();
}

void FolderSizeItemListModel::setProperty(const int index, FolderSizeItemListModel::FolderSizeItemRoles role, QVariant value)
{
    if (index >= 0 && index < rowCount()) {
        // Update to limited properties.
        FolderSizeItem item = m.getItem(index);
        if (role == IsRunningRole)
            item.isRunning = value.toBool();
        else if (role == RunningValueRole)
            item.runningValue = value.toLongLong();
        else if (role == RunningMaxValueRole)
            item.runningMaxValue = value.toLongLong();

        m.setItem(index, item);

        refreshItem(index);
    } else {
        // TODO do nothing.
    }
}

void FolderSizeItemListModel::setProperty(const QString localPath, FolderSizeItemListModel::FolderSizeItemRoles role, QVariant value)
{
    // Find index for localPath on currentDir.
    int modelIndex = getIndexOnCurrentDir(localPath);
//    qDebug() << "FolderSizeItemListModel::setProperty" << modelIndex;

    if (modelIndex > -1) {
        setProperty(modelIndex, role, value);
    } else if (modelIndex == -1) {
        // Do nothing as it's not on currentDir.
    } else if (modelIndex == -2) {
        refreshItems();
    }
}

QString FolderSizeItemListModel::formatFileSize(double size)
{
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

void FolderSizeItemListModel::changeDir(const QString &name)
{
    // m.changeDir() reuses m.setCurrentDir().
    bool isDirChanged = m.changeDir(name);

    if (isDirChanged) {
        // Emit to update currentDir on UI.
        emit currentDirChanged();

        // Invoke background refresh
        refreshDir(false);

        // Invoke refreshItems to emit dataChanged.
        refreshItems();
    }
}

void FolderSizeItemListModel::refreshDir(const bool clearCache)
{
    qDebug() << "FolderSizeItemListModel::refreshDir clearCache" << clearCache;

    if (isReady()) {
        if (!clearCache && !isDirSizeCacheExisting()) {
            // If UI invoke general refresh but there is no existing cache, ask user for confirmation.
            emit requestResetCache();
        } else {
            // If UI invoke reset cache or their is existing cache, proceed as user requested.
            m.setClearCache(clearCache);
            m.setRunMethod(m.FetchDirSize);
            m.start();
        }
    } else {
        qDebug() << "FolderSizeItemListModel::refreshDir is not ready. Refresh itemList as-is.";
    }

    // Populate and sort directory content to itemList. Then respond to UI.
    m.refreshItemList();
}

QString FolderSizeItemListModel::getUrl(const QString absPath)
{
    return QUrl::fromLocalFile(absPath).toString();
}

bool FolderSizeItemListModel::isRoot()
{
    QDir dir(m.currentDir());
    return (dir.isRoot());
}

int FolderSizeItemListModel::getSortFlag() const
{
    return m.sortFlag();
}

void FolderSizeItemListModel::setSortFlag(const int sortFlag)
{
    if (m.setSortFlag(sortFlag)) {
        // If itemList is actually sorted, refreshItems to emit dataChanged.
        refreshItems();
    }
}

void FolderSizeItemListModel::refreshItems()
{
    beginResetModel();
    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, columnCount()-1));
    endResetModel();
}

void FolderSizeItemListModel::refreshItem(const int index)
{
    emit dataChanged(createIndex(index,0), createIndex(index, columnCount()-1));
}

bool FolderSizeItemListModel::removeRow(int row, const QModelIndex &parent)
{
    return removeRows(row, 1, parent);
}

bool FolderSizeItemListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    bool res = true;
    for (int i = row; i < row+count; i++) {
        QString filePath = data(this->index(i,0), AbsolutePathRole).toString();
        bool iRes = false;
        QFileInfo fileInfo(filePath);

        if (fileInfo.isFile()) {
            // Remove file.
            QFile file(fileInfo.absoluteFilePath());
            iRes = file.remove();
        } else if (fileInfo.isDir()) {
            // Remove dir.
            QDir parentDir = fileInfo.dir();
            iRes = parentDir.rmdir(fileInfo.fileName());
        }

        // Remove item from model's itemList.
        if (iRes) {
            beginRemoveRows(parent, i, i);

            // Remote item from itemList.
            m.removeItem(i);
            // Remove deleted file path cache.
            m.removeDirSizeCache(fileInfo.absolutePath());

            endRemoveRows();

            qDebug() << "FolderSizeItemListModel::removeRows" << fileInfo.absoluteFilePath() << " has been removed.";
        } else {
            qDebug() << "FolderSizeItemListModel::removeRows" << fileInfo.absoluteFilePath() << " can't' be removed because it's not empty.";
        }

        res = res && iRes;
    }

    return res;
}

bool FolderSizeItemListModel::deleteFile(const QString absPath)
{
    QFile file(absPath);
    QFileInfo fileInfo(file);

    // Remove file from file system.
    bool res = file.remove();

    if (res) {
        // Remove deleted file path cache.
        m.removeDirSizeCache(fileInfo.absolutePath());
    }

    return res;
}

bool FolderSizeItemListModel::copyFile(const QString sourceAbsFilePath, const QString targetPath)
{
    m.setRunMethod(m.CopyFile);
    m.setCopyPath(sourceAbsFilePath, targetPath);
    m.start();

    return true;
}

bool FolderSizeItemListModel::moveFile(const QString sourceAbsFilePath, const QString targetPath)
{
    m.setRunMethod(m.MoveFile);
    m.setCopyPath(sourceAbsFilePath, targetPath);
    m.start();

    return true;
}

bool FolderSizeItemListModel::createDir(const QString name)
{
    QDir dir(currentDir());
    return dir.mkdir(name);
}

QString FolderSizeItemListModel::getDirPath(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.absolutePath();
}

bool FolderSizeItemListModel::isDir(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.isDir();
}

bool FolderSizeItemListModel::isFile(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.isFile();
}

bool FolderSizeItemListModel::canCopy(const QString sourceAbsFilePath, const QString targetPath)
{
    QString targetAbsFilePath;
    QFileInfo sourceFileInfo(sourceAbsFilePath);
    QFileInfo targetFileInfo(targetPath);
    if (targetFileInfo.isDir()) {
        targetAbsFilePath = QDir(targetFileInfo.absoluteFilePath()).absoluteFilePath(sourceFileInfo.fileName());
    } else {
        targetAbsFilePath = targetPath;
    }
    targetFileInfo = QFileInfo(targetAbsFilePath);

    return !targetFileInfo.exists();
}

QString FolderSizeItemListModel::getFileName(const QString absFilePath)
{
    QFileInfo fileInfo(absFilePath);

    return fileInfo.fileName();
}

QString FolderSizeItemListModel::getNewFileName(const QString absFilePath)
{
    // Get new name as *Copy.* , *Copy 2.*, ...
    QFileInfo file(absFilePath);
    QStringList caps = splitFileName(file.absoluteFilePath());
    int copyIndex = caps.at(0).lastIndexOf("Copy");
    QString originalFileName = (copyIndex != -1) ? caps.at(0).left(copyIndex) : caps.at(0);
    QString originalFileExtension = caps.at(1);

    int i = 1;
    while (file.exists()) {
        QString newFilePath;
        if (i == 1) {
            newFilePath = originalFileName + QString(" %1.").arg("Copy") + originalFileExtension;
        } else {
            newFilePath = originalFileName + QString(" Copy %1.").arg(i) + originalFileExtension;
        }
        file = QFileInfo(newFilePath);
        i++;
    }

    return file.fileName();
}

QString FolderSizeItemListModel::getAbsolutePath(const QString dirPath, const QString fileName)
{
    QDir dir(dirPath);
    return dir.absoluteFilePath(fileName);
}

QStringList FolderSizeItemListModel::splitFileName(const QString fileName)
{
    // Parse fileName with RegExp
    QRegExp rx("(.+)(\\.)(\\w{3,4})$");
    rx.indexIn(fileName);

    QStringList caps;
    caps << rx.cap(1) << rx.cap(3);

    return caps;
}

QString FolderSizeItemListModel::getDirContentJson(const QString dirPath)
{
    QList<FolderSizeItem> itemList = m.getDirContent(dirPath);
    QString jsonText = "[ ";
    for (int i=0; i<itemList.length(); i++) {
        FolderSizeItem item = itemList.at(i);
        if (i>0) jsonText.append(", ");
        jsonText.append(item.toJsonText());
    }
    jsonText.append(" ]");

    return jsonText;
}

int FolderSizeItemListModel::getIndexOnCurrentDir(const QString absFilePath)
{
    bool isOnCurrentDir = false;
    if (getDirPath(absFilePath) == currentDir()) {
        isOnCurrentDir = true;
        int i=0;
        foreach (FolderSizeItem item, m.getItemList()) {
            if (item.absolutePath == absFilePath) {
                return i;
            }
            i++;
        }
    }

    return (isOnCurrentDir)?-2:-1;
}

void FolderSizeItemListModel::removeCache(const QString absPath)
{
    m.removeDirSizeCache(absPath);
}

bool FolderSizeItemListModel::isRunning()
{
    return m.isRunning();
}

void FolderSizeItemListModel::postLoadSlot()
{
    refreshDir();
}

void FolderSizeItemListModel::postFetchSlot()
{
    emit refreshCompleted();
    refreshItems();
    emit fetchDirSizeFinished();
}
