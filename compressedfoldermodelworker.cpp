#include "compressedfoldermodelworker.h"
#include <JlCompress.h>
#include <rar.hpp>
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
        emit listFinished(compressedFilePath, m_listErrorCode);
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
    qDebug() << "CompressedFolderModelWorker::list started." << compressedFilePath;

    // Empty list.
    QStringList itemNameList;
    itemList->clear();
    m_listErrorCode = 0;

    if (compressedFilePath.endsWith(".zip", Qt::CaseInsensitive)) {
        QuaZip* zip = new QuaZip(QFileInfo(compressedFilePath).absoluteFilePath());
        if (!zip->open(QuaZip::mdUnzip)) {
            delete zip;
            m_listErrorCode = -1;
            return QStringList();
        }

        QuaZipFileInfo info;
        for (bool more = zip->goToFirstFile(); more; more = zip->goToNextFile()) {
            if (!zip->getCurrentFileInfo(&info)) {
                delete zip;
                m_listErrorCode = -1;
                return QStringList();
            }

            // Add item to model.
            CompressedFolderModelItem item;
            item.name = info.name;
            item.size = info.uncompressedSize;
            item.compressedSize = info.compressedSize;
            item.lastModified = info.dateTime;
            itemList->append(item);
            itemNameList << info.name;
        }

        zip->close();
        if(zip->getZipError() != 0) {
            delete zip;
            m_listErrorCode = zip->getZipError();
            return QStringList();
        }

        delete zip;
    } else if (compressedFilePath.endsWith(".rar", Qt::CaseInsensitive)) {
        try {
            QString commandLine = "unrar l " + compressedFilePath;

            // NOTE
            // Uses C functions to parse copied command into argv.
            // Copy command into new buffer because data() is allocated by QString.
            char command[1024];
            strcpy(command, commandLine.toLocal8Bit().data());
            int argc = 0;
            char *argv[64]; // Set max arguments as 64.
            char *p2 = strtok(command, " "); // First call with source string.
            while (p2 && argc < 64) {
                argv[argc++] = p2;
                p2 = strtok(0, " "); // Next call with NULL string.
            }

            CommandData *Cmd = new CommandData;
            Cmd->ParseCommandLine(false, argc, argv); // Need to disable Proprocess.
            qDebug() << "CompressedFolderModelWorker::list rar ParseCommandLine"
                     << QString::fromWCharArray(Cmd->Command)
                     << QString::fromWCharArray(Cmd->ArcName);

            // Verify parsed ArcName.
            if (QString::fromWCharArray(Cmd->ArcName).compare(compressedFilePath) != 0) {
                qDebug() << "CompressedFolderModelWorker::list rar ArcName is different.";
                m_listErrorCode = -1;
                return itemNameList;
            }

            Archive Arc(Cmd);
            if (Arc.WOpen(Cmd->ArcName)) {
                qDebug() << "CompressedFolderModelWorker::list rar Arc is opened.";
                bool FileMatched=true;
                while (1) {
                    if (Arc.IsArchive(true)) {
                        qDebug() << "CompressedFolderModelWorker::list rar Arc is an archive.";
                        while(Arc.ReadHeader()>0) {
                            HEADER_TYPE HeaderType=Arc.GetHeaderType();
                            if (HeaderType==HEAD_ENDARC) {
                                qDebug() << "CompressedFolderModelWorker::list rar HEAD_ENDARC";
                                // End of file.
                                break;
                            }

                            switch(HeaderType) {
                            case HEAD_FILE:
                                FileMatched=Cmd->IsProcessFile(Arc.FileHead)!=0;
                                if (FileMatched) {
                                    qDebug() << "CompressedFolderModelWorker::list rar HEAD_FILE" << QString::fromWCharArray(Arc.FileHead.FileName);
                                    // Add item to model.
                                    CompressedFolderModelItem item;
                                    item.name = QString::fromWCharArray(Arc.FileHead.FileName);
                                    item.size = Arc.FileHead.UnpSize;
                                    item.compressedSize = Arc.FileHead.PackSize;
                                    item.lastModified = QDateTime::fromTime_t(Arc.FileHead.mtime.GetUnix());
                                    itemList->append(item);
                                    itemNameList << item.name;
                                }
                                break;
                            case HEAD_SERVICE:
                                if (FileMatched) {
                                    qDebug() << "CompressedFolderModelWorker::list rar HEAD_SERVICE" << QString::fromWCharArray(Arc.FileHead.FileName);
                                }
                                break;
                            }
                            Arc.SeekToNext();
                        }

                        if (Cmd->VolSize != 0
                                && (Arc.FileHead.SplitAfter || Arc.GetHeaderType()==HEAD_ENDARC && Arc.EndArcHead.NextVolume)
                                && MergeArchive(Arc,NULL,false,Cmd->Command[0]) ) {
                            Arc.Seek(0,SEEK_SET);
                        } else {
                            break;
                        }
                    } else {
                        m_listErrorCode = -1;
                        break;
                    } // IsArchive
                } // while (1)
            } else {
                m_listErrorCode = -1;
            } // Arc.WOpen

            delete Cmd;
        } catch (std::exception &e) {
            m_listErrorCode = -1;
        } catch (...) {
            m_listErrorCode = -1;
        }
    }

    qDebug() << "CompressedFolderModelWorker::list finished." << compressedFilePath << itemNameList << m_listErrorCode;

    return itemNameList;
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
    } else if (compressedFilePath.endsWith(".rar", Qt::CaseInsensitive)) {
        try {
            // Create target parent path.
            QDir().mkpath(targetParentPath);

            QString commandLine = "unrar x " + compressedFilePath + " " + (sourcePathList.empty() ? "" : (sourcePathList.join(" ") + " ")) + targetParentPath;

            // NOTE
            // Uses C functions to parse copied command into argv.
            // Copy command into new buffer because data() is allocated by QString.
            char command[1024];
            strcpy(command, commandLine.toLocal8Bit().data());
            int argc = 0;
            char *argv[64]; // Set max arguments as 64.
            char *p2 = strtok(command, " "); // First call with source string.
            while (p2 && argc < 64) {
                argv[argc++] = p2;
                p2 = strtok(0, " "); // Next call with NULL string.
            }

            CommandData *Cmd = new CommandData;
            Cmd->ParseCommandLine(false, argc, argv);
            qDebug() << "CompressedFolderModelWorker::extract rar ParseCommandLine"
                     << QString::fromWCharArray(Cmd->Command)
                     << QString::fromWCharArray(Cmd->ArcName)
                     << Cmd->ArcNames.ItemsCount()
                     << Cmd->FileArgs.ItemsCount()
                     << Cmd->ExclArgs.ItemsCount()
                     << Cmd->InclArgs.ItemsCount()
                     << QString::fromWCharArray(Cmd->ExtrPath);

            // Verify parsed ArcName.
            if (QString::fromWCharArray(Cmd->ArcName).compare(compressedFilePath) != 0) {
                qDebug() << "CompressedFolderModelWorker::extract rar ArcName is different.";
                return QStringList();
            }

            Cmd->ProcessCommand();
            delete Cmd;
        } catch (RAR_EXIT ErrCode) {
            ErrHandler.SetErrorCode(ErrCode);
        } catch (std::bad_alloc) {
            ErrHandler.MemoryErrorMsg();
            ErrHandler.SetErrorCode(RARX_MEMORY);
        } catch (...) {
            ErrHandler.SetErrorCode(RARX_FATAL);
        }
    }

    return QStringList();
}

QString CompressedFolderModelWorker::getDefaultCompressedFilePath(QString sourcePath)
{
    QFileInfo sourceInfo(sourcePath);

    return sourceInfo.absoluteDir().absoluteFilePath(sourceInfo.baseName() + ".zip");
}

char * CompressedFolderModelWorker::getCharPointer(QString s)
{
    char *c = s.toLocal8Bit().data();
    if (QString(c).compare(s) != 0) {
        return 0;
    } else {
        return c;
    }
}
