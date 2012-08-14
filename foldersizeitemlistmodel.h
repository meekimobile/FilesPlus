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
#include <qsystemstorageinfo.h>
#include "foldersizeitem.h"
#include "foldersizemodelthread.h"
#include "foldersizejob.h"

using namespace QtMobility;

class FolderSizeItemListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(SortFlags)
    Q_ENUMS(RunnableMethods)
    Q_ENUMS(FolderSizeItemRoles)
    Q_ENUMS(RunningOperations)
    Q_ENUMS(IndexOnCurrentDir)
    Q_PROPERTY(QString currentDir READ currentDir WRITE setCurrentDir NOTIFY currentDirChanged)
    Q_PROPERTY(int sortFlag READ getSortFlag WRITE setSortFlag)
    Q_PROPERTY(int count READ rowCount)
    Q_PROPERTY(QStringList nameFilters READ getNameFilters WRITE setNameFilters)
public:
    static const int TimerInterval;
    static const int MaxRunningJobCount;

    enum SortFlags {
        SortByName,
        SortByTime,
        SortBySize,
        SortByType
    };

    enum RunnableMethods {
        FetchDirSize,
        LoadDirSizeCache,
        CopyFile,
        MoveFile,
        DeleteFile
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
        RunningOperationRole,
        RunningValueRole,
        RunningMaxValueRole,
        IsCheckedRole,
        IsDirtyRole
    };

    enum RunningOperations {
        NoOperation,
        ReadOperation,
        WriteOperation,
        SyncOperation,
        UploadOperation,
        DownloadOperation
    };

    enum IndexOnCurrentDir {
        IndexNotOnCurrentDir = -1,
        IndexOnCurrentDirButNotFound = -2
    };

    explicit FolderSizeItemListModel(QObject *parent = 0);
    ~FolderSizeItemListModel();

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    int columnCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    QString currentDir() const;
    void setCurrentDir(const QString &path);
    int getSortFlag() const;
    Q_INVOKABLE void setSortFlag(const int sortFlag, const bool saveSortFlag = true);
    Q_INVOKABLE void revertSortFlag();
    QStringList getNameFilters() const;
    void setNameFilters(const QStringList nameFilters);
    Q_INVOKABLE QVariant getProperty(const int index, FolderSizeItemRoles role);
    Q_INVOKABLE void setProperty(const int index, FolderSizeItemRoles role, QVariant value);
    Q_INVOKABLE void setProperty(const QString localPath, FolderSizeItemRoles role, QVariant value);

    Q_INVOKABLE void refreshDir(const QString caller, const bool clearCache = false);
    Q_INVOKABLE void changeDir(const QString &name, const int sortFlag = -1);
    Q_INVOKABLE QString getUrl(const QString absPath);
    Q_INVOKABLE bool isRoot(); // Overload method for verifying on currentDir.
    Q_INVOKABLE bool isRoot(const QString absPath);
    Q_INVOKABLE QString getItemJson(const QString absFilePath);
    Q_INVOKABLE QString getDirContentJson(const QString dirPath);
    Q_INVOKABLE int getIndexOnCurrentDir(const QString absFilePath);
    Q_INVOKABLE void removeCache(const QString absPath);
    Q_INVOKABLE bool isRunning();

    // File/Dir manipulation methods.
    Q_INVOKABLE bool removeRow(int row, const QModelIndex & parent = QModelIndex());
    Q_INVOKABLE bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    Q_INVOKABLE bool deleteFile(const QString absPath);
    Q_INVOKABLE bool copy(const QString sourcePath, const QString targetPath);
    Q_INVOKABLE bool move(const QString sourcePath, const QString targetPath);
    Q_INVOKABLE bool createDir(const QString name);
    Q_INVOKABLE bool createDirPath(const QString absPath);
    Q_INVOKABLE bool renameFile(const QString fileName, const QString newFileName);

    // Informative methods which don't use FolderSizeModelThread.
    Q_INVOKABLE void refreshItems();
    Q_INVOKABLE void refreshItem(const int index);
    Q_INVOKABLE void refreshItem(const QString localPath);
    Q_INVOKABLE QString getDirPath(const QString absFilePath);
    Q_INVOKABLE QStringList getPathToRoot(const QString absFilePath);
    Q_INVOKABLE bool isDir(const QString absFilePath);
    Q_INVOKABLE bool isFile(const QString absFilePath);
    Q_INVOKABLE bool canCopy(const QString sourcePath, const QString targetPath);
    Q_INVOKABLE QString getFileName(const QString absFilePath);
    Q_INVOKABLE QString getNewFileName(const QString absFilePath, const QString targetPath);
    Q_INVOKABLE QString getAbsolutePath(const QString dirPath, const QString fileName);
    Q_INVOKABLE QStringList getDriveList();
    Q_INVOKABLE QStringList getLogicalDriveList();
    Q_INVOKABLE QString formatFileSize(double size);
    Q_INVOKABLE void cancelQueuedJobs();
    Q_INVOKABLE int getQueuedJobCount() const;
    Q_INVOKABLE void abortThread(bool rollbackFlag = true);

    QList<FolderSizeItem> getItemList() const;
    void removeItem(const int index);
    FolderSizeItem getItem(const int index);
    void setItem(const int index, FolderSizeItem &item);
private:
    Q_DISABLE_COPY(FolderSizeItemListModel)
    FolderSizeModelThread m;
    QTimer *timer;

    QQueue<FolderSizeJob> m_jobQueue;
    int runningJobCount;
    QString createNonce();

    QList<FolderSizeItem> itemList;
    void refreshItemList();

    bool isDirSizeCacheExisting();
    bool isReady();
    QStringList splitFileName(const QString fileName);

    QCache<QString, QString> *m_pathToRootCache;
    QHash<QString, int> *m_indexOnCurrentDirHash;

    QStringList m_driveList;

    QMutex mutex;
public slots:
    void loadDirSizeCacheFinishedFilter();
    void fetchDirSizeFinishedFilter();
    void copyProgressFilter(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinishedFilter(int fileAction, QString sourcePath, QString targetPath, QString msg, int err, qint64 bytes, qint64 totalBytes);
    void deleteProgressFilter(int fileAction, QString sourceSubPath, QString msg, int err);
    void deleteFinishedFilter(int fileAction, QString sourcePath, QString msg, int err);
    void proceedNextJob();
    void jobDone();
Q_SIGNALS:
    void currentDirChanged();
    void refreshBegin();
    void refreshCompleted();
    void requestResetCache();
    void copyStarted(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void copyProgress(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinished(int fileAction, QString sourcePath, QString targetPath, QString msg, int err, qint64 bytes, qint64 totalBytes);
    void deleteStarted(int fileAction, QString sourcePath);
    void deleteProgress(int fileAction, QString sourceSubPath, QString msg, int err);
    void deleteFinished(int fileAction, QString sourcePath, QString msg, int err);
    void createFinished(QString targetPath);
    void renameFinished(QString sourcePath, QString targetPath, QString msg, int err);
    void fetchDirSizeStarted();
    void fetchDirSizeFinished();
    void fetchDirSizeUpdated(QString dirPath);
    void proceedNextJobSignal();
};

#endif // FOLDERSIZEITEMLISTMODEL_H
