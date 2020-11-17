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

#ifndef SEEKABLEWIDGET_H
#define SEEKABLEWIDGET_H

#include "common/rational.h"
#include "timeline/timelinepoints.h"
#include "widget/timelinewidget/snapservice.h"
#include "widget/timelinewidget/timelinescaledobject.h"

OLIVE_NAMESPACE_ENTER

class SeekableWidget : public TimelineScaledWidget
{
  Q_OBJECT
public:
  SeekableWidget(QWidget *parent = nullptr);

  const int64_t& GetTime() const;

  const int& GetScroll() const;

  void ConnectTimelinePoints(TimelinePoints* points);

  void SetSnapService(SnapService* service);

public slots:
  void SetTime(const int64_t &r);

  void SetScroll(int s);

protected:
  void SeekToScreenPoint(int screen);

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(const double&) override;

  void DrawTimelinePoints(QPainter *p, int marker_bottom = 0);

  TimelinePoints* timeline_points() const;

  double ScreenToUnitFloat(int screen);

  int64_t ScreenToUnit(int screen);
  int64_t ScreenToUnitRounded(int screen);

  int UnitToScreen(int64_t unit);

  int TimeToScreen(const rational& time);

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
  void TimeChanged(int64_t);

private:
  int64_t time_;

  TimelinePoints* timeline_points_;

  int scroll_;

  int text_height_;

  int playhead_width_;

  SnapService* snap_service_;

};

OLIVE_NAMESPACE_EXIT

#endif // SEEKABLEWIDGET_H
