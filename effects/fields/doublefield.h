/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#ifndef DOUBLEFIELD_H
#define DOUBLEFIELD_H

#include "../effectfield.h"
#include "ui/labelslider.h"

class DoubleField : public EffectField
{
  Q_OBJECT
public:
  DoubleField(EffectRow* parent, const QString& id);

  /**
   * @brief Get double value at timecode
   *
   * Convenience function. Equivalent to GetValueAt().toDouble()
   *
   * @param timecode
   *
   * Timecode to retrieve value at
   *
   * @return
   *
   * Double value at the set timecode
   */
  double GetDoubleAt(double timecode);

  void SetMinimum(double minimum);
  void SetMaximum(double maximum);
  void SetDefault(double maximum);

  void SetDisplayType(LabelSlider::DisplayType type);
  void SetFrameRate(const double& rate);

  virtual QVariant ConvertStringToValue(const QString& s) override;
  virtual QString ConvertValueToString(const QVariant& v) override;

  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;
signals:
  void MaximumChanged(double maximum);
  void MinimumChanged(double maximum);
private:
  double min_;
  double max_;
  double default_;

  LabelSlider::DisplayType display_type_;
  double frame_rate_;

  bool value_set_;

  KeyframeDataChange* kdc_;
private slots:
  void ValueHasBeenSet();
  void UpdateFromWidget(double d);
};

#endif // DOUBLEFIELD_H
