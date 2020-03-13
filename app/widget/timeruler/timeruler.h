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

#include "common/timerange.h"
#include "seekablewidget.h"

class TimeRuler : public SeekableWidget
{
  Q_OBJECT
public:
  TimeRuler(bool text_visible = true, bool cache_status_visible = false, QWidget* parent = nullptr);

  void SetCenteredText(bool c);

public slots:
  void CacheInvalidatedRange(const TimeRange &range);

  void CacheTimeReady(const rational& time);

  void SetCacheStatusLength(const rational& length);

protected:
  virtual void paintEvent(QPaintEvent* e) override;

  virtual void TimebaseChangedEvent(const rational& tb) override;

private:
  void UpdateHeight();

  void DrawPlayhead(QPainter* p, int x, int y);

  int CacheStatusHeight() const;

  int text_height_;

  int cache_status_height_;

  int minimum_gap_between_lines_;

  int playhead_width_;

  bool text_visible_;

  bool centered_text_;

  double timebase_flipped_dbl_;

  bool show_cache_status_;

  rational cache_length_;

  TimeRangeList dirty_cache_ranges_;

};

#endif // TIMERULER_H
