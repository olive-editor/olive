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

#ifndef TIMELINEVIEWMOUSEEVENT_H
#define TIMELINEVIEWMOUSEEVENT_H

#include <QMimeData>
#include <QPointF>
#include <QPoint>

#include "timeline/timelinecoordinate.h"

class TimelineViewMouseEvent
{
public:
  TimelineViewMouseEvent(const TimelineCoordinate& coord,
                         const Qt::KeyboardModifiers& modifiers = Qt::NoModifier);

  TimelineViewMouseEvent(const rational& frame,
                         const TrackReference &track,
                         const Qt::KeyboardModifiers& modifiers = Qt::NoModifier);

  const TimelineCoordinate& GetCoordinates();
  const Qt::KeyboardModifiers GetModifiers();

  const QMimeData *GetMimeData();
  void SetMimeData(const QMimeData *data);

  void SetEvent(QEvent* event);

  void accept();
  void ignore();

private:
  TimelineCoordinate coord_;

  Qt::KeyboardModifiers modifiers_;

  QEvent* source_event_;

  const QMimeData* mime_data_;

};

#endif // TIMELINEVIEWMOUSEEVENT_H
