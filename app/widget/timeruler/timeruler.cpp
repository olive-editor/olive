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
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

#include "common/timecodefunctions.h"
#include "common/qtversionabstraction.h"
#include "config/config.h"
#include "core.h"

TimeRuler::TimeRuler(bool text_visible, QWidget* parent) :
  QWidget(parent),
  scroll_(0),
  centered_text_(true),
  scale_(1.0),
  time_(0),
  snapping_(false)
{
  QFontMetrics fm = fontMetrics();

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  // Text height is used to calculate widget height
  text_height_ = fm.height();

  // Get the "minimum" space allowed between two line markers on the ruler (in screen pixels)
  // Mediocre but reliable way of scaling UI objects by font/DPI size
  minimum_gap_between_lines_ = QFontMetricsWidth(fm, "H");

  // Set width of playhead marker
  playhead_width_ = minimum_gap_between_lines_;

  // Text visibility affects height, so we set that here
  SetTextVisible(text_visible);
}

void TimeRuler::SetTextVisible(bool e)
{
  text_visible_ = e;

  // Text visibility affects height, if text is visible the widget doubles in height with the top half for text and
  // the bottom half for ruler markings
  if (text_visible_) {
    setMinimumHeight(text_height_ * 2);
  } else {
    setMinimumHeight(text_height_);
  }

  update();
}

const double &TimeRuler::scale()
{
  return scale_;
}

void TimeRuler::SetScale(const double &d)
{
  scale_ = d;

  update();
}

void TimeRuler::SetTimebase(const rational &r)
{
  timebase_ = r;

  timebase_dbl_ = timebase_.toDouble();

  timebase_flipped_dbl_ = timebase_.flipped().toDouble();

  update();
}

void TimeRuler::SetSnapping(bool snapping)
{
  snapping_ = snapping;
}

const int64_t &TimeRuler::GetTime()
{
  return time_;
}

void TimeRuler::SetTime(const int64_t &r)
{
  time_ = r;

  update();
}

void TimeRuler::SetScroll(int s)
{
  scroll_ = s;

  update();
}

void TimeRuler::paintEvent(QPaintEvent *)
{
  // Nothing to paint if the timebase is invalid
  if (timebase_.isNull()) {
    return;
  }

  QPainter p(this);

  double width_of_frame = timebase_dbl_ * scale_;
  double width_of_second = 0;
  do {
    width_of_second += timebase_dbl_;
  } while (width_of_second < 1.0);
  width_of_second *= scale_;
  double width_of_minute = width_of_second * 60;
  double width_of_hour = width_of_minute * 60;
  double width_of_day = width_of_hour * 24;

  double long_interval, short_interval;
  int long_rate;

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
    if (long_interval == -1) {
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
  int long_height = fm.height();
  int short_height = long_height/2;
  int long_y = height() - long_height;
  int short_y = height() - short_height;
  int line_bottom = height();

  // Draw long lines
  int last_long_unit = -1;
  int last_short_unit = -1;
  int last_text_draw = INT_MIN;

  // FIXME: Hardcoded number
  const int kAverageTextWidth = 200;

  for (int i=-kAverageTextWidth;i<width()+kAverageTextWidth;i++) {
    double screen_pt = static_cast<double>(i + scroll_);

    if (long_interval > -1) {
      int this_long_unit = qFloor(screen_pt/long_interval);
      if (this_long_unit != last_long_unit) {
        int line_y = long_y;

        if (text_visible_) {
          QRect text_rect;
          Qt::Alignment text_align;
          QString timecode_str = Timecode::timestamp_to_timecode(ScreenToUnit(i), timebase_, Timecode::CurrentDisplay());
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
                       text_align,
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

  // Draw the playhead if it's on screen at the moment
  int playhead_pos = qFloor(static_cast<double>(time_) * scale_ * timebase_dbl_) - scroll_;
  if (playhead_pos + playhead_width_ >= 0 && playhead_pos - playhead_width_ < width()) {
    p.setPen(Qt::NoPen);
    p.setBrush(style_.PlayheadColor());
    DrawPlayhead(&p, playhead_pos, height());
  }
}

void TimeRuler::mousePressEvent(QMouseEvent *event)
{
  SeekToScreenPoint(event->pos().x());
}

void TimeRuler::mouseMoveEvent(QMouseEvent *event)
{
  if (event->buttons() & Qt::LeftButton) {
    SeekToScreenPoint(event->pos().x());
  }
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

double TimeRuler::ScreenToUnitFloat(int screen)
{
  return (screen + scroll_) / scale_ / timebase_dbl_;
}

int64_t TimeRuler::ScreenToUnit(int screen)
{
  return qFloor(ScreenToUnitFloat(screen));
}

void TimeRuler::SeekToScreenPoint(int screen)
{
  int64_t timestamp = qMax(0, qRound(ScreenToUnitFloat(screen)));

  SetTime(timestamp);

  emit TimeChanged(timestamp);
}
