#ifndef FOLDERSIZEJOB_H
#define FOLDERSIZEJOB_H

#include <QtCore>

class FolderSizeJob
{
public:
    FolderSizeJob();
    FolderSizeJob(QString jobId, int operation, QString sourcePath, QString targetPath);

    QString jobId;
    int operation;
    QString sourcePath;
    QString targetPath;
    bool isRunning;
    qint64 bytes;
    qint64 bytesTotal;

    QString toJsonText();
};

#endif // FOLDERSIZEJOB_H
