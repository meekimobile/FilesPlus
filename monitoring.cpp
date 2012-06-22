#include "monitoring.h"

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
#endif

    // Start timer.
    monitorTimer.setInterval(2000);
    monitorTimer.setSingleShot(false);
    connect(&monitorTimer, SIGNAL(timeout()), this, SLOT(log()) );
}

Monitoring::~Monitoring()
{
    qDebug() << "Monitoring is destroyed.";

    qDebug() << "monitorFile" << monitorFile;
    monitorFile->flush();
    monitorFile->close();
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
#endif
}

void Monitoring::start()
{
    QString filePath = QString("E:/FilesPlus_Heap_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    monitorFile = new QFile(filePath);
    if (monitorFile->open(QFile::WriteOnly | QIODevice::Text)) {
        qDebug() << "Monitoring::Monitoring open file" << monitorFile->fileName();
        out.setDevice(monitorFile);
        out << "Time,UserCountAllocCells,FreeCells,UserHeapCount,UserHeapSize,UserHeapMaxLength,UserAvailable,CpuDelta\n";
        monitorTimer.start();
    } else {
        qDebug() << "Monitoring::Monitoring I can't open" << monitorFile->fileName();
    }
}

void Monitoring::stop()
{
    monitorTimer.stop();

    qDebug() << "monitorFile" << monitorFile;
    if (monitorFile == 0 || monitorFile->isOpen()) {
        monitorFile->flush();
        monitorFile->close();
    }
}
