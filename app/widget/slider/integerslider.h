/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef INTEGERSLIDER_H
#define INTEGERSLIDER_H

#include "sliderbase.h"

namespace olive {

class IntegerSlider : public SliderBase
{
  Q_OBJECT
public:
  IntegerSlider(QWidget* parent = nullptr);

  int64_t GetValue();

  void SetValue(const int64_t& v);

  void SetMinimum(const int64_t& d);

  void SetMaximum(const int64_t& d);

protected:
  virtual QString ValueToString(const QVariant& v) override;

  virtual QVariant StringToValue(const QString& s, bool* ok) override;

signals:
  void ValueChanged(int64_t);

private slots:
  void ConvertValue(QVariant v);
};

}

#endif // INTEGERSLIDER_H
