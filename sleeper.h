#ifndef SLEEPER_H
#define SLEEPER_H

#include <QObject>
#include <QTimer>
#include <QApplication>
#include <QThread>

class Sleeper : public QThread
{
    Q_OBJECT
public:
    explicit Sleeper(QObject *parent = 0);
    ~Sleeper();

    static void msleep(int msec = 1000) { QThread::msleep(msec); }

    void sleep(int msec = 1000);
signals:
    
public slots:
    
private:
    QTimer timer;
};

#endif // SLEEPER_H
