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

#include "renderticket.h"

namespace olive {

RenderTicket::RenderTicket() :
  is_running_(false),
  has_result_(false),
  finish_count_(0)
{
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

RenderTicketWatcher::RenderTicketWatcher(QObject *parent) :
  QObject(parent),
  ticket_(nullptr)
{
}

void RenderTicketWatcher::SetTicket(RenderTicketPtr ticket)
{
  if (ticket_) {
    qCritical() << "Tried to set a ticket on a RenderTicketWatcher twice";
    return;
  }

  if (!ticket) {
    qCritical() << "Tried to set a null ticket on a RenderTicketWatcher";
    return;
  }

  ticket_ = ticket;

  // Lock ticket so we can query if it's already finished by the time this code runs
  QMutexLocker locker(ticket->lock());

  connect(ticket_.get(), &RenderTicket::Finished, this, &RenderTicketWatcher::TicketFinished);

  if (!ticket_->IsRunning(false) && ticket_->GetFinishCount(false) > 0) {
    // Ticket has already finished before, so we emit a signal
    locker.unlock();
    TicketFinished();
  }
}

bool RenderTicketWatcher::IsRunning()
{
  if (ticket_) {
    return ticket_->IsRunning();
  } else {
    return false;
  }
}

void RenderTicketWatcher::WaitForFinished()
{
  if (ticket_) {
    ticket_->WaitForFinished();
  }
}

QVariant RenderTicketWatcher::Get()
{
  if (ticket_) {
    return ticket_->Get();
  } else {
    return QVariant();
  }
}

bool RenderTicketWatcher::HasResult()
{
  if (ticket_) {
    return ticket_->HasResult();
  } else {
    return false;
  }
}

void RenderTicketWatcher::Cancel()
{
  if (ticket_) {
    ticket_->Cancel();
  }
}

void RenderTicketWatcher::TicketFinished()
{
  emit Finished(this);
}

}
