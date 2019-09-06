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

#ifndef TIMERULER_H
#define TIMERULER_H

#include <QTimer>
#include <QWidget>

#include "common/rational.h"
#include "widget/timelineview/timelineplayhead.h"

class TimeRuler : public QWidget
{
  Q_OBJECT
public:
  TimeRuler(bool text_visible = true, QWidget* parent = nullptr);

  void SetTextVisible(bool e);

  const double& scale();
  void SetScale(const double& d);

  void SetTimebase(const rational& r);

  void SetCenteredText(bool c);

  const int64_t& Time();

public slots:
  void SetTime(const int64_t &r);

  void SetScroll(int s);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;

signals:
  /**
   * @brief Signal emitted whenever the time changes on this ruler, either by user or programatically
   */
  void TimeChanged(int64_t);

private:
  void DrawPlayhead(QPainter* p, int x, int y);

  double ScreenToUnitFloat(int screen);

  int64_t ScreenToUnit(int screen);

  void SeekToScreenPoint(int screen);

  int text_height_;

  int minimum_gap_between_lines_;

  int playhead_width_;

  int scroll_;

  bool text_visible_;

  bool centered_text_;

  double scale_;

  rational timebase_;

  double timebase_dbl_;

  double timebase_flipped_dbl_;

  int64_t time_;

  TimelinePlayhead style_;

};

#endif // TIMERULER_H
