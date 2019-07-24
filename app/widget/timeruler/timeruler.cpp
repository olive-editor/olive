#include "timeruler.h"

#include <QPainter>

#include "common/qtversionabstraction.h"

TimeRuler::TimeRuler(bool text_visible, QWidget* parent) :
  QWidget(parent),
  scroll_(0),
  scale_(1.0)
{
  // Text height is used to calculate widget height
  text_height_ = fontMetrics().height();

  // FIXME: Fairly mediocre, but reliable way of scaling UI objects by font/DPI size
  QFontMetrics fm = fontMetrics();
  width_of_second_ = QFontMetricsWidth(&fm, "HHHHHHHHHHHHHHHH");
  minimum_gap_between_lines_ = QFontMetricsWidth(&fm, "HH");

  SetTextVisible(text_visible);
}

void TimeRuler::SetTextVisible(bool e)
{
  text_visible_ = e;

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

void TimeRuler::paintEvent(QPaintEvent *e)
{
  Q_UNUSED(e)

  QPainter p(this);

  // FIXME: Make this CSS configurable
  p.setPen(Qt::white);

  // Calculate where the ruler lines should be
  int ruler_second_line_top = 0;
  int ruler_second_line_bottom = height();

  //int ruler_frame_line_top = (text_visible_) ? text_height_ : 0;
  //int ruler_frame_line_bottom = height();

  // Get scaled width of a second
  double scaled_width = width_of_second_ * scale_;

  // Draw a line for each second
  int last_draw = -1;
  for (double i=0;i<width();i+=scaled_width) {

    // Ensure a reasonable gap has been left between the last line drawn and the line we're about to draw
    int line_x = qRound(i);
    if (last_draw < 0 || line_x - last_draw > minimum_gap_between_lines_) {

      p.drawLine(line_x, ruler_second_line_top, line_x, ruler_second_line_bottom);

      p.drawText(line_x, p.fontMetrics().ascent(), QString::number(line_x));

      last_draw = line_x;
    }
  }
}
