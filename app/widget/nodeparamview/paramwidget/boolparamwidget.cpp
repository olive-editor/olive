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

#include "boolparamwidget.h"

#include <QCheckBox>

namespace olive {

BoolParamWidget::BoolParamWidget(QObject *parent)
  : AbstractParamWidget{parent}
{

}

void BoolParamWidget::Initialize(QWidget *parent, size_t channels)
{
  for (size_t i = 0; i < channels; i++) {
    QCheckBox *c = new QCheckBox(parent);
    connect(c, &QCheckBox::clicked, this, &BoolParamWidget::Arbitrate);
    AddWidget(c);
  }
}

void BoolParamWidget::SetValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    static_cast<QCheckBox*>(GetWidgets().at(i))->setChecked(val.value<int64_t>(i));
  }
}

void BoolParamWidget::Arbitrate()
{
  QCheckBox *w = static_cast<QCheckBox *>(sender());

  for (size_t i = 0; i < GetWidgets().size(); i++) {
    if (GetWidgets().at(i) == w) {
      emit ChannelValueChanged(i, int64_t(w->isChecked()));
      break;
    }
  }
}

}
