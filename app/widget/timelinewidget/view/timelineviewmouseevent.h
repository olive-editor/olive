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

namespace olive {

class TimelineViewMouseEvent
{
public:
  TimelineViewMouseEvent(const qreal& scene_x,
                         const double& scale_x,
                         const rational& timebase,
                         const TrackReference &track,
                         const Qt::MouseButton &button,
                         const Qt::KeyboardModifiers& modifiers = Qt::NoModifier);

  TimelineCoordinate GetCoordinates(bool round_time = false) const;
  const Qt::KeyboardModifiers& GetModifiers() const;

  /**
   * @brief Gets the time at this cursor point
   *
   * @param round
   *
   * If set to true, the time will be rounded to the nearest time. If set to false, the time is floored so the time is
   * always to the left of the cursor. The former behavior is better for clicking between frames (e.g. razor tool) and
   * the latter is better for clicking directly on frames (e.g. pointer tool).
   */
  rational GetFrame(bool round = false) const;

  const TrackReference& GetTrack() const;

  const QMimeData *GetMimeData();
  void SetMimeData(const QMimeData *data);

  void SetEvent(QEvent* event);

  const qreal& GetSceneX() const;

  const Qt::MouseButton& GetButton() const;

  void accept();
  void ignore();

private:
  qreal scene_x_;
  double scale_x_;
  rational timebase_;

  TrackReference track_;

  Qt::MouseButton button_;

  Qt::KeyboardModifiers modifiers_;

  QEvent* source_event_;

  const QMimeData* mime_data_;

};

}

#endif // TIMELINEVIEWMOUSEEVENT_H
