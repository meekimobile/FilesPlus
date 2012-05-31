#ifndef FOLDERSIZEMODEL_H
#define FOLDERSIZEMODEL_H

#include "foldersizeitem.h"
#include <QDir>
#include <QHash>
#include <QRunnable>

class FolderSizeModel : public QRunnable
{
public:
    static const QString CACHE_FILE_PATH;
    static const QString DEFAULT_CURRENT_DIR;


    FolderSizeModel();
    FolderSizeModel(QString currentDir);

    QString currentDir() const;
    void setCurrentDir(const QString currentDir);

    QStringList getDriveList();
    void fetchDirSize(const bool clearCache = false);
    QList<FolderSizeItem> getDirContent() const;
    void loadDirSizeCache();
    void saveDirSizeCache();
    bool isDirSizeCacheExisting();
    void removeDirSizeCache(const QString key);
    QString formatFileSize(double size);
    bool changeDir(const QString dirName);
    void removeItem(const int index);

    bool isReady();

    bool clearCache();
    void setClearCache(bool clearCache);

    void run();
private:
    FolderSizeItem getCachedDir(const QFileInfo dir, const bool clearCache = false);
    FolderSizeItem getFileItem(const QFileInfo fileInfo);

    QHash<QString, FolderSizeItem> dirSizeCache;
    QList<FolderSizeItem> itemList;
    QString m_currentDir;
    bool m_isReady;
    bool m_clearCache;
};

#endif // FOLDERSIZEMODEL_H
