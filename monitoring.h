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
signals:
    
public slots:
    void log();
private:
    QTimer monitorTimer;
    QFile *monitorFile;
    QTextStream out;
};

#endif // MONITORING_H
