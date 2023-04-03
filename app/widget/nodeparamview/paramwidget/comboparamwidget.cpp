/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#include "comboparamwidget.h"

#include <QComboBox>

namespace olive {

#define super AbstractParamWidget

ComboParamWidget::ComboParamWidget(QObject *parent)
  : super{parent}
{
}

void ComboParamWidget::Initialize(QWidget *parent, size_t channels)
{
  for (size_t i = 0; i < channels; i++) {
    QComboBox* combobox = new QComboBox(parent);
    AddWidget(combobox);
    connect(combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &ComboParamWidget::ArbitrateCombo);
  }
}

void ComboParamWidget::SetValue(const value_t &val)
{
  for (size_t j = 0; j < val.size() && j < GetWidgets().size(); j++) {
    QComboBox* cb = static_cast<QComboBox*>(GetWidgets().at(j));
    cb->blockSignals(true);
    int index = val.value<int64_t>(j);
    for (int i=0; i<cb->count(); i++) {
      if (cb->itemData(i).toInt() == index) {
        cb->setCurrentIndex(i);
      }
    }
    cb->blockSignals(false);
  }
}

void ComboParamWidget::SetProperty(const QString &key, const value_t &val)
{
  if (key == QStringLiteral("combo_str")) {
    for (size_t j = 0; j < val.size() && j < GetWidgets().size(); j++) {
      QComboBox* cb = static_cast<QComboBox*>(GetWidgets().at(j));

      int old_index = cb->currentIndex();

      // Block the combobox changed signals since we anticipate the index will be the same and not require a re-render
      cb->blockSignals(true);

      cb->clear();

      QStringList items = val.value<QStringList>(j);
      int index = 0;
      foreach (const QString& s, items) {
        if (s.isEmpty()) {
          cb->insertSeparator(cb->count());
          cb->setItemData(cb->count()-1, -1);
        } else {
          cb->addItem(s, index);
          index++;
        }
      }

      cb->setCurrentIndex(old_index);

      cb->blockSignals(false);

      // In case the amount of items is LESS and the previous index cannot be set, NOW we trigger a re-cache since the
      // value has changed
      if (cb->currentIndex() != old_index) {
        ArbitrateSpecificCombo(cb);
      }
    }
  } else {
    super::SetProperty(key, val);
  }
}

void ComboParamWidget::ArbitrateSpecificCombo(QComboBox *cb)
{
  size_t channel = 0;
  for (size_t i = 0; i < GetWidgets().size(); i++) {
    if (GetWidgets().at(i) == cb) {
      channel = i;
      break;
    }
  }

  int64_t index = cb->currentIndex();

  // Subtract any splitters up until this point
  for (int i=index-1; i>=0; i--) {
    if (cb->itemData(i, Qt::AccessibleDescriptionRole).toString() == QStringLiteral("separator")) {
      index--;
    }
  }

  emit ChannelValueChanged(channel, index);
}

void ComboParamWidget::ArbitrateCombo()
{
  // Widget is a QComboBox
  QComboBox* cb = static_cast<QComboBox*>(sender());
  ArbitrateSpecificCombo(cb);
}

}
