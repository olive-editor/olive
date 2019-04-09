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

#include "combofield.h"

#include <QDebug>

#include "ui/comboboxex.h"

ComboField::ComboField(EffectRow* parent, const QString& id) :
  EffectField(parent, id, olive::nodes::kCombo)
{}

void ComboField::AddItem(const QString &text, const QVariant &data)
{
  ComboFieldItem item;

  item.name = text;
  item.data = data;

  items_.append(item);
}

QWidget *ComboField::CreateWidget(QWidget *existing)
{
  ComboBoxEx* cb;

  if (existing == nullptr) {
    cb = new ComboBoxEx();

    cb->setScrollingEnabled(false);

    for (int i=0;i<items_.size();i++) {
      cb->addItem(items_.at(i).name);
    }
  } else {
    cb = static_cast<ComboBoxEx*>(existing);
  }

  connect(cb, SIGNAL(activated(int)), this, SLOT(UpdateFromWidget(int)));
  connect(this, SIGNAL(EnabledChanged(bool)), cb, SLOT(setEnabled(bool)));

  return cb;
}

void ComboField::UpdateWidgetValue(QWidget *widget, double timecode)
{
  QVariant data = GetValueAt(timecode);

  ComboBoxEx* cb = static_cast<ComboBoxEx*>(widget);

  for (int i=0;i<items_.size();i++) {
    if (items_.at(i).data == data) {
      cb->blockSignals(true);
      cb->setCurrentIndex(i);
      cb->blockSignals(false);

      emit DataChanged(data);
      return;
    }
  }

  qWarning() << "Failed to set ComboField value from data";
}

void ComboField::UpdateFromWidget(int index)
{
  KeyframeDataChange* kdc = new KeyframeDataChange(this);

  SetValueAt(Now(), items_.at(index).data);

  kdc->SetNewKeyframes();
  olive::undo_stack.push(kdc);
}
