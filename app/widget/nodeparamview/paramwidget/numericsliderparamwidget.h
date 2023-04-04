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

#ifndef NUMERICSLIDERPARAMWIDGET_H
#define NUMERICSLIDERPARAMWIDGET_H

#include "abstractparamwidget.h"
#include "common/qtutils.h"

namespace olive {

class NumericSliderParamWidget : public AbstractParamWidget
{
  Q_OBJECT
public:
  explicit NumericSliderParamWidget(QObject *parent = nullptr);

  virtual void SetProperty(const QString &key, const value_t &val) override;

protected:
  template <typename T>
  void CreateSliders(QWidget *parent, size_t channels)
  {
    for (size_t i = 0; i < channels; i++) {
      T *s = new T(parent);

      s->SetLadderElementCount(2);

      // HACK: Force some spacing between sliders
      s->setContentsMargins(0, 0, QtUtils::QFontMetricsWidth(s->fontMetrics(), QStringLiteral("        ")), 0);

      connect(s, &T::ValueChanged, this, &NumericSliderParamWidget::ArbitrateSliders);

      AddWidget(s);
    }
  }

};

}

#endif // NUMERICSLIDERPARAMWIDGET_H
