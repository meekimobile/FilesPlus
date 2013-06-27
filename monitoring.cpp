#include "monitoring.h"
#include <QCoreApplication>
#include <QDir>
#ifdef Q_OS_SYMBIAN
#include "e32std.h"
#endif

Monitoring::Monitoring(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Monitoring is constructed.";

#ifdef Q_OS_SYMBIAN
    // Initialize.
    lastCpuTime = 0;
#elif defined(Q_WS_HARMATTAN)
    statmPath = QString("/proc/%1/statm").arg(QCoreApplication::applicationPid());
    statm.setFileName(statmPath);
    statPath = QString("/proc/%1/stat").arg(QCoreApplication::applicationPid());
    stat.setFileName(statPath);
    memoryPageSize = 4096;
    lastCpuTime = 0;
#endif

    // Start timer.
    monitorTimer = new QTimer(this);
    monitorTimer->setInterval(4000);
    monitorTimer->setSingleShot(false);
    connect(monitorTimer, SIGNAL(timeout()), this, SLOT(log()) );
    connect(this, SIGNAL(destroyed()), monitorTimer, SIGNAL(destroyed()) );
}

Monitoring::~Monitoring()
{
    qDebug() << "Monitoring is destroyed.";

    stop();
}

void Monitoring::log()
{
#ifdef Q_OS_SYMBIAN
    TInt freeCell, biggestBlock;
    TTimeIntervalMicroSeconds cpuTime;

    RThread().GetCpuTime(cpuTime);
    TInt64 cpuDelta = cpuTime.Int64() - lastCpuTime.Int64();

    // TODO Show available heap on Symbian.
    QString logMessage = QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
            .arg(QTime::currentTime().toString(Qt::ISODate))
            .arg(User::CountAllocCells(freeCell))
            .arg(freeCell)
            .arg(User::Heap().Count())
            .arg(User::Heap().Size())
            .arg(User::Heap().MaxLength())
            .arg(User::Available(biggestBlock))
            .arg(cpuDelta);

    qDebug() << logMessage;

    out << logMessage;
    out.flush();

    lastCpuTime = cpuTime;
#elif defined(Q_WS_HARMATTAN)
    qint32 utime = 0;
    qint32 stime = 0;
    if (stat.open(QFile::ReadOnly)) {
        QStringList tokens = QString(stat.readAll()).split(' ');
        utime = tokens.at(13).toInt();
        stime = tokens.at(14).toInt();
    }
    qint32 cpuDelta = utime - lastCpuTime;
    lastCpuTime = utime;
    stat.close();

    if (statm.open(QFile::ReadOnly)) {
        QStringList tokens = QString(statm.readAll()).split(' ');
        QString logMessage = QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
                .arg(QTime::currentTime().toString(Qt::ISODate))
                .arg(0)
                .arg(0)
                .arg(0)
                .arg(tokens.at(5).toInt() * memoryPageSize)
                .arg(tokens.at(1).toInt() * memoryPageSize)
                .arg(0)
                .arg(cpuDelta);

        qDebug() << logMessage;
        out << logMessage;
        out.flush();
    } else {
        qDebug() << "Monitoring::log can't open" << statmPath;
    }
    statm.close();
#endif
}

void Monitoring::start()
{
#ifdef Q_OS_SYMBIAN
    filePath = QString("E:/FilesPlus_Heap_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
#elif defined(Q_WS_HARMATTAN)
    filePath = QString("/home/user/MyDocs/FilesPlus_Heap_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd"));
#endif
    monitorFile = new QFile(filePath);
    if (monitorFile->open(QFile::Append | QIODevice::Text)) {
        qDebug() << "Monitoring::Monitoring open file" << monitorFile->fileName();
        out.setDevice(monitorFile);
#ifdef Q_OS_SYMBIAN
        out << "Time,UserCountAllocCells,FreeCells,UserHeapCount,UserHeapSize,UserHeapMaxLength,UserAvailable,CpuDelta\n";
#elif defined(Q_WS_HARMATTAN)
        out << "Time,UserCountAllocCells,FreeCells,UserHeapCount,UserHeapSize,UserHeapMaxLength,UserAvailable,CpuDelta\n";
#endif
        monitorTimer->start();
    } else {
        qDebug() << "Monitoring::Monitoring I can't open" << monitorFile->fileName();
    }
}

void Monitoring::stop()
{
    monitorTimer->stop();

    qDebug() << "Monitoring::stop monitorFile" << monitorFile;
    if (monitorFile == 0 || monitorFile->isOpen()) {
        monitorFile->flush();
        monitorFile->close();
    }
}

QString Monitoring::getMonitoringFilePath() const
{
    return filePath;
}
