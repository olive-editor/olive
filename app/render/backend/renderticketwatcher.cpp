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

#include "renderticketwatcher.h"

OLIVE_NAMESPACE_ENTER

RenderTicketWatcher::RenderTicketWatcher(QObject *parent) :
  QObject(parent),
  ticket_(nullptr)
{
}

void RenderTicketWatcher::SetTicket(RenderTicketPtr ticket)
{
  // Ensure that a ticket has NOT already been set and that this ticket is NOT NULL
  Q_ASSERT(!ticket_ && ticket);

  ticket_ = ticket;

  QMutexLocker locker(ticket->lock());

  if (ticket_->IsFinished(false)) {
    locker.unlock();
    emit Finished();
  } else {
    connect(ticket_.get(), &RenderTicket::Finished, this, &RenderTicketWatcher::Finished);
  }
}

bool RenderTicketWatcher::WasCancelled()
{
  if (ticket_) {
    return ticket_->WasCancelled();
  } else {
    return false;
  }
}

bool RenderTicketWatcher::IsFinished()
{
  if (ticket_) {
    return ticket_->IsFinished();
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

OLIVE_NAMESPACE_EXIT
