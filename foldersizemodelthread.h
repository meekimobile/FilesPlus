#ifndef FOLDERSIZEMODELTHREAD_H
#define FOLDERSIZEMODELTHREAD_H

#include "foldersizeitem.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QThread>

class FolderSizeModelThread : public QThread
{
    Q_OBJECT
public:
    static const QString CACHE_FILE_PATH;
    static const QString DEFAULT_CURRENT_DIR;

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
        MoveFile
    };

    FolderSizeModelThread(QObject *parent = 0);

    QString currentDir() const;
    void setCurrentDir(const QString currentDir);
    int sortFlag() const;
    bool setSortFlag(int sortFlag);
    QStringList nameFilters() const;
    void setNameFilters(const QStringList nameFilters);
    bool clearCache();
    void setClearCache(bool clearCache);

    // Thread methods.
    void fetchDirSize(const bool clearCache = false);
    void loadDirSizeCache();
    void saveDirSizeCache();
    bool copy(int method, const QString sourcePath, const QString targetPath);
    bool copyFile(int method, const QString sourcePath, const QString targetPath);
    bool deleteDir(const QString targetPath);

    void getDirContent(const QString dirPath, QList<FolderSizeItem> &itemList);
    void sortItemList(QList<FolderSizeItem> &itemList);
    bool isDirSizeCacheExisting();
    void removeDirSizeCache(const QString key);
    bool changeDir(const QString dirName);

    void setRunMethod(int method);
    void setCopyPath(const QString sourcePath, const QString targetPath);
    void run();
signals:
    void loadDirSizeCacheFinished();
    void fetchDirSizeStarted();
    void fetchDirSizeFinished();
    void copyStarted(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void copyProgress(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinished(int fileAction, QString sourcePath, QString targetPath, QString msg, int err);
    void fetchDirSizeUpdated(QString dirPath);
    void deleteFinished(QString localPath);
private:
    FolderSizeItem getCachedDir(const QFileInfo dir, const bool clearCache = false);
    FolderSizeItem getItem(const QFileInfo fileInfo) const;
    FolderSizeItem getFileItem(const QFileInfo fileInfo) const;
    FolderSizeItem getDirItem(const QFileInfo dirInfo) const;

    QHash<QString, FolderSizeItem> dirSizeCache;
    QString m_currentDir;
    bool m_clearCache;
    int m_sortFlag;
    QStringList m_nameFilters;

    int m_runMethod;
    QString m_sourcePath;
    QString m_targetPath;
};

#endif // FOLDERSIZEMODELTHREAD_H
