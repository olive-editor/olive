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

#ifndef STRINGFIELD_H
#define STRINGFIELD_H

#include "../effectfield.h"

/**
 * @brief The StringField class
 *
 * An EffectField derivative that produces arbitrary strings entered by the user and uses a TextEditEx as its
 * visual representation.
 */
class StringField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   *
   * Provides a setting for whether this StringField - and its attached TextEditEx objects - should operate in rich
   * text or plain text mode, defaulting to rich text mode.
   */
  StringField(NodeIO* parent, bool rich_text = true);

  /**
   * @brief Get the string at the given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toString()
   *
   * @param timecode
   *
   * The timecode to retrieve the string at
   *
   * @return
   *
   * The string at this timecode
   */
  QString GetStringAt(double timecode);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a TextEditEx.
   */
  virtual QWidget *CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget* widget, double timecode) override;
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current checked state of the QWidget (EmbeddedFileChooser in this case). Automatically set when this slot
   * is connected to the EmbeddedFileChooser::changed() signal.
   */
  void UpdateFromWidget(const QString& b);
private:
  /**
   * @brief Internal value for whether this field is in rich text or plain text mode
   */
  bool rich_text_;
};

#endif // STRINGFIELD_H
