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

#ifndef RESIZABLETIMELINESCROLLBAR_H
#define RESIZABLETIMELINESCROLLBAR_H

#include "resizablescrollbar.h"
#include "timeline/timelinepoints.h"
#include "widget/timebased/timescaledobject.h"

namespace olive {

class ResizableTimelineScrollBar : public ResizableScrollBar, public TimeScaledObject
{
  Q_OBJECT
public:
  ResizableTimelineScrollBar(QWidget* parent = nullptr);
  ResizableTimelineScrollBar(Qt::Orientation orientation, QWidget* parent = nullptr);

  void ConnectTimelinePoints(TimelinePoints* points);

  void SetScale(double d);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

private:
  TimelinePoints* points_;

  double scale_;

};

}

#endif // RESIZABLETIMELINESCROLLBAR_H
