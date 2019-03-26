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

#ifndef COLORFIELD_H
#define COLORFIELD_H

#include "../effectfield.h"

/**
 * @brief The ColorField class
 *
 * An EffectField derivative that produces color values and uses a ColorButton as its UI representative.
 */
class ColorField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  ColorField(EffectRow* parent, const QString& id);

  /**
   * @brief Get the color value at a given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).value<QColor>().
   *
   * @param timecode
   *
   * The timecode to retrieve the color at
   *
   * @return
   *
   * The color value at this timecode
   */
  QColor GetColorAt(double timecode);

  /**
   * @brief CreateWidget
   *
   * Creates and connects to a ColorButton.
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
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current color selected by the QWidget (ColorButton in this case). Automatically triggered when this slot is
   * connected to the ColorButton::color_changed() signal.
   */
  void UpdateFromWidget(const QColor &c);
};

#endif // COLORFIELD_H
