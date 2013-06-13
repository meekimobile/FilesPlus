#include "compressedfoldermodelworker.h"
#include <JlCompress.h>
#include <QDebug>

CompressedFolderModelWorker::CompressedFolderModelWorker(QObject *parent) :
    QObject(parent)
{
}

void CompressedFolderModelWorker::run()
{
    bool res = false;
    QStringList extractedList;

    switch (action) {
    case Compress:
        emit compressStarted(compressedFilePath);
        if (sourcePathList.count() == 1) {
            res = compress(compressedFilePath, sourcePathList.first());
        } else {
            res = compress(compressedFilePath, sourcePathList);
        }
        emit compressFinished(compressedFilePath, res ? 0 : -1);
        break;
    case Extract:
        emit extractStarted(compressedFilePath);
        qDebug() << "CompressedFolderModelWorker::run extracting started."
                 << "compressedFilePath" << compressedFilePath
                 << "sourcePath" << sourcePath << "targetPath" << targetPath
                 << "sourcePathList" << sourcePathList << "targetParentPath" << targetParentPath;
        if (sourcePath != "" && targetPath != "") {
            extractedList << extract(compressedFilePath, sourcePath, targetPath);
        } else {
            extractedList << extract(compressedFilePath, sourcePathList, targetParentPath);
        }
        qDebug() << "CompressedFolderModelWorker::run extracting is done. extractedList" << extractedList;
        emit extractFinished(compressedFilePath, 0, extractedList);
        break;
    case List:
        emit listStarted(compressedFilePath);
        sourcePathList = list(compressedFilePath);
        emit listFinished(compressedFilePath, 0);
        break;
    }

    emit finished();
}

void CompressedFolderModelWorker::start()
{
    run();
}

QStringList CompressedFolderModelWorker::list(QString compressedFilePath)
{
    if (compressedFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
//        return JlCompress::getFileList(compressedFilePath);

        QuaZip* zip = new QuaZip(QFileInfo(compressedFilePath).absoluteFilePath());
        if (!zip->open(QuaZip::mdUnzip)) {
            delete zip;
            return QStringList();
        }

        itemList->clear();
        QStringList lst;
        QuaZipFileInfo info;
        for (bool more = zip->goToFirstFile(); more; more = zip->goToNextFile()) {
            if (!zip->getCurrentFileInfo(&info)) {
                delete zip;
                return QStringList();
            }

            CompressedFolderModelItem item;
            item.name = info.name;
            item.size = info.uncompressedSize;
            item.compressedSize = info.compressedSize;
            item.lastModified = info.dateTime;
            itemList->append(item);

            lst << info.name;
        }

        zip->close();
        if(zip->getZipError() != 0) {
            delete zip;
            return QStringList();
        }
        delete zip;

        return lst;

    }

    return QStringList();
}

bool CompressedFolderModelWorker::compress(QString compressedFilePath, QString sourcePath)
{
    bool res = false;
    if (compressedFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
        QFileInfo sourceInfo(sourcePath);
        if (sourceInfo.isDir()) {
            res = JlCompress::compressDir(compressedFilePath, sourcePath);
        } else {
            res = JlCompress::compressFile(compressedFilePath, sourcePath);
        }
    }

    return res;
}

bool CompressedFolderModelWorker::compress(QString compressedFilePath, QStringList sourceFilePathList)
{
//    qDebug() << "CompressedFolderModelWorker::compress" << compressedFilePath << sourceFilePathList;

    if (compressedFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
        return JlCompress::compressFiles(compressedFilePath, sourceFilePathList);
    }

    return false;
}

QString CompressedFolderModelWorker::extract(QString compressedFilePath, QString sourcePath, QString targetPath)
{
    if (sourcePath == "") return "";

    if (compressedFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
        return JlCompress::extractFile(compressedFilePath, sourcePath, targetPath);
    }

    return "";
}

QStringList CompressedFolderModelWorker::extract(QString compressedFilePath, QStringList sourcePathList, QString targetParentPath)
{
    if (compressedFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
        if (sourcePathList.empty()) {
//            qDebug() << "CompressedFolderModelWorker::extract extractDir" << compressedFilePath << sourcePathList << targetParentPath;
            return JlCompress::extractDir(compressedFilePath, targetParentPath);
        } else {
//            qDebug() << "CompressedFolderModelWorker::extract extractFiles" << compressedFilePath << sourcePathList << targetParentPath;
            return JlCompress::extractFiles(compressedFilePath, sourcePathList, targetParentPath);
        }
    }

    return QStringList();
}

QString CompressedFolderModelWorker::getDefaultCompressedFilePath(QString sourcePath)
{
    QFileInfo sourceInfo(sourcePath);

    return sourceInfo.absoluteDir().absoluteFilePath(sourceInfo.baseName() + ".zip");
}
