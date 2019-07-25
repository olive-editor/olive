#include "timeruler.h"

#include <QDebug>
#include <QPainter>
#include <QtMath>

#include "common/qtversionabstraction.h"

TimeRuler::TimeRuler(bool text_visible, QWidget* parent) :
  QWidget(parent),
  scroll_(0),
  centered_text_(true),
  scale_(16.0)
{
  QFontMetrics fm = fontMetrics();

  // Text height is used to calculate widget height
  text_height_ = fm.height();

  // Get the "minimum" space allowed between two line markers on the ruler (in screen pixels)
  // Mediocre but reliable way of scaling UI objects by font/DPI size
  minimum_gap_between_lines_ = QFontMetricsWidth(&fm, "H");

  // Text visibility affects height, so we set that here
  SetTextVisible(text_visible);

  // FIXME: Test code
  time_base_ = rational(1001, 30000);
  connect(&test_timer_, SIGNAL(timeout()), this, SLOT(TimeOut()));
  test_timer_.setInterval(20);
  test_timer_.start();
  // End test code
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

  int last_unit = -1;
  int last_sec = -1;
  /*
  // Test code that draws a line every single time unit
  for (int i=0;i<width();i++) {
    int unit = qFloor((i + scroll_) / scale_);
    int sec = qFloor(unit * time_base_.ToDouble());

    if (unit > last_unit) {

      if (sec > last_sec) {
        p.setPen(Qt::red);
        last_sec = sec;
      } else {
        p.setPen(Qt::white);
      }

      p.drawLine(i, height()/2, i, height());

      last_unit = unit;
    }
  }
  */

  // Depending on the scale,
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
  int loop_start = -1;
  int loop_end = width();

  // Determine where it can draw text
  int text_skip = 1;
  int half_average_text_width = 0;
  int text_y = 0;
  if (text_visible_) {
    QFontMetrics fm = p.fontMetrics();
    double width_of_second = time_base_.flipped().ToDouble() * scale_;
    int average_text_width = QFontMetricsWidth(&fm, "00:00:00;00"); // FIXME: Hardcoded string
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
  p.setPen(palette().text().color());

  // Calculate where each line starts
  int line_top = text_visible_ ? text_height_ : 0;

  // Calculate all three line lengths
  int line_length = text_height_;
  int line_sec_bottom = line_top + line_length;
  int line_halfsec_bottom = line_top + line_length / 3 * 2;
  int line_frame_bottom = line_top + line_length / 3;

  for (int i=loop_start;i<loop_end;i++) {
    int unit = qFloor((i + scroll_) / scale_);

    // Check if enough space has passed since the last line drawn
    if (qFloor(unit/real_divider) > qFloor(last_unit/real_divider)) {

      // Determine if this unit is a whole second or not
      int sec = qFloor(unit * time_base_.ToDouble());

      if (sec > last_sec) {
        // This line marks a second so we make it long
        p.drawLine(i, line_top, i, line_sec_bottom);

        last_sec = sec;

        // Try to draw text here
        if (text_visible_ && sec%text_skip == 0) {
          QString timecode_string = QString("00:00:%1;00").arg(sec, 2, 10, QChar('0'));

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

  p.setPen(Qt::NoPen);
  p.setBrush(Qt::red);
  DrawPlayhead(&p, 100, height());
}

void TimeRuler::DrawPlayhead(QPainter *p, int x, int y)
{
  p->setRenderHint(QPainter::Antialiasing);

  int half_text_height = text_height_ / 3;
  int half_gap = minimum_gap_between_lines_ / 2;

  QPoint points[] = {
    QPoint(x, y),
    QPoint(x - half_gap, y - half_text_height),
    QPoint(x - half_gap, y - text_height_),
    QPoint(x + half_gap, y - text_height_),
    QPoint(x + half_gap, y - half_text_height)
  };

  p->drawPolygon(points, 5);
}

double TimeRuler::UnitToScreen(const int &u)
{
  return u * scale_;
}

int TimeRuler::ScreenToUnit(const int &p)
{
  double uf = p / scale_;

  int u = qRound(uf);

  if (qFuzzyCompare(UnitToScreen(u), uf)) {
    return u;
  }

  return -1;
}

void TimeRuler::TimeOut()
{
  SetScroll(scroll_ + 2);
}
