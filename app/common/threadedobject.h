/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef THREADEDOBJECT_H
#define THREADEDOBJECT_H

#include <QMutex>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

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

OLIVE_NAMESPACE_EXIT

#endif // THREADEDOBJECT_H
