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

#ifndef FONTFIELD_H
#define FONTFIELD_H

#include "combofield.h"

/**
 * @brief The FontField class
 *
 * An EffectField derivative the produces font family names in string and uses a QComboBox
 * as its visual representation.
 *
 * TODO Upgrade to QFontComboBox.
 */
class FontField : public EffectField {
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  FontField(NodeIO* parent);

  /**
   * @brief Get the font family name at the given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toString()
   *
   * @param timecode
   *
   * The timecode to retrieve the font family name at
   *
   * @return
   *
   * The font family name at this timecode
   */
  QString GetFontAt(double timecode);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a QComboBox.
   */
  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;

private:
  /**
   * @brief Internal list of fonts to add to a QComboBox when creating one in CreateWidget().
   *
   * NOTE: Deprecated. Once QComboBox is replaced by QFontComboBox this will be completely unnecessary.
   */
  QStringList font_list;
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current font name specified by the QWidget (QComboBox in this case). Automatically set when this slot
   * is connected to the QComboBox::currentTextChanged() signal.
   */
  void UpdateFromWidget(const QString& index);
};

#endif // FONTFIELD_H
