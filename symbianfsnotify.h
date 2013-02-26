#ifndef SYMBIANFSNOTIFY_H
#define SYMBIANFSNOTIFY_H

#include "eikenv.h"
#include "f32file.h"
#include "e32base.h"
#include "symbianfsobserver.h"

class SymbianFSNotify : public CActive
{
public:
    SymbianFSNotify(SymbianFSObserver& observer);
    ~SymbianFSNotify();

    void StartMonitoring();

    void RunL();
    void DoCancel();

private:
    RFs iFs;
    SymbianFSObserver& iObserver;
};

#endif // SYMBIANFSNOTIFY_H
