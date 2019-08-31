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

#ifndef STRINGSLIDER_H
#define STRINGSLIDER_H

#include "sliderbase.h"

class StringSlider : public SliderBase
{
  Q_OBJECT
public:
  StringSlider(QWidget* parent = nullptr);

  void SetDragMultiplier(const double& d) = delete;

  QString GetValue();

  void SetValue(const QString& v);

signals:
  void ValueChanged(QString);

private slots:
  void ConvertValue(QVariant v);
};

#endif // STRINGSLIDER_H
