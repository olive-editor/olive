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

#include "rationalsliderparamwidget.h"

#include "widget/slider/rationalslider.h"

namespace olive {

#define super NumericSliderParamWidget

RationalSliderParamWidget::RationalSliderParamWidget(QObject *parent)
  : NumericSliderParamWidget{parent}
{

}

void RationalSliderParamWidget::Initialize(QWidget *parent, size_t channels)
{
  CreateSliders<RationalSlider>(parent, channels);
}

void RationalSliderParamWidget::SetValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    static_cast<RationalSlider*>(GetWidgets().at(i))->SetValue(val.value<rational>(i));
  }
}

void RationalSliderParamWidget::SetDefaultValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    static_cast<RationalSlider*>(GetWidgets().at(i))->SetDefaultValue(val.value<rational>(i));
  }
}

void RationalSliderParamWidget::SetProperty(const QString &key, const value_t &val)
{
  if (key == QStringLiteral("view")) {
    RationalSlider::DisplayType display_type = static_cast<RationalSlider::DisplayType>(val.toInt());

    foreach (QWidget* w, GetWidgets()) {
      static_cast<RationalSlider*>(w)->SetDisplayType(display_type);
    }
  } else if (key == QStringLiteral("viewlock")) {
    bool locked = val.toBool();

    foreach (QWidget* w, GetWidgets()) {
      static_cast<RationalSlider*>(w)->SetLockDisplayType(locked);
    }
  } else {
    super::SetProperty(key, val);
  }
}

void RationalSliderParamWidget::SetTimebase(const rational &timebase)
{
  for (QWidget *w : GetWidgets()) {
    static_cast<RationalSlider*>(w)->SetTimebase(timebase);
  }
}

}
