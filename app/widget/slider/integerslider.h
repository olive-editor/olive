/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

class IntegerSlider : public SliderBase
{
  Q_OBJECT
public:
  IntegerSlider(QWidget* parent = nullptr);

  int GetValue();

  void SetValue(const int& v);

  void SetMinimum(const int& d);

  void SetMaximum(const int& d);

signals:
  void ValueChanged(int);

private slots:
  void ConvertValue(QVariant v);
};

#endif // INTEGERSLIDER_H
