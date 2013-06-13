#ifndef COMPRESSEDFOLDERMODELWORKER_H
#define COMPRESSEDFOLDERMODELWORKER_H

#include <QObject>
#include <QRunnable>
#include <QStringList>
#include "compressedfoldermodelitem.h"

class CompressedFolderModelWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit CompressedFolderModelWorker(QObject *parent = 0);

    enum Actions {
        List,
        Compress,
        Extract
    };

    Actions action;
    QString compressedFilePath;
    QStringList sourcePathList;
    QString sourcePath;
    QString targetParentPath;
    QString targetPath;
    QList<CompressedFolderModelItem> *itemList;

    void run();
signals:
    void compressStarted(QString compressedFilePath);
    void compressFinished(QString compressedFilePath, int err);
    void extractStarted(QString compressedFilePath);
    void extractFinished(QString compressedFilePath, int err, QStringList extractedFileList);
    void listStarted(QString compressedFilePath);
    void listFinished(QString compressedFilePath, int err);
    void finished();
public slots:
    void start();
private:
    int m_listErrorCode;

    QStringList list(QString compressedFilePath);
    bool compress(QString compressedFilePath, QString sourcePath);
    bool compress(QString compressedFilePath, QStringList sourceFilePathList);
    QString extract(QString compressedFilePath, QString sourcePath, QString targetPath = "");
    QStringList extract(QString compressedFilePath, QStringList sourcePathList, QString targetParentPath = "");

    QString getDefaultCompressedFilePath(QString sourcePath);
    char *getCharPointer(QString s);
};

#endif // COMPRESSEDFOLDERMODELWORKER_H
