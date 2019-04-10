/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "buttonfield.h"

#include <QPushButton>

ButtonField::ButtonField(EffectRow *parent) :
  EffectField(parent, EffectField::EFFECT_FIELD_UI),
  button_text_(string)
{}

void ButtonField::SetCheckable(bool c)
{
  checkable_ = c;
}

void ButtonField::SetChecked(bool c)
{
  checked_ = c;
  emit CheckedChanged(c);
}

QWidget *ButtonField::CreateWidget(QWidget *existing)
{
  QPushButton* button;

  if (existing == nullptr) {

    button = new QPushButton();

    button->setCheckable(checkable_);
    button->setEnabled(IsEnabled());
    button->setText(button_text_);

  } else {

    button = static_cast<QPushButton*>(existing);

  }

  connect(this, SIGNAL(CheckedChanged(bool)), button, SLOT(setChecked(bool)));
  connect(this, SIGNAL(EnabledChanged(bool)), button, SLOT(setEnabled(bool)));
  connect(button, SIGNAL(clicked(bool)), this, SIGNAL(Clicked()));
  connect(button, SIGNAL(toggled(bool)), this, SLOT(SetChecked(bool)));
  connect(button, SIGNAL(toggled(bool)), this, SIGNAL(Toggled(bool)));

  return button;
}
