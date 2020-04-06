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

#include "colorpreviewbox.h"

#include <QPainter>

OLIVE_NAMESPACE_ENTER

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

OLIVE_NAMESPACE_EXIT
