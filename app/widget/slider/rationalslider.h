/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include <QMouseEvent>

#include "base/decimalsliderbase.h"
#include "common/rational.h"

namespace olive {

 /**
 * @brief A olive::rational based slider
 *
 * A slider that can display rationals as either timecode (drop or non-drop), a timestamp (frames),
 * or a float (seconds).
 */
class RationalSlider : public DecimalSliderBase
{
  Q_OBJECT
public:
  /**
   * @brief enum containing the possibly display types
   */
  enum DisplayType {
    kTime,
    kFloat,
    kRational
  };

  RationalSlider(QWidget* parent = nullptr);

  /**
   * @brief Returns the sliders value as a rational
   */
  rational GetValue();

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
   * @brief Sets the display type of the slider
   */
  void SetDisplayType(const DisplayType& type);

  /**
   * @brief Set whether the user can change the display type or not
   */
  void SetLockDisplayType(bool e);

  /**
   * @brief Get whether the user can change the display type or not
   */
  bool GetLockDisplayType();

  /**
   * @brief Hide display type in menu
   */
  void DisableDisplayType(DisplayType type);

public slots:
  /**
   * @brief Sets the sliders timebase which is also the minimum increment of the slider
   */
  void SetTimebase(const rational& timebase);

  /**
   * @brief Sets the sliders value
   */
  void SetValue(const rational& d);

protected:
  virtual QString ValueToString(const QVariant& v) const override;

  virtual QVariant StringToValue(const QString& s, bool* ok) const override;

  virtual QVariant AdjustDragDistanceInternal(const QVariant &start, const double &drag) const override;

  virtual void ValueSignalEvent(const QVariant& v) override;

  virtual bool ValueGreaterThan(const QVariant& lhs, const QVariant& rhs) const override;

  virtual bool ValueLessThan(const QVariant& lhs, const QVariant& rhs) const override;

signals:
  void ValueChanged(rational);

private slots:
  void ShowDisplayTypeMenu();

  void SetDisplayTypeFromMenu();

private:
  DisplayType display_type_;

  rational timebase_;

  bool lock_display_type_;

  QVector<DisplayType> disabled_;

};

}

#endif // RATIONALSLIDER_H
