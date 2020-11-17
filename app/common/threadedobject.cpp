/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "threadedobject.h"

OLIVE_NAMESPACE_ENTER

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

OLIVE_NAMESPACE_EXIT
