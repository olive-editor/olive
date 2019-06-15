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

#ifndef FILEFIELD_H
#define FILEFIELD_H

#include "../effectfield.h"

/**
 * @brief The FileInput class
 *
 * An EffectField derivative that produces filenames in string and uses an EmbeddedFileChooser
 * as its visual representation.
 */
class FileField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  FileField(NodeIO* parent);

  /**
   * @brief Get the filename at the given timecode
   *
   * A convenience function, equivalent to GetValueAt(timecode).toString()
   *
   * @param timecode
   *
   * The timecode to retrieve the filename at
   *
   * @return
   *
   * The filename at this timecode
   */
  QString GetFileAt(double timecode);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a EmbeddedFileChooser.
   */
  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;

  /**
   * @brief Reimplementation of EffectField::UpdateWidgetValue()
   */
  virtual void UpdateWidgetValue(QWidget *widget, double timecode) override;
private slots:
  /**
   * @brief Internal function connected to any QWidget made from CreateWidget() to update the value based on user input
   *
   * @param b
   *
   * The current string of the QWidget (TextEditEx in this case). Automatically set when this slot
   * is connected to the TextEditEx::textModified() signal.
   */
  void UpdateFromWidget(const QString &s);
};

#endif // FILEFIELD_H
