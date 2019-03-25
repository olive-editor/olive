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

#ifndef BUTTONFIELD_H
#define BUTTONFIELD_H

#include "../effectfield.h"

/**
 * @brief The ButtonField class
 *
 * A UI-type EffectField. This field is largely an EffectField wrapper around a QPushButton and provides no data that's
 * usable in the Effect. It's primarily useful for other UI functions (e.g. showing/hiding a dialog or other UI
 * elements). This field is not exposed to the external shader API as it requires raw C++ code to connect it to other
 * elements.
 *
 * As with all widgets created from EffectField::CreateWidget(), you should never interface with the resulting widget
 * directly (apart from adding it to a layout and deleting it when it's unnecessary). All signals/slots should pass
 * through ButtonField instead to keep consistency with every layer involved.
 */
class ButtonField : public EffectField
{
  Q_OBJECT
public:
  /**
   * @brief See Effect::Effect().
   */
  ButtonField(EffectRow* parent, const QString& string);

  /**
   * @brief Set whether this pushbutton is checkable
   *
   * This function is mainly a wrapper around QPushButton::setCheckable().
   *
   * "Checkable" means the button can be toggled between a state of being "normal" and being "pressed". In checkable
   * mode this field still cannot be used as a value in an Effect. Instead use BoolField (which uses a QCheckBox
   * representation) for passing values to the Effect that can only be true or false.
   *
   * @param c
   *
   * TRUE if this button should be checkable or not.
   */
  void SetCheckable(bool c);

  /**
   * @brief See EffectField::CreateWidget()
   */
  virtual QWidget* CreateWidget(QWidget *existing = nullptr) override;

public slots:
  /**
   * @brief A slot for when a widget's (created and connected from CreateWidget() ) checked state is changed
   *
   * @param c
   *
   * The current checked state (automatically filled by the QPushButton::toggled() signal)
   */
  void SetChecked(bool c);

signals:
  /**
   * @brief A signal emitted whenever the field's internal checked state is changed
   *
   * Primarily used to set any connected widget's checked state to be consistent with the field's.
   */
  void CheckedChanged(bool);

  /**
   * @brief A signal emitted whenever the checked state of a connected widget changes
   *
   * Any widgets associated with this field will emit this signal when their checked state changes.
   */
  void Toggled(bool);

private:
  /**
   * @brief Internal button text string passed to widgets created by CreateWidget()
   */
  bool checkable_;

  /**
   * @brief Internal checked value passed to and from widgets created by CreateWidget()
   */
  bool checked_;

  /**
   * @brief Internal button text string passed to widgets created by CreateWidget()
   */
  QString button_text_;
};

#endif // BUTTONFIELD_H
