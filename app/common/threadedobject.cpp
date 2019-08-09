#include "threadedobject.h"

ThreadedObject::ThreadedObject()
{
}

void ThreadedObject::LockDeletes()
{
  threadobj_delete_lock_++;
}

void ThreadedObject::UnlockDeletes()
{
  Q_ASSERT(AreDeletesLocked());

  threadobj_delete_lock_--;
}

bool ThreadedObject::AreDeletesLocked()
{
  return (threadobj_delete_lock_ > 0);
}

void ThreadedObject::LockMutex()
{
  threadobj_main_lock_.lock();
}

void ThreadedObject::UnlockMutex()
{
  threadobj_main_lock_.unlock();
}

bool ThreadedObject::TryLockMutex(int timeout)
{
  return threadobj_main_lock_.tryLock(timeout);
}
