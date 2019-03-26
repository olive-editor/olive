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

#ifndef BOOLFIELD_H
#define BOOLFIELD_H

#include "../effectfield.h"

/**
 * @brief The BoolField class
 *
 * An EffectField derivative the produces boolean values (true or false) and uses a checkbox as its visual representation.
 */
class BoolField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  BoolField(EffectRow* parent, const QString& id);

  /**
   * @brief Get the boolean value at a given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toBool()
   *
   * @param timecode
   *
   * The timecode to retrieve the value at
   *
   * @return
   *
   * The boolean value at this timecode
   */
  bool GetBoolAt(double timecode);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a QCheckBox.
   */
  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

  /**
   * @brief Reimplementation of EffectField::ConvertStringToValue()
   */
  virtual QVariant ConvertStringToValue(const QString& s) override;

  /**
   * @brief Reimplementation of EffectField::ConvertValueToString()
   */
  virtual QString ConvertValueToString(const QVariant& v) override;
signals:
  /**
   * @brief Emitted whenever the UI widget's boolean value has changed
   *
   * For any QCheckBox created through this field's CreateWidget() function, this signal is emitted any time the
   * checkbox value changes (either through user intervention or keyframing). It is mostly useful for
   * enabling/disabling/changing other UI elements based on the checked
   * state of this field's value (e.g. enabling other fields if this field is checked).
   *
   * It is NOT a reliable signal that the value has changed at all, as it is only emitted if a widget (created
   * from CreateWidget() ) is currently active.
   */
  void Toggled(bool);
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current checked state of the QWidget (QCheckBox in this case). Automatically set when this slot is connected
   * to the QCheckBox::toggled() signal.
   */
  void UpdateFromWidget(bool b);
};

#endif // BOOLFIELD_H
