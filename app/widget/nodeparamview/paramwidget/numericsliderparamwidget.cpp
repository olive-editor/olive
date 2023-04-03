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

#include "numericsliderparamwidget.h"

#include "widget/slider/base/numericsliderbase.h"

namespace olive {

#define super AbstractParamWidget

NumericSliderParamWidget::NumericSliderParamWidget(QObject *parent)
  : AbstractParamWidget{parent}
{

}

void NumericSliderParamWidget::SetProperty(const QString &key, const value_t &val)
{
  if (key == QStringLiteral("color")) {
    QColor c(val.toString());

    if (key.size() == 5) {
      // Set for all tracks
      for (size_t i=0; i<GetWidgets().size(); i++) {
        static_cast<SliderBase*>(GetWidgets().at(i))->SetColor(c);
      }
    } else {
      bool ok;
      size_t element = key.midRef(5).toInt(&ok);
      if (ok && element < GetWidgets().size()) {
        static_cast<SliderBase*>(GetWidgets().at(element))->SetColor(c);
      }
    }
  } else if (key == QStringLiteral("base")) {
    double d = val.toDouble();
    for (size_t i=0; i<GetWidgets().size(); i++) {
      static_cast<NumericSliderBase*>(GetWidgets().at(i))->SetDragMultiplier(d);
    }
  } else {
    super::SetProperty(key, val);
  }
}

}
