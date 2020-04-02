#include "colorpreviewbox.h"

#include <QPainter>

ColorPreviewBox::ColorPreviewBox(QWidget *parent) :
  QWidget(parent)
{
}

void ColorPreviewBox::SetColorProcessor(ColorProcessorPtr to_linear, ColorProcessorPtr to_display)
{
  to_linear_processor_ = to_linear;
  to_display_processor_ = to_display;

  update();
}

void ColorPreviewBox::SetColor(const Color &c)
{
  color_ = c;
  update();
}

void ColorPreviewBox::paintEvent(QPaintEvent *e)
{
  QWidget::paintEvent(e);

  QColor c;

  // Color management
  if (to_linear_processor_ && to_display_processor_) {
    c = to_display_processor_->ConvertColor(to_linear_processor_->ConvertColor(color_)).toQColor();
  } else {
    c = color_.toQColor();
  }

  QPainter p(this);

  p.setPen(Qt::black);
  p.setBrush(c);

  p.drawRect(rect());
}
