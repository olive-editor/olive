/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

namespace olive {

ColorButton::ColorButton(ColorManager* color_manager, bool show_dialog_on_click, QWidget *parent) :
  QPushButton(parent),
  color_manager_(color_manager),
  color_processor_(nullptr)
{
  setAutoFillBackground(true);

  if (show_dialog_on_click) {
    connect(this, &ColorButton::clicked, this, &ColorButton::ShowColorDialog);
  }

  SetColor(Color(1.0f, 1.0f, 1.0f));
}

const ManagedColor &ColorButton::GetColor() const
{
  return color_;
}

void ColorButton::SetColor(const ManagedColor &c)
{
  color_ = c;

  color_.set_color_input(color_manager_->GetCompliantColorSpace(color_.color_input()));
  color_.set_color_output(color_manager_->GetCompliantColorSpace(color_.color_output()));

  UpdateColor();
}

void ColorButton::ShowColorDialog()
{
  ColorDialog *cd = new ColorDialog(color_manager_, color_, this);

  connect(cd, &ColorDialog::finished, this, &ColorButton::ColorDialogFinished);

  cd->show();
}

void ColorButton::ColorDialogFinished(int e)
{
  ColorDialog *cd = static_cast<ColorDialog*>(sender());

  if (e == QDialog::Accepted) {
    color_ = cd->GetSelectedColor();

    UpdateColor();

    emit ColorChanged(color_);
  }

  cd->deleteLater();
}

void ColorButton::UpdateColor()
{
  color_processor_ = ColorProcessor::Create(color_manager_,
                                            color_.color_input(),
                                            color_.color_output());

  QColor managed = color_processor_->ConvertColor(color_).toQColor();

  setStyleSheet(QStringLiteral("%1--ColorButton {background: %2;}").arg(MACRO_VAL_AS_STR(olive), managed.name()));
}

}
