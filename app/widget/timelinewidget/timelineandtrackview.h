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

#ifndef TIMELINEANDTRACKVIEW_H
#define TIMELINEANDTRACKVIEW_H

#include <QSplitter>
#include <QWidget>

#include "view/timelineview.h"
#include "trackview/trackview.h"

OLIVE_NAMESPACE_ENTER

class TimelineAndTrackView : public QWidget
{
public:
  TimelineAndTrackView(Qt::Alignment vertical_alignment = Qt::AlignTop,
                       QWidget* parent = nullptr);

  QSplitter* splitter() const;

  TimelineView* view() const;

  TrackView* track_view() const;

private:
  QSplitter* splitter_;

  TimelineView* view_;

  TrackView* track_view_;

private slots:
  void ViewValueChanged(int v);

  void TracksValueChanged(int v);

};

OLIVE_NAMESPACE_EXIT

#endif // TIMELINEANDTRACKVIEW_H
