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
    bool clearCache();
    void setClearCache(bool clearCache);

    // Thread methods.
    void fetchDirSize(const bool clearCache = false);
    void loadDirSizeCache();
    void saveDirSizeCache();
    bool copyFile(int method, const QString sourcePath, const QString targetPath);
    bool copy(int method, const QString sourcePath, const QString targetPath);
    bool deleteDir(const QString targetPath);

    void refreshItemList();
    QList<FolderSizeItem> getItemList() const;
    QList<FolderSizeItem> getDirContent(const QString dirPath);
    bool isDirSizeCacheExisting();
    void removeDirSizeCache(const QString key);
    bool changeDir(const QString dirName);
    FolderSizeItem getItem(const int index);
    void setItem(const int index, FolderSizeItem &item);
    void removeItem(const int index);

    void setRunMethod(int method);
    void setCopyPath(const QString sourcePath, const QString targetPath);
    void run();
signals:
    void loadDirSizeCacheFinished();
    void fetchDirSizeStarted();
    void fetchDirSizeFinished();
    void copyProgress(int fileAction, QString sourcePath, QString targetPath, qint64 bytes, qint64 bytesTotal);
    void copyFinished(int fileAction, QString sourcePath, QString targetPath, QString msg);
    void fetchDirSizeUpdated(QString dirPath);
    void deleteFinished(QString localPath);
private:
    FolderSizeItem getCachedDir(const QFileInfo dir, const bool clearCache = false);
    FolderSizeItem getFileItem(const QFileInfo fileInfo);
    FolderSizeItem getDirItem(const QFileInfo dirInfo);
    void sortItemList(QList<FolderSizeItem> &itemList);

    QHash<QString, FolderSizeItem> dirSizeCache;
    QList<FolderSizeItem> itemList;
    QString m_currentDir;
    bool m_clearCache;
    int m_sortFlag;

    int m_runMethod;
    QString m_sourcePath;
    QString m_targetPath;
};

#endif // FOLDERSIZEMODELTHREAD_H
