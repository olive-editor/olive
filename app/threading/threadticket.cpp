/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "threadticket.h"

namespace olive {

RenderTicket::RenderTicket() :
  is_running_(false),
  has_result_(false),
  finish_count_(0)
{
  SetJobTime();
}

void RenderTicket::WaitForFinished(QMutex *mutex)
{
  if (is_running_) {
    wait_.wait(mutex);
  }
}

void RenderTicket::Start()
{
  QMutexLocker locker(&lock_);

  is_running_ = true;
  has_result_ = false;
  result_.clear();
}

void RenderTicket::Finish()
{
  FinishInternal(false, QVariant());
}

void RenderTicket::Finish(QVariant result)
{
  FinishInternal(true, result);
}

QVariant RenderTicket::Get()
{
  WaitForFinished();

  // We don't have to mutex around this because there is no way to write to `result_` after
  // the ticket has finished and the above function blocks the calling thread until it is finished
  return result_;
}

void RenderTicket::WaitForFinished()
{
  QMutexLocker locker(&lock_);

  WaitForFinished(&lock_);
}

bool RenderTicket::IsRunning(bool lock)
{
  if (lock) {
    lock_.lock();
  }

  bool running = is_running_;

  if (lock) {
    lock_.unlock();
  }

  return running;
}

int RenderTicket::GetFinishCount(bool lock)
{
  if (lock) {
    lock_.lock();
  }

  int count = finish_count_;

  if (lock) {
    lock_.unlock();
  }

  return count;
}

bool RenderTicket::HasResult()
{
  QMutexLocker locker(&lock_);

  return has_result_;
}

void RenderTicket::FinishInternal(bool has_result, QVariant result)
{
  QMutexLocker locker(&lock_);

  if (!is_running_) {
    qWarning() << "Tried to finish ticket that wasn't running";
  } else {
    is_running_ = false;
    has_result_ = has_result;
    result_ = result;
    finish_count_++;

    wait_.wakeAll();

    locker.unlock();

    emit Finished();
  }
}

}
