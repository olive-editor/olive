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

#ifndef LABELFIELD_H
#define LABELFIELD_H

#include "../effectfield.h"

/**
 * @brief The LabelField class
 *
 * A UI-type EffectField. This field is largely an EffectField wrapper around a QLabel and provides no data that's
 * usable in the Effect. It's primarily useful for showing UI information. This field is not exposed to the external
 * shader API as it requires raw C++ code to connect it to other elements.
 */
class LabelField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief Reimplementation of EffectField::EffectField().
   */
  LabelField(NodeIO* parent, const QString& string);

  /**
   * @brief Reimplementation of EffectField::CreateWidget()
   *
   * Creates and connects to a QLabel.
   */
  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;
private:
  /**
   * @brief Internal text string
   */
  QString label_text_;
};

#endif // LABELFIELD_H
