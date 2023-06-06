/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef PREFERENCESEDITTAB_H
#define PREFERENCESEDITTAB_H


#include "dialog/configbase/configdialogbasetab.h"

class QCheckBox;
class QGroupBox;
class QRadioButton;
class QSpinBox;
class QLineEdit;
class QPushButton;


namespace olive {

class PreferencesEditTab : public ConfigDialogBaseTab
{
  Q_OBJECT
public:
  PreferencesEditTab();

  virtual bool Validate() override;

  virtual void Accept(MultiUndoCommand* command) override;

private slots:
  void onSelectDialogRequest();

private:
  QRadioButton * use_internal_editor_;
  QRadioButton * use_external_editor_;

  // for internal editor
  QSpinBox * font_size_;
  QSpinBox * indent_size_;
  QSpinBox * window_width_;
  QSpinBox * window_heigth_;

  // for external editor
  QLineEdit * ext_command_;
  QPushButton * ext_select_button;
  QLineEdit * ext_params_;
};

}

#endif // PREFERENCESEDITTAB_H
