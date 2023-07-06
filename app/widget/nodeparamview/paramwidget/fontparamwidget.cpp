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

#include "fontparamwidget.h"

#include <QFontComboBox>

namespace olive {

FontParamWidget::FontParamWidget(QObject *parent)
  : AbstractParamWidget{parent}
{

}

void FontParamWidget::Initialize(QWidget *parent, size_t channels)
{
  for (size_t i = 0; i < channels; i++) {
    QFontComboBox* font_combobox = new QFontComboBox(parent);
    AddWidget(font_combobox);
    connect(font_combobox, &QFontComboBox::currentFontChanged, this, &FontParamWidget::Arbitrate);
  }
}

void FontParamWidget::SetValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    QFontComboBox* fc = static_cast<QFontComboBox*>(GetWidgets().at(i));
    fc->blockSignals(true);
    fc->setCurrentFont(val.value<QString>(i));
    fc->blockSignals(false);
  }
}

void FontParamWidget::Arbitrate()
{
  QFontComboBox* ff = static_cast<QFontComboBox*>(sender());

  for (size_t i = 0; i < GetWidgets().size(); i++) {
    if (GetWidgets().at(i) == ff) {
      emit ChannelValueChanged(i, ff->currentFont().toString());
      break;
    }
  }
}

}
