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

#ifndef RATIONALSLIDER_H
#define RATIONALSLIDER_H

#include "sliderbase.h"

#include "common/rational.h"

namespace olive {

class RationalSlider : public SliderBase
{
  Q_OBJECT
public:
  RationalSlider(QWidget* parent = nullptr);

  enum DisplayType {
    kTimecode,
    kTimestamp,
    kRational,
    kFloat
  };

  rational GetValue();

  void SetValue(const rational& d);

  void SetMinimum(const rational& d);

  void SetMaximum(const rational& d);

  void SetDecimalPlaces(int i);

  void SetTimebase(const rational& timebase);

  void SetDisplayType(const DisplayType& type);

  void SetAutoTrimDecimalPlaces(bool e);

protected:
  virtual QString ValueToString(const QVariant& v) override;

  virtual QVariant StringToValue(const QString& s, bool* ok) override;

  virtual double AdjustDragDistanceInternal(const double& start, const double& drag) override;

signals:
  void ValueChanged(rational);

private slots:
  void ConvertValue(QVariant v);

private:
  DisplayType display_type_;

  int decimal_places_;

  bool autotrim_decimal_places_;

  rational timebase_;

};

}

#endif // RATIONALSLIDER_H
