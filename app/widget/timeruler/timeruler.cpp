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

TimeRuler::TimeRuler(bool text_visible, QWidget* parent) :
  QWidget(parent),
  scroll_(0),
  centered_text_(true),
  scale_(1.0), // FIXME: Temporary value
  time_(0)
{
  QFontMetrics fm = fontMetrics();

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  // Text height is used to calculate widget height
  text_height_ = fm.height();

  // Get the "minimum" space allowed between two line markers on the ruler (in screen pixels)
  // Mediocre but reliable way of scaling UI objects by font/DPI size
  minimum_gap_between_lines_ = QFontMetricsWidth(&fm, "H");

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

void TimeRuler::SetScale(double d)
{
  scale_ = d;

  update();
}

void TimeRuler::SetTimebase(const rational &r)
{
  time_base_ = r;

  update();
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

void TimeRuler::paintEvent(QPaintEvent *e)
{
  Q_UNUSED(e)

  // Nothing to paint if the timebase is invalid
  if (time_base_.denominator() == 0) {
    return;
  }

  QPainter p(this);

  int64_t last_unit = -1;
  int last_sec = -1;

  // Depending on the scale, we don't need all the lines drawn or else they'll start to become unhelpful
  // Determine an even number to divide the frame count by
  int rough_frames_in_second = qRound(time_base_.flipped().ToDouble());
  int test_divider = 1;
  while (!((rough_frames_in_second%test_divider == 0 || test_divider > rough_frames_in_second)
         && scale_ * test_divider >= minimum_gap_between_lines_)) {
    if (test_divider < rough_frames_in_second) {
      test_divider++;
    } else {
      test_divider += rough_frames_in_second;
    }
  }
  double reverse_divider = double(rough_frames_in_second) / double(test_divider);
  qreal real_divider = qMax(1.0, time_base_.flipped().ToDouble() / reverse_divider);

  // Set where the loop ends (affected by text)
  int loop_start = - playhead_width_;
  int loop_end = width() + playhead_width_;

  // Determine where it can draw text
  int text_skip = 1;
  int half_average_text_width = 0;
  int text_y = 0;
  if (text_visible_) {
    QFontMetrics fm = p.fontMetrics();
    double width_of_second = time_base_.flipped().ToDouble() * scale_;
    int average_text_width = QFontMetricsWidth(&fm, olive::timestamp_to_timecode(0, time_base_, kTimecodeDisplay));
    half_average_text_width = average_text_width/2;
    while (width_of_second * text_skip < average_text_width) {
      text_skip++;
    }

    if (centered_text_) {
      loop_start = -half_average_text_width;
      loop_end += half_average_text_width;
    } else {
      loop_start = -average_text_width;
    }

    text_y = fm.ascent();
  }

  // Set line color to main text color
  p.setBrush(Qt::NoBrush);
  p.setPen(palette().text().color());

  // Calculate where each line starts
  int line_top = text_visible_ ? text_height_ : 0;

  // Calculate all three line lengths
  int line_length = text_height_;
  int line_sec_bottom = line_top + line_length;
  int line_halfsec_bottom = line_top + line_length / 3 * 2;
  int line_frame_bottom = line_top + line_length / 3;

  int playhead_pos = -1;

  for (int i=loop_start;i<loop_end;i++) {
    int64_t unit = ScreenToUnit(i);

    // Queue a playhead draw
    if (unit == time_ && playhead_pos == -1) {
      playhead_pos = i;
    }

    // Check if enough space has passed since the last line drawn
    if (qFloor(double(unit)/real_divider) > qFloor(double(last_unit)/real_divider)) {

      // Determine if this unit is a whole second or not
      int sec = qFloor(double(unit) * time_base_.ToDouble());

      if (sec > last_sec) {
        // This line marks a second so we make it long
        p.drawLine(i, line_top, i, line_sec_bottom);

        last_sec = sec;

        // Try to draw text here
        if (text_visible_ && sec%text_skip == 0) {
          QString timecode_string = olive::timestamp_to_timecode(sec, time_base_, kTimecodeDisplay);

          int text_x = i;

          if (centered_text_) {
            text_x -= half_average_text_width;
          } else {
            timecode_string.prepend(" ");
            p.drawLine(i, 0, i, line_top);
          }

          p.drawText(text_x, text_y, timecode_string);
        }
      } else if (unit%rough_frames_in_second == rough_frames_in_second/2) {

        // This line marks the half second point so we make it somewhere in between
        p.drawLine(i, line_top, i, line_halfsec_bottom);

      } else {

        // This line just marks a frame so we make it short
        p.drawLine(i, line_top, i, line_frame_bottom);

      }

      last_unit = unit;
    }
  }

  // If we found the playhead along here, draw it last
  if (playhead_pos >= 0) {
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::red); // FIXME: Make configurable in CSS
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
    QPoint(x + half_width, y - text_height_),
    QPoint(x + half_width, y - half_text_height)
  };

  p->drawPolygon(points, 5);
}

double TimeRuler::ScreenToUnitFloat(int screen)
{
  return (screen + scroll_) / scale_;
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
