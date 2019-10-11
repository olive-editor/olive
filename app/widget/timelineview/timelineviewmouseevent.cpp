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

#include "timelineviewmouseevent.h"

#include <QEvent>

TimelineViewMouseEvent::TimelineViewMouseEvent(const TimelineCoordinate &coord,
                                               const Qt::KeyboardModifiers &modifiers) :
  coord_(coord),
  modifiers_(modifiers),
  source_event_(nullptr),
  mime_data_(nullptr)
{
}

TimelineViewMouseEvent::TimelineViewMouseEvent(const rational &frame,
                                               const TrackReference &track,
                                               const Qt::KeyboardModifiers &modifiers) :
  coord_(frame, track),
  modifiers_(modifiers),
  source_event_(nullptr),
  mime_data_(nullptr)
{
}

const TimelineCoordinate &TimelineViewMouseEvent::GetCoordinates()
{
  return coord_;
}

const Qt::KeyboardModifiers TimelineViewMouseEvent::GetModifiers()
{
  return modifiers_;
}

const QMimeData* TimelineViewMouseEvent::GetMimeData()
{
  return mime_data_;
}

void TimelineViewMouseEvent::SetMimeData(const QMimeData *data)
{
  mime_data_ = data;
}

void TimelineViewMouseEvent::SetEvent(QEvent *event)
{
  source_event_ = event;
}

void TimelineViewMouseEvent::accept()
{
  if (source_event_ != nullptr)
    source_event_->accept();
}

void TimelineViewMouseEvent::ignore()
{
  if (source_event_ != nullptr)
    source_event_->ignore();
}
