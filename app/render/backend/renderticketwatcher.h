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

#ifndef RENDERTICKETWATCHER_H
#define RENDERTICKETWATCHER_H

#include "renderticket.h"

OLIVE_NAMESPACE_ENTER

class RenderTicketWatcher : public QObject
{
  Q_OBJECT
public:
  RenderTicketWatcher(QObject* parent = nullptr);

  RenderTicketPtr GetTicket() const
  {
    return ticket_;
  }

  void SetTicket(RenderTicketPtr ticket);

  bool WasCancelled();

  bool IsFinished();

  void WaitForFinished();

  QVariant Get();

signals:
  void Finished();

private:
  RenderTicketPtr ticket_;

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERTICKETWATCHER_H
