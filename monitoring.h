#ifndef MONITORING_H
#define MONITORING_H

#include <QObject>
#include <QFile>
#include <QTimer>
#include <QTime>
#include <QTextStream>
#include <QDebug>

class Monitoring : public QObject
{
    Q_OBJECT
public:
    explicit Monitoring(QObject *parent = 0);
    ~Monitoring();
    void start();
    void stop();
    QString getMonitoringFilePath() const;
signals:
    
public slots:
    void log();
private:
    QTimer *monitorTimer;
    QFile *monitorFile;
    QString filePath;
    QTextStream out;
#ifdef Q_OS_SYMBIAN
    TTimeIntervalMicroSeconds lastCpuTime;
#elif defined(Q_WS_HARMATTAN)
    QString statmPath;
    QFile statm;
    QString statPath;
    QFile stat;
    qint32 memoryPageSize;
    qint32 lastCpuTime;
#endif
};

#endif // MONITORING_H
