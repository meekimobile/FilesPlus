#ifndef SLEEPER_H
#define SLEEPER_H

#include <QObject>
#include <QTimer>
#include <QApplication>

class Sleeper : public QObject
{
    Q_OBJECT
public:
    explicit Sleeper(QObject *parent = 0);
    ~Sleeper();

    void sleep(int msec = 1000);
signals:
    
public slots:
    
private:
    QTimer timer;
};

#endif // SLEEPER_H
