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

#include "renderticket.h"

OLIVE_NAMESPACE_ENTER

RenderTicket::RenderTicket(Type type, const QVariant &time) :
  finished_(false),
  cancelled_(false),
  time_(time),
  type_(type),
  job_time_(0)
{
}

void RenderTicket::WaitForFinished()
{
  QMutexLocker locker(&lock_);

  if (!finished_) {
    wait_.wait(&lock_);
  }
}

QVariant RenderTicket::Get()
{
  QMutexLocker locker(&lock_);

  if (!finished_) {
    wait_.wait(&lock_);
  }

  return result_;
}

bool RenderTicket::IsFinished(bool lock)
{
  if (lock) {
    lock_.lock();
  }

  bool finished = finished_;

  if (lock) {
    lock_.unlock();
  }

  return finished;
}

bool RenderTicket::WasCancelled()
{
  QMutexLocker locker(&lock_);

  return cancelled_;
}

void RenderTicket::Finish(QVariant result)
{
  QMutexLocker locker(&lock_);

  finished_ = true;
  result_ = result;

  wait_.wakeAll();

  locker.unlock();

  emit Finished();
}

void RenderTicket::Cancel()
{
  QMutexLocker locker(&lock_);

  finished_ = true;
  cancelled_ = true;

  wait_.wakeAll();

  locker.unlock();

  emit Finished();
}

OLIVE_NAMESPACE_EXIT
