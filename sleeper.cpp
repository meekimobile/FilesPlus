#include "sleeper.h"

Sleeper::Sleeper(QObject *parent) :
    QObject(parent)
{
}

Sleeper::~Sleeper()
{
    timer.deleteLater();
}

void Sleeper::sleep(int msec)
{
    QEventLoop loop;
    timer.setSingleShot(true);
    timer.setInterval(msec);
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start();
    loop.exec();
}
