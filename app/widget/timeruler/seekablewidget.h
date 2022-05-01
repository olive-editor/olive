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

#ifndef SEEKABLEWIDGET_H
#define SEEKABLEWIDGET_H

#include <QHBoxLayout>
#include <QScrollBar>

#include "common/rational.h"
#include "timeline/timelinepoints.h"
#include "widget/snapservice/snapservice.h"
#include "widget/timebased/timebasedviewselectionmanager.h"
//#include "widget/marker/markercopypaste.h"

namespace olive {

class SeekableWidget : public TimeBasedView//, public MarkerCopyPasteService
{
  Q_OBJECT
public:
  SeekableWidget(QWidget *parent = nullptr);

  int GetScroll() const
  {
    return horizontalScrollBar()->value();
  }

  void ConnectTimelinePoints(TimelinePoints* points);

  bool IsDraggingPlayhead() const
  {
    return dragging_;
  }

  void DeleteSelected();

  void CopySelected(bool cut);

  void PasteMarkers(bool insert, rational insert_time);

  void DeselectAllMarkers();

  void SeekToScenePoint(qreal scene);

public slots:
  void SetScroll(int i)
  {
    horizontalScrollBar()->setValue(i);
  }

  void SetMarkerColor(int c);

  /*void SetMarkerTime();

  void SetMarkerName();*/

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void focusOutEvent(QFocusEvent *event) override;

  void DrawTimelinePoints(QPainter *p, int marker_bottom = 0);

  TimelinePoints* timeline_points() const;

  void DrawPlayhead(QPainter* p, int x, int y);

  inline const int& text_height() const {
    return text_height_;
  }

  inline const int& playhead_width() const {
    return playhead_width_;
  }

private:
  TimelinePoints* timeline_points_;

  int text_height_;

  int playhead_width_;

  bool dragging_;

  TimeBasedViewSelectionManager<TimelineMarker> selection_manager_;

};

}

#endif // SEEKABLEWIDGET_H
