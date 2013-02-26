#include "symbianfsnotify.h"

SymbianFSNotify::SymbianFSNotify(SymbianFSObserver &observer)
    : CActive(EPriorityStandard),
      iObserver(observer)
{
    CleanupClosePushL(iFs);
    User::LeaveIfError(iFs.Connect());
    TFileName path;
    User::LeaveIfError(iFs.SessionPath(path));

    CActiveScheduler::Add(this);
}

SymbianFSNotify::~SymbianFSNotify()
{
    Cancel();

    iFs.Close();
    CleanupStack::PopAndDestroy();
}

void SymbianFSNotify::RunL()
{
    if (iStatus==KErrNone) {
        iObserver.CChange();
        StartMonitoring();
    } else {
        // there was an error so all monitoring will now stop
    }
}

void SymbianFSNotify::DoCancel()
{
    iFs.NotifyChangeCancel(iStatus);
}

void SymbianFSNotify::StartMonitoring()
{
   if (!IsActive()) {
        iFs.NotifyChange(ENotifyWrite, iStatus);

        SetActive();
   } else {
       // already waiting for a change so do nothing
   }
}
