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

#include "fontfield.h"

#include <QFontDatabase>
#include <QDebug>

#include "ui/comboboxex.h"
#include "nodes/node.h"
#include "undo/undo.h"

// NOTE/TODO: This shares a lot of similarity with ComboInput, and could probably be a derived class of it

FontField::FontField(EffectRow* parent) :
  EffectField(parent, EffectField::EFFECT_FIELD_FONT)
{
  font_list = QFontDatabase().families();

  SetValueAt(0, font_list.first());
}

QString FontField::GetFontAt(double timecode)
{
  return GetValueAt(timecode).toString();
}

QWidget *FontField::CreateWidget(QWidget *existing)
{
  ComboBoxEx* fcb = new ComboBoxEx();

  if (existing == nullptr) {

    fcb = new ComboBoxEx();

    fcb->setScrollingEnabled(false);

    fcb->addItems(font_list);

  } else {

    fcb = static_cast<ComboBoxEx*>(existing);

  }

  connect(fcb, SIGNAL(currentTextChanged(const QString &)), this, SLOT(UpdateFromWidget(const QString &)));
  connect(this, SIGNAL(EnabledChanged(bool)), fcb, SLOT(setEnabled(bool)));

  return fcb;
}

void FontField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  QVariant data = GetValueAt(timecode);

  ComboBoxEx* cb = static_cast<ComboBoxEx*>(widget);

  for (int i=0;i<font_list.size();i++) {
    if (font_list.at(i) == data) {
      cb->blockSignals(true);
      cb->setCurrentIndex(i);
      cb->blockSignals(false);
      return;
    }
  }

  qWarning() << "Failed to set FontField value from data";
}

void FontField::UpdateFromWidget(const QString& s)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(GetParentRow()->GetParentEffect()->Now(), s);

  kdc->SetNewKeyframes();
  olive::undo_stack.push(kdc);
}
