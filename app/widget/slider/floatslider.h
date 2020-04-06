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

#ifndef FLOATSLIDER_H
#define FLOATSLIDER_H

#include "sliderbase.h"

OLIVE_NAMESPACE_ENTER

class FloatSlider : public SliderBase
{
  Q_OBJECT
public:
  FloatSlider(QWidget* parent = nullptr);

  enum DisplayType {
    kNormal,
    kDecibel,
    kPercentage
  };

  double GetValue();

  void SetValue(const double& d);

  void SetMinimum(const double& d);

  void SetMaximum(const double& d);

  void SetDecimalPlaces(int i);

  void SetDisplayType(const DisplayType& type);

protected:
  virtual QString ValueToString(const QVariant& v) override;

  virtual QVariant StringToValue(const QString& s, bool* ok) override;

  virtual double AdjustDragDistanceInternal(const double& start, const double& drag) override;

signals:
  void ValueChanged(double);

private slots:
  void ConvertValue(QVariant v);

private:
  DisplayType display_type_;
};

OLIVE_NAMESPACE_EXIT

#endif // FLOATSLIDER_H
