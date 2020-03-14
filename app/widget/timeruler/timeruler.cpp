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

#include "timeruler.h"

#include <QDebug>
#include <QPainter>

#include "common/timecodefunctions.h"
#include "common/qtutils.h"
#include "config/config.h"
#include "core.h"

TimeRuler::TimeRuler(bool text_visible, bool cache_status_visible, QWidget* parent) :
  SeekableWidget(parent),
  text_visible_(text_visible),
  centered_text_(true),
  show_cache_status_(cache_status_visible)
{
  QFontMetrics fm = fontMetrics();

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  // Text height is used to calculate widget height
  text_height_ = fm.height();
  cache_status_height_ = text_height_ / 4;

  // Get the "minimum" space allowed between two line markers on the ruler (in screen pixels)
  // Mediocre but reliable way of scaling UI objects by font/DPI size
  minimum_gap_between_lines_ = QFontMetricsWidth(fm, "H");

  // Set width of playhead marker
  playhead_width_ = minimum_gap_between_lines_;

  // Text visibility affects height, so we set that here
  UpdateHeight();
}

void TimeRuler::SetCacheStatusLength(const rational &length)
{
  if (show_cache_status_) {
    cache_length_ = length;

    dirty_cache_ranges_.RemoveTimeRange(TimeRange(length, RATIONAL_MAX));

    update();
  }
}

void TimeRuler::CacheInvalidatedRange(const TimeRange& range)
{
  if (show_cache_status_) {
    dirty_cache_ranges_.InsertTimeRange(range);

    update();
  }
}

void TimeRuler::CacheTimeReady(const rational &time)
{
  if (show_cache_status_) {
    dirty_cache_ranges_.RemoveTimeRange(TimeRange(time, time + timebase()));

    update();
  }
}

