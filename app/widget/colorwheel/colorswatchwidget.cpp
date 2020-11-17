/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "colorswatchwidget.h"

#include <QMouseEvent>

namespace olive {

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
  if (GetSelectedColor().GetRoughLuminance() > 0.66) {
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

}
