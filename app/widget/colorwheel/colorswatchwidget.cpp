#include "colorswatchwidget.h"

#include <QMouseEvent>

#include "render/backend/opengl/openglrenderfunctions.h"

ColorSwatchWidget::ColorSwatchWidget(QWidget *parent) :
  QWidget(parent)
{
}

const Color &ColorSwatchWidget::GetSelectedColor() const
{
  return selected_color_;
}

void ColorSwatchWidget::SetSelectedColor(const Color &c)
{
  selected_color_ = c;
  SelectedColorChangedEvent(c);
  update();
}

void ColorSwatchWidget::mousePressEvent(QMouseEvent *e)
{
  QWidget::mousePressEvent(e);

  SetSelectedColor(GetColorFromScreenPos(e->pos()));
  emit SelectedColorChanged(GetSelectedColor());
}

void ColorSwatchWidget::mouseMoveEvent(QMouseEvent *e)
{
  QWidget::mouseMoveEvent(e);

  if (e->buttons() & Qt::LeftButton) {
    SetSelectedColor(GetColorFromScreenPos(e->pos()));
    emit SelectedColorChanged(GetSelectedColor());
  }
}

void ColorSwatchWidget::SelectedColorChangedEvent(const Color &)
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
