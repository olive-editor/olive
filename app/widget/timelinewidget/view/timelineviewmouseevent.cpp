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

#include "widget/timelinewidget/timelinescaledobject.h"

OLIVE_NAMESPACE_ENTER

TimelineViewMouseEvent::TimelineViewMouseEvent(const qreal &scene_x,
                                               const double &scale_x,
                                               const rational &timebase,
                                               const TrackReference &track,
                                               const Qt::KeyboardModifiers &modifiers) :
  scene_x_(scene_x),
  scale_x_(scale_x),
  timebase_(timebase),
  track_(track),
  modifiers_(modifiers),
  source_event_(nullptr),
  mime_data_(nullptr)
{
}

TimelineCoordinate TimelineViewMouseEvent::GetCoordinates(bool round_time) const
{
  return TimelineCoordinate(GetFrame(round_time), track_);
}

const Qt::KeyboardModifiers TimelineViewMouseEvent::GetModifiers() const
{
  return modifiers_;
}

rational TimelineViewMouseEvent::GetFrame(bool round) const
{
  return TimelineScaledObject::SceneToTime(scene_x_, scale_x_, timebase_, round);
}

const TrackReference &TimelineViewMouseEvent::GetTrack() const
{
  return track_;
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

const qreal &TimelineViewMouseEvent::GetSceneX() const
{
  return scene_x_;
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

OLIVE_NAMESPACE_EXIT
