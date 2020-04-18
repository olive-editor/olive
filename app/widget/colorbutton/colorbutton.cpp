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

#include "colorbutton.h"

#include "dialog/color/colordialog.h"

OLIVE_NAMESPACE_ENTER

ColorButton::ColorButton(ColorManager* color_manager, QWidget *parent) :
  QPushButton(parent),
  color_manager_(color_manager),
  color_(1.0f, 1.0f, 1.0f),
  color_processor_(nullptr)
{
  setAutoFillBackground(true);

  connect(this, &ColorButton::clicked, this, &ColorButton::ShowColorDialog);

  UpdateColor();
}

const ManagedColor &ColorButton::GetColor() const
{
  return color_;
}

void ColorButton::SetColor(const ManagedColor &c)
{
  color_ = c;

  UpdateColor();
}

void ColorButton::ShowColorDialog()
{
  ColorDialog cd(color_manager_, color_, this);

  if (cd.exec() == QDialog::Accepted) {
    color_ = cd.GetSelectedColor();

    UpdateColor();

    emit ColorChanged(color_);
  }
}

void ColorButton::UpdateColor()
{
  color_processor_ = ColorProcessor::Create(color_manager_,
                                            color_.color_input(),
                                            color_.color_display(),
                                            color_.color_view(),
                                            color_.color_look());

  QColor managed = color_processor_->ConvertColor(color_).toQColor();

  setStyleSheet(QStringLiteral("%1--ColorButton {background: %2;}").arg(MACRO_VAL_AS_STR(OLIVE_NAMESPACE), managed.name()));
}

OLIVE_NAMESPACE_EXIT
