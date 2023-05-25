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

#include "floatsliderparamwidget.h"

#include "widget/slider/floatslider.h"

namespace olive {

#define super NumericSliderParamWidget

FloatSliderParamWidget::FloatSliderParamWidget(QObject *parent)
  : super{parent}
{
}

void FloatSliderParamWidget::Initialize(QWidget *parent, size_t channels)
{
  CreateSliders<FloatSlider>(parent, channels);
}

void FloatSliderParamWidget::SetValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    static_cast<FloatSlider*>(GetWidgets().at(i))->SetValue(val.value<double>(i));
  }
}

void FloatSliderParamWidget::SetDefaultValue(const value_t &val)
{
  for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
    static_cast<FloatSlider*>(GetWidgets().at(i))->SetDefaultValue(val.value<double>(i));
  }
}

void FloatSliderParamWidget::SetProperty(const QString &key, const value_t &val)
{
  if (key == QStringLiteral("min")) {
    for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
      static_cast<FloatSlider*>(GetWidgets().at(i))->SetMinimum(val.value<double>(i));
    }
  } else if (key == QStringLiteral("max")) {
    for (size_t i = 0; i < val.size() && i < GetWidgets().size(); i++) {
      static_cast<FloatSlider*>(GetWidgets().at(i))->SetMaximum(val.value<double>(i));
    }
  } else if (key == QStringLiteral("view")) {
    FloatSlider::DisplayType display_type = static_cast<FloatSlider::DisplayType>(val.toInt());

    foreach (QWidget* w, GetWidgets()) {
      static_cast<FloatSlider*>(w)->SetDisplayType(display_type);
    }
  } else if (key == QStringLiteral("decimalplaces")) {
    int dec_places = val.toInt();

    foreach (QWidget* w, GetWidgets()) {
      static_cast<FloatSlider*>(w)->SetDecimalPlaces(dec_places);
    }
  } else if (key == QStringLiteral("autotrim")) {
    bool autotrim = val.toBool();

    foreach (QWidget* w, GetWidgets()) {
      static_cast<FloatSlider*>(w)->SetAutoTrimDecimalPlaces(autotrim);
    }
  } else if (key == QStringLiteral("offset")) {
    for (size_t i=0; i<GetWidgets().size(); i++) {
      static_cast<FloatSlider*>(GetWidgets().at(i))->SetOffset(val.value<double>(i));
    }
  } else {
    super::SetProperty(key, val);
  }
}

}
