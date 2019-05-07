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

#include "labelfield.h"

#include <QLabel>

LabelField::LabelField(NodeIO *parent, const QString &string) :
  EffectField(parent, EffectField::EFFECT_FIELD_UI),
  label_text_(string)
{}

QWidget *LabelField::CreateWidget(QWidget *existing)
{
  QLabel* label;

  if (existing == nullptr) {

    label = new QLabel(label_text_);

    label->setEnabled(IsEnabled());

  } else {

    label = static_cast<QLabel*>(existing);

  }

  connect(this, SIGNAL(EnabledChanged(bool)), label, SLOT(setEnabled(bool)));

  return label;
}
