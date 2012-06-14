#include "monitoring.h"

#ifdef Q_OS_SYMBIAN
#include "e32std.h"
#endif

Monitoring::Monitoring(QObject *parent) :
    QObject(parent)
{
    qDebug() << "Monitoring is constructed.";

    QString filePath = QString("E:/FilesPlus_Heap_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    monitorFile = new QFile(filePath);
    if (monitorFile->open(QFile::WriteOnly | QIODevice::Text)) {
    out.setDevice(monitorFile);
    out << "Time,UserCountAllocCells,FreeCells,UserHeapCount,UserHeapSize,UserHeapMaxLength,UserAvailable,RThreadGetCpuTime\n";
    } else {
        qDebug() << "Monitoring::Monitoring I can't open" << monitorFile->fileName();
    }

    // TODO monitor heap.
    monitorTimer.setInterval(2000);
    monitorTimer.setSingleShot(false);
    connect(&monitorTimer, SIGNAL(timeout()), this, SLOT(log()) );
    monitorTimer.start();
}

Monitoring::~Monitoring()
{
    qDebug() << "Monitoring is destroyed.";

    monitorFile->flush();
    monitorFile->close();
}

void Monitoring::log()
{
#ifdef Q_OS_SYMBIAN
    TInt freeCell, biggestBlock;
    TTimeIntervalMicroSeconds cpuTime;

    RThread().GetCpuTime(cpuTime);

    // TODO Show available heap on Symbian.
    QString logMessage = QString("%1,%2,%3,%4,%5,%6,%7,%8\n")
            .arg(QTime::currentTime().toString(Qt::ISODate))
            .arg(User::CountAllocCells(freeCell))
            .arg(freeCell)
            .arg(User::Heap().Count())
            .arg(User::Heap().Size())
            .arg(User::Heap().MaxLength())
            .arg(User::Available(biggestBlock))
            .arg(cpuTime.Int64());

    qDebug() << logMessage;

    out << logMessage;
    out.flush();
#endif
}
