#ifndef THREADEDOBJECT_H
#define THREADEDOBJECT_H

#include <QMutex>

class ThreadedObject
{
public:

  void LockMutex();
  void UnlockMutex();
  bool TryLockMutex(int timeout = 0);

  void LockDeletes();
  void UnlockDeletes();
  bool AreDeletesLocked();

private:
  QMutex threadobj_main_lock_;

  QAtomicInt threadobj_delete_lock_;
};

#endif // THREADEDOBJECT_H
