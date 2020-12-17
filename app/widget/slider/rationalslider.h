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

#include <QMouseEvent>

#include "common/rational.h"

namespace olive {

 /**
 * @brief A olive::rational based slider
 * 
 * A slider that can display rationals as either timecode, a timestamp (frames), a rational (a/b)
 * or a float (seconds). 
 * 
 * Control clikcing the slider (see sliderlabel.h) changes thedisplay type
 */
class RationalSlider : public SliderBase
{
  Q_OBJECT
public:
  /**
   * @brief enum containing the possibly display types
   */
  enum DisplayType {
    kTimecode,
    kTimestamp,
    kRational,
    kFloat
  };

  RationalSlider(rational timebase, QWidget* parent = nullptr);

  /**
   * @brief Returns the sliders value as a rational
   */
  rational GetValue();

  /**
   * @brief Sets the sliders value
   */
  void SetValue(const rational& d);

  /**
   * @brief Sets the sliders default value
   */
  void SetDefaultValue(const rational& r);

  /**
   * @brief Sets the sliders minimum value
   */
  void SetMinimum(const rational& d);

  /**
   * @brief Sets the sliders maximum value
   */
  void SetMaximum(const rational& d);

  /**
   * @brief Sets the number of decimal places the slider shows when displaying a float
   */
  void SetDecimalPlaces(int i);

  /**
   * @brief Sets the sliders timebase which is also the minimum increment of the slider
   */
  void SetTimebase(const rational& timebase);

  /**
   * @brief Sets the display type of the slider
   */
  void SetDisplayType(const DisplayType& type);

  void SetLockDisplayType(bool e);

  bool LockDisplayType();

  void SetAutoTrimDecimalPlaces(bool e);

protected:
  virtual QString ValueToString(const QVariant& v) override;

  virtual QVariant StringToValue(const QString& s, bool* ok) override;

  virtual double AdjustDragDistanceInternal(const double& start, const double& drag) override;

signals:
  void ValueChanged(rational);

private slots:
  void ConvertValue(QVariant v);

  void changeDisplayType();

  /**
   * @brief Changes the timecodes display type (e.g. drop frome to none drop frame)
   */
  void ChangeTimecodeDisplayType();

private:
  DisplayType display_type_;

  int decimal_places_;

  bool autotrim_decimal_places_;

  rational timebase_;

  bool lock_display_type_;
};

}

#endif // RATIONALSLIDER_H
