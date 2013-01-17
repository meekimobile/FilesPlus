#include "symbianfsobserver.h"
#include <QDebug>

SymbianFSObserver::SymbianFSObserver()
{
}

SymbianFSObserver::~SymbianFSObserver()
{
}

void SymbianFSObserver::CChange()
{
    qDebug() << "SymbianFSObserver::CChange";
}
