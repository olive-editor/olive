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

#include "threadticketwatcher.h"

namespace olive {

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

void RenderTicketWatcher::TicketFinished()
{
  emit Finished(this);
}

}
