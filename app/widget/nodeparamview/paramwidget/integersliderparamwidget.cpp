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

#include "integersliderparamwidget.h"

#include "widget/slider/integerslider.h"

namespace olive {

#define super NumericSliderParamWidget

IntegerSliderParamWidget::IntegerSliderParamWidget(QObject *parent) :
  super(parent)
{
}

void IntegerSliderParamWidget::Initialize(QWidget *parent, size_t channels)
{
  CreateSliders<IntegerSlider>(parent, channels);
}

void IntegerSliderParamWidget::SetValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    static_cast<IntegerSlider*>(GetWidgets().at(i))->SetValue(val.value<int64_t>(i));
  }
}

void IntegerSliderParamWidget::SetProperty(const QString &key, const value_t &val)
{
  if (key == QStringLiteral("min")) {
    for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
      static_cast<IntegerSlider*>(GetWidgets().at(i))->SetMinimum(val.value<int64_t>(i));
    }
  } else if (key == QStringLiteral("max")) {
    for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
      static_cast<IntegerSlider*>(GetWidgets().at(i))->SetValue(val.value<int64_t>(i));
    }
  } else if (key == QStringLiteral("offset")) {
    for (size_t i=0; i<GetWidgets().size(); i++) {
      static_cast<IntegerSlider*>(GetWidgets().at(i))->SetOffset(val.value<int64_t>(i));
    }
  } else {
    super::SetProperty(key, val);
  }
}

}