void TimeRuler::paintEvent(QPaintEvent *)
{
  // Nothing to paint if the timebase is invalid
  if (timebase().isNull()) {
    return;
  }

  QPainter p(this);

  // Draw timeline points if connected
  if (timeline_points()) {

    // Draw in/out workarea
    if (timeline_points()->workarea()->enabled()) {
      int workarea_left = qMax(0, TimeToScreen(timeline_points()->workarea()->in()));
      int workarea_right;

      if (timeline_points()->workarea()->out() == TimelineWorkArea::kResetOut) {
        workarea_right = width();
      } else {
        workarea_right = qMin(width(), TimeToScreen(timeline_points()->workarea()->out()));
      }

      p.fillRect(workarea_left, 0, workarea_right - workarea_left, height(), palette().highlight());
    }

    // Draw markers
    if (!timeline_points()->markers()->list().isEmpty()) {
      int marker_bottom = height() - text_height_;

      if (show_cache_status_) {
        marker_bottom -= cache_status_height_;
      }

      if (text_visible_) {
        marker_bottom -= cache_status_height_;
      }

      int marker_top = marker_bottom - text_height_;

      // FIXME: Hardcoded marker colors
      p.setPen(Qt::black);
      p.setBrush(Qt::green);

      foreach (TimelineMarker* marker, timeline_points()->markers()->list()) {
        int marker_left = TimeToScreen(marker->time().in());
        int marker_right = TimeToScreen(marker->time().out());

        if (marker_left >= width() || marker_right < 0)  {
          continue;
        }

        if (marker->time().length() == 0) {
          // Single point in time marker
          DrawPlayhead(&p, marker_left, marker_bottom);
        } else {
          // Marker range
          int rect_left = qMax(0, marker_left);
          int rect_right = qMin(width(), marker_right);

          QRect marker_rect(rect_left, marker_top, rect_right - rect_left, marker_bottom - marker_top);

          p.drawRect(marker_rect);

          if (!marker->name().isEmpty()) {
            p.drawText(marker_rect, marker->name());
          }
        }
      }
    }
  }

  double width_of_frame = timebase_dbl() * GetScale();
  double width_of_second = 0;
  do {
    width_of_second += timebase_dbl();
  } while (width_of_second < 1.0);
  width_of_second *= GetScale();
  double width_of_minute = width_of_second * 60;
  double width_of_hour = width_of_minute * 60;
  double width_of_day = width_of_hour * 24;

  double long_interval, short_interval;
  int long_rate = 0;

  // Used for comparison, even if one unit can technically fit, we have to fit at least two for it to matter
  int doubled_gap = minimum_gap_between_lines_ * 2;

  if (width_of_day < doubled_gap) {
    long_interval = -1;
    short_interval = width_of_day;
  } else if (width_of_hour < doubled_gap) {
    long_interval = width_of_day;
    long_rate = 24;
    short_interval = width_of_hour;
  } else if (width_of_minute < doubled_gap) {
    long_interval = width_of_hour;
    long_rate = 60;
    short_interval = width_of_minute;
  } else if (width_of_second < doubled_gap) {
    long_interval = width_of_minute;
    long_rate = 60;
    short_interval = width_of_second;
  } else if (width_of_frame < doubled_gap) {
    long_interval = width_of_second;
    long_rate = qRound(timebase_flipped_dbl_);
    short_interval = width_of_frame;
  } else {
    // FIXME: Implement this...
    long_interval = width_of_second;
    short_interval = width_of_frame;
  }

  if (short_interval < minimum_gap_between_lines_) {
    if (long_interval <= 0) {
      do {
        short_interval *= 2;
      } while (short_interval < minimum_gap_between_lines_);
    } else {
      int div;

      short_interval = long_interval;

      for (div=long_rate;div>0;div--) {
        if (long_rate%div == 0) {
          // This division produces a whole number
          double test_frame_width = long_interval / static_cast<double>(div);

          if (test_frame_width >= minimum_gap_between_lines_) {
            short_interval = test_frame_width;
            break;
          }
        }
      }
    }
  }

  // Set line color to main text color
  p.setBrush(Qt::NoBrush);
  p.setPen(palette().text().color());

  // Calculate line dimensions
  QFontMetrics fm = p.fontMetrics();
  int line_bottom = height();

  if (show_cache_status_) {
    line_bottom -= cache_status_height_;
  }

  int long_height = fm.height();
  int short_height = long_height/2;
  int long_y = line_bottom - long_height;
  int short_y = line_bottom - short_height;

  // Draw long lines
  int last_long_unit = -1;
  int last_short_unit = -1;
  int last_text_draw = INT_MIN;

  // FIXME: Hardcoded number
  const int kAverageTextWidth = 200;

  for (int i=-kAverageTextWidth;i<width()+kAverageTextWidth;i++) {
    double screen_pt = static_cast<double>(i + GetScroll());

    if (long_interval > -1) {
      int this_long_unit = qFloor(screen_pt/long_interval);
      if (this_long_unit != last_long_unit) {
        int line_y = long_y;

        if (text_visible_) {
          QRect text_rect;
          Qt::Alignment text_align;
          QString timecode_str = Timecode::timestamp_to_timecode(ScreenToUnit(i), timebase(), Timecode::CurrentDisplay());
          int timecode_width = QFontMetricsWidth(fm, timecode_str);
          int timecode_left;

          if (centered_text_) {
            text_rect = QRect(i - kAverageTextWidth/2, 0, kAverageTextWidth, fm.height());
            text_align = Qt::AlignCenter;
            timecode_left = i - timecode_width/2;
          } else {
            text_rect = QRect(i, 0, kAverageTextWidth, fm.height());
            text_align = Qt::AlignLeft | Qt::AlignVCenter;
            timecode_left = i;

            // Add gap to left between line and text
            timecode_str.prepend(' ');
          }

          if (timecode_left > last_text_draw) {
            p.drawText(text_rect,
                       static_cast<int>(text_align),
                       timecode_str);

            last_text_draw = timecode_left + timecode_width;

            if (!centered_text_) {
              line_y = 0;
            }
          }
        }

        p.drawLine(i, line_y, i, line_bottom);
        last_long_unit = this_long_unit;
      }
    }

    if (short_interval > -1) {
      int this_short_unit = qFloor(screen_pt/short_interval);
      if (this_short_unit != last_short_unit) {
        p.drawLine(i, short_y, i, line_bottom);
        last_short_unit = this_short_unit;
      }
    }
  }

  // If cache status is enabled
  if (show_cache_status_) {
    int cache_screen_length = qMin(TimeToScreen(cache_length_), width());

    if (cache_screen_length > 0) {
      int cache_y = height() - cache_status_height_;

      p.fillRect(0, cache_y, cache_screen_length , cache_status_height_, Qt::green);

      foreach (const TimeRange& range, dirty_cache_ranges_) {
        int range_left = TimeToScreen(range.in());
        int range_right = TimeToScreen(range.out());

        if (range_left >= width() || range_right < 0) {
          continue;
        }

        int adjusted_left = qMax(0, range_left);

        p.fillRect(adjusted_left, cache_y, qMin(width(), range_right) - adjusted_left, cache_status_height_, Qt::red);
      }
    }
  }

  // Draw the playhead if it's on screen at the moment
  int playhead_pos = UnitToScreen(GetTime());
  if (playhead_pos + playhead_width_ >= 0 && playhead_pos - playhead_width_ < width()) {
    p.setPen(Qt::NoPen);
    p.setBrush(GetPlayheadColor());
    DrawPlayhead(&p, playhead_pos, line_bottom);
  }
}

void TimeRuler::TimebaseChangedEvent(const rational &tb)
{
  timebase_flipped_dbl_ = tb.flipped().toDouble();

  update();
}

void TimeRuler::DrawPlayhead(QPainter *p, int x, int y)
{
  p->setRenderHint(QPainter::Antialiasing);

  int half_text_height = text_height_ / 3;
  int half_width = playhead_width_ / 2;

  QPoint points[] = {
    QPoint(x, y),
    QPoint(x - half_width, y - half_text_height),
    QPoint(x - half_width, y - text_height_),
    QPoint(x + 1 + half_width, y - text_height_),
    QPoint(x + 1 + half_width, y - half_text_height),
    QPoint(x + 1, y),
  };

  p->drawPolygon(points, 6);
}

int TimeRuler::CacheStatusHeight() const
{
  return fontMetrics().height() / 4;
}

void TimeRuler::UpdateHeight()
{
  int height = text_height_;

  // Add text height
  if (text_visible_) {
    height += text_height_;
  }

  // Add cache status height
  if (show_cache_status_) {
    height += cache_status_height_;
  }

  // Add marker height
  height += text_height_;

  setFixedHeight(height);
}
