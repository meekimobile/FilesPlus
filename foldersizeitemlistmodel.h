#ifndef FOLDERSIZEITEMLISTMODEL_H
#define FOLDERSIZEITEMLISTMODEL_H

#include <QAbstractListModel>
#include <QTimer>
#include <QtCore>
#include <QDebug>
#include <QThreadPool>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include "foldersizeitem.h"
#include "foldersizemodelthread.h"

class FolderSizeItemListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(SortFlags)
    Q_ENUMS(FolderSizeItemRoles)
    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(int sortFlag READ getSortFlag WRITE setSortFlag)
    Q_PROPERTY(int count READ rowCount)
public:
    static const int TimerInterval;

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType
    };

    enum FolderSizeItemRoles {
        NameRole = Qt::UserRole + 1,
        AbsolutePathRole,
        SizeRole,
        LastModifiedRole,
        IsDirRole,
        SubDirCountRole,
        SubFileCountRole,
        FileTypeRole,
        IsRunningRole,
        RunningValueRole,
        RunningMaxValueRole
    };

    explicit FolderSizeItemListModel(QObject *parent = 0);
    ~FolderSizeItemListModel();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    QString currentDir() const;
    void setCurrentDir(const QString &path);
    int getSortFlag() const;
    void setSortFlag(const int sortFlag);
    Q_INVOKABLE QVariant getProperty(const int index, FolderSizeItemRoles role);
    Q_INVOKABLE void setProperty(const int index, FolderSizeItemRoles role, QVariant value);
    Q_INVOKABLE void setProperty(const QString localPath, FolderSizeItemRoles role, QVariant value);

    Q_INVOKABLE void refreshDir(const bool clearCache = false);
    Q_INVOKABLE void changeDir(const QString &name);
    Q_INVOKABLE QString getUrl(const QString absPath);
    Q_INVOKABLE bool isRoot();
    Q_INVOKABLE QString getDirContentJson(const QString dirPath);
    Q_INVOKABLE int getIndexOnCurrentDir(const QString absFilePath);
    Q_INVOKABLE void removeCache(const QString absPath);
    Q_INVOKABLE bool isRunning();

    // File/Dir manipulation methods.
    Q_INVOKABLE bool removeRow(int row, const QModelIndex & parent = QModelIndex());
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    Q_INVOKABLE bool deleteFile(const QString absPath);
    Q_INVOKABLE bool copyFile(const QString sourceAbsFilePath, const QString targetPath);
    Q_INVOKABLE bool moveFile(const QString sourceAbsFilePath, const QString targetPath);
    Q_INVOKABLE bool createDir(const QString name);

    // Informative methods which don't use FolderSizeModel.
    Q_INVOKABLE void refreshItems();
    Q_INVOKABLE void refreshItem(const int index);
    Q_INVOKABLE QString getDirPath(const QString absFilePath);
    Q_INVOKABLE QStringList getPathToRoot(const QString absFilePath);
    Q_INVOKABLE bool isDir(const QString absFilePath);
    Q_INVOKABLE bool isFile(const QString absFilePath);
    Q_INVOKABLE bool canCopy(const QString sourceAbsFilePath, const QString targetPath);
    Q_INVOKABLE QString getFileName(const QString absFilePath);
    Q_INVOKABLE QString getNewFileName(const QString absFilePath);
    Q_INVOKABLE QString getAbsolutePath(const QString dirPath, const QString fileName);
    Q_INVOKABLE QStringList getDriveList();
    Q_INVOKABLE QString formatFileSize(double size);
private:
    Q_DISABLE_COPY(FolderSizeItemListModel)
    FolderSizeModelThread m;
    QTimer *timer;

    bool isDirSizeCacheExisting();
    bool isReady();
    QStringList splitFileName(const QString fileName);
public slots:
    void postLoadSlot();
    void postFetchSlot();
Q_SIGNALS:
    void currentDirChanged();
    void refreshBegin();
    void refreshCompleted();
    void requestResetCache();
    void copyProgress(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinished(int fileAction, QString sourcePath, QString targetPath, QString msg);
    void deleteFinished(QString targetPath);
    void createFinished(QString targetPath);
    void fetchDirSizeStarted();
    void fetchDirSizeFinished();
    void fetchDirSizeUpdated(QString dirPath);
};

#endif // FOLDERSIZEITEMLISTMODEL_H
