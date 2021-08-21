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

#include "common/rational.h"
#include "timeline/timelinepoints.h"
#include "widget/snapservice/snapservice.h"
#include "widget/timebased/timescaledobject.h"
#include "widget/marker/marker.h"
#include "widget/marker/markercopypaste.h"

namespace olive {

class SeekableWidget : public TimelineScaledWidget, public MarkerCopyPasteService
{
  Q_OBJECT
public:
  SeekableWidget(QWidget *parent = nullptr);

  const rational& GetTime() const
  {
    return time_;
  }

  const int& GetScroll() const;

  void ConnectTimelinePoints(TimelinePoints* points);

  void SetSnapService(SnapService* service);

  bool IsDraggingPlayhead() const
  {
    return dragging_;
  }

  void DeleteSelected();

  void CopySelected(bool cut);

  void PasteMarkers(bool insert, rational insert_time);

  QVector<TimelineMarker*> GetActiveTimelineMarkers();

  void DeselectAllMarkers();

  void SeekToScreenPoint(int screen);

public slots:
  void SetTime(const rational &r);

  void SetScroll(int s);

  void addMarker(TimelineMarker* marker);

  void removeMarker(TimelineMarker* marker);

  void updateMarkerPositions();

  void SetMarkerColor(int c);

protected:

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(const double&) override;

  void DrawTimelinePoints(QPainter *p, int marker_bottom = 0);

  TimelinePoints* timeline_points() const;

  int TimeToScreen(const rational& time) const;
  rational ScreenToTime(int x) const;

  void DrawPlayhead(QPainter* p, int x, int y);

  inline const int& text_height() const {
    return text_height_;
  }

  inline const int& playhead_width() const {
    return playhead_width_;
  }

signals:
  /**
   * @brief Signal emitted whenever the time changes on this ruler, either by user or programmatically
   */
  void TimeChanged(const rational &time);

private:
  rational time_;

  TimelinePoints* timeline_points_;

  int scroll_;

  int text_height_;

  int playhead_width_;

  SnapService* snap_service_;

  bool dragging_;

  QMap<TimelineMarker*, Marker*> marker_map_;

  QMap<TimelineMarker*, Marker*> active_markers_map_;

};

}

#endif // SEEKABLEWIDGET_H
