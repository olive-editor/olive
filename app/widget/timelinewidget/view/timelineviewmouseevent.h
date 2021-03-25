/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
#include "widget/timebased/timescaledobject.h"

namespace olive {

class TimelineViewMouseEvent
{
public:
  TimelineViewMouseEvent(const qreal& scene_x,
                         const double& scale_x,
                         const rational& timebase,
                         const Track::Reference &track,
                         const Qt::MouseButton &button,
                         const Qt::KeyboardModifiers& modifiers = Qt::NoModifier) :
    scene_x_(scene_x),
    scale_x_(scale_x),
    timebase_(timebase),
    track_(track),
    button_(button),
    modifiers_(modifiers),
    source_event_(nullptr),
    mime_data_(nullptr)
  {
  }

  TimelineCoordinate GetCoordinates(bool round_time = false) const
  {
    return TimelineCoordinate(GetFrame(round_time), track_);
  }

  const Qt::KeyboardModifiers& GetModifiers() const
  {
    return modifiers_;
  }

  /**
   * @brief Gets the time at this cursor point
   *
   * @param round
   *
   * If set to true, the time will be rounded to the nearest time. If set to false, the time is floored so the time is
   * always to the left of the cursor. The former behavior is better for clicking between frames (e.g. razor tool) and
   * the latter is better for clicking directly on frames (e.g. pointer tool).
   */
  rational GetFrame(bool round = false) const
  {
    return TimeScaledObject::SceneToTime(scene_x_, scale_x_, timebase_, round);
  }

  const Track::Reference& GetTrack() const
  {
    return track_;
  }

  const QMimeData *GetMimeData()
  {
    return mime_data_;
  }

  void SetMimeData(const QMimeData *data)
  {
    mime_data_ = data;
  }

  void SetEvent(QEvent* event)
  {
    source_event_ = event;
  }

  const qreal& GetSceneX() const
  {
    return scene_x_;
  }

  const Qt::MouseButton& GetButton() const
  {
    return button_;
  }

  void accept()
  {
    if (source_event_ != nullptr)
      source_event_->accept();
  }

  void ignore()
  {
    if (source_event_ != nullptr)
      source_event_->ignore();
  }

private:
  qreal scene_x_;
  double scale_x_;
  rational timebase_;

  Track::Reference track_;

  Qt::MouseButton button_;

  Qt::KeyboardModifiers modifiers_;

  QEvent* source_event_;

  const QMimeData* mime_data_;

};

}

#endif // TIMELINEVIEWMOUSEEVENT_H
