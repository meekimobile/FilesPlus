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
    connect(&m, SIGNAL(fetchDirSizeFinished()), this, SLOT(postFetchSlot()) );
    connect(&m, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)), this, SIGNAL(copyProgress(int,QString,QString,qint64,qint64)) );
    connect(&m, SIGNAL(copyFinished(int,QString,QString,QString)), this, SIGNAL(copyFinished(int,QString,QString,QString)) );

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

    return m.getDirContent().count();
}

int FolderSizeItemListModel::columnCount(const QModelIndex &parent) const
{
    return roleNames().count();
}

QVariant FolderSizeItemListModel::data(const QModelIndex & index, int role) const {
//    qDebug() << "FolderSizeItemListModel::data " + QString("%1 %2").arg(index.row()).arg(role);

    if (index.row() < 0 || index.row() > m.getDirContent().count())
        return QVariant();

    const FolderSizeItem &item = m.getDirContent().at(index.row());
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
    if (isReady()) {
        m.setCurrentDir(path);

        emit currentDirChanged();

        // Invoke background refresh
        refreshDir(false);
    }
    qDebug() << QTime::currentTime() << "FolderSizeItemListModel::setCurrentDir";
}

QStringList FolderSizeItemListModel::getDriveList()
{
    return m.getDriveList();
}

bool FolderSizeItemListModel::isDirSizeCacheExisting()
{
    return m.isDirSizeCacheExisting();
}

bool FolderSizeItemListModel::isReady()
{
    return m.isReady();
}

QVariant FolderSizeItemListModel::getProperty(const int index, FolderSizeItemListModel::FolderSizeItemRoles role)
{
    if (isReady() && index >= 0 && index < rowCount()) {
        return data(createIndex(index,0), role);
    }

    return QVariant();
}

void FolderSizeItemListModel::setProperty(const int index, FolderSizeItemListModel::FolderSizeItemRoles role, QVariant value)
{
    if (isReady() && index >= 0 && index < rowCount()) {
        // Update to limited properties.
        FolderSizeItem item = m.getItem(index);
        if (role == IsRunningRole)
            item.isRunning = value.toBool();
        else if (role == RunningValueRole)
            item.runningValue = value.toLongLong();
        else if (role == RunningMaxValueRole)
            item.runningMaxValue = value.toLongLong();

        m.updateItem(index, item);

        emit dataChanged(createIndex(index,0), createIndex(index, columnCount()));
    } else {

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
    return m.formatFileSize(size);
}

void FolderSizeItemListModel::changeDir(const QString &name)
{
    if (isReady()) {
        bool isDirChanged = m.changeDir(name);
        if (isDirChanged) {
            emit currentDirChanged();

            // Invoke background refresh
            refreshDir(false);
        }
    }
}

void FolderSizeItemListModel::refreshDir(const bool clearCache)
{
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
        qDebug() << "FolderSizeItemListModel::refreshDir is not ready.";
    }
}

QString FolderSizeItemListModel::getUrl(const QString absPath)
{
    return QUrl::fromLocalFile(absPath).toString();
}

bool FolderSizeItemListModel::isRoot()
{
    return m.isRoot();
}

int FolderSizeItemListModel::getSortFlag() const
{
    return m.sortFlag();
}

void FolderSizeItemListModel::setSortFlag(const int sortFlag)
{
    if (m.setSortFlag(sortFlag)) {
        // If itemList is actually sorted, emit dataChanged.
        emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
    }
}

void FolderSizeItemListModel::refreshItems()
{
    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, 0));
}

void FolderSizeItemListModel::refreshItem(const int index)
{
    emit dataChanged(createIndex(index,0), createIndex(index, 0));
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
    // TODO need to fix to get new name as *Copy.* , *Copy 2.*, ...
    QFileInfo file(absFilePath);
    int i = 1;
    while (file.exists()) {
        QStringList caps = splitFileName(file.absoluteFilePath());
        QString newFilePath;
        if (i == 1) {
            newFilePath = caps.at(0) + " Copy." + caps.at(1);
        } else {
            newFilePath = caps.at(0) + QString(" %2").arg(i) + "." + caps.at(1);
        }
        file = QFileInfo(newFilePath);
        i++;
    }

    return file.fileName();
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
        foreach (FolderSizeItem item, m.getDirContent()) {
            if (item.absolutePath == absFilePath) {
                return i;
            }
            i++;
        }
    }

    return (isOnCurrentDir)?-2:-1;
}

void FolderSizeItemListModel::postLoadSlot()
{
    refreshDir();
}

void FolderSizeItemListModel::postFetchSlot()
{
    emit refreshCompleted();
    reset();
    emit dataChanged(createIndex(0,0), createIndex(rowCount()-1, columnCount()-1));
}
