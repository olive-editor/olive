/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "customcachetask.h"

namespace olive {

CustomCacheTask::CustomCacheTask(const QString &sequence_name) :
  cancelled_through_finish_(false)
{
  SetTitle(tr("Caching custom range for \"%1\"").arg(sequence_name));
}

void CustomCacheTask::Finish()
{
  mutex_.lock();

  cancelled_through_finish_ = true;
  Cancel();

  mutex_.unlock();
}

bool CustomCacheTask::Run()
{
  mutex_.lock();

  while (!IsCancelled()) {
    wait_cond_.wait(&mutex_);
  }

  mutex_.unlock();

  return true;
}

void CustomCacheTask::CancelEvent()
{
  if (!cancelled_through_finish_) {
    emit Cancelled();
  }
  wait_cond_.wakeOne();
}

}
