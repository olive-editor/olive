#include "colorswatchwidget.h"

#include <QMouseEvent>

#include "render/backend/opengl/openglrenderfunctions.h"

ColorSwatchWidget::ColorSwatchWidget(QWidget *parent) :
  QWidget(parent),
  to_linear_processor_(nullptr),
  to_display_processor_(nullptr)
{
}

const Color &ColorSwatchWidget::GetSelectedColor() const
{
  return selected_color_;
}

void ColorSwatchWidget::SetColorProcessor(ColorProcessorPtr to_linear, ColorProcessorPtr to_display)
{
  to_linear_processor_ = to_linear;
  to_display_processor_ = to_display;

  // Force full update
  SelectedColorChangedEvent(GetSelectedColor(), true);
  update();
}

void ColorSwatchWidget::SetSelectedColor(const Color &c)
{
  SetSelectedColorInternal(c, true);
}

void ColorSwatchWidget::mousePressEvent(QMouseEvent *e)
{
  QWidget::mousePressEvent(e);

  SetSelectedColorInternal(GetColorFromScreenPos(e->pos()), false);
  emit SelectedColorChanged(GetSelectedColor());
}

void ColorSwatchWidget::mouseMoveEvent(QMouseEvent *e)
{
  QWidget::mouseMoveEvent(e);

  if (e->buttons() & Qt::LeftButton) {
    SetSelectedColorInternal(GetColorFromScreenPos(e->pos()), false);
    emit SelectedColorChanged(GetSelectedColor());
  }
}

void ColorSwatchWidget::SelectedColorChangedEvent(const Color &, bool)
{
}

Qt::GlobalColor ColorSwatchWidget::GetUISelectorColor() const
{
  float rough_color_luma = (GetSelectedColor().red()+GetSelectedColor().red()+GetSelectedColor().blue()+GetSelectedColor().green()+GetSelectedColor().green()+GetSelectedColor().green())/6;

  if (rough_color_luma > 0.66) {
    return Qt::black;
  } else {
    return Qt::white;
  }
}

Color ColorSwatchWidget::GetManagedColor(const Color &input) const
{
  if (to_linear_processor_ && to_display_processor_) {
    return to_display_processor_->ConvertColor(to_linear_processor_->ConvertColor(input));
  }

  return input;
}

void ColorSwatchWidget::SetSelectedColorInternal(const Color &c, bool external)
{
  selected_color_ = c;
  SelectedColorChangedEvent(c, external);
  update();
}
