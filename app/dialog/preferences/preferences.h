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

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QCheckBox>
#include <QDialog>
#include <QListWidget>
#include <QMenuBar>
#include <QStackedWidget>
#include <QTabWidget>

#include "dialog/configbase/configdialogbase.h"

namespace olive {

class MainWindow;

/**
 * @brief The PreferencesDialog class
 *
 * A dialog for the global application settings. Mostly an interface for Config.
 */
class PreferencesDialog : public ConfigDialogBase
{
  Q_OBJECT

public:
  PreferencesDialog(MainWindow *main_window);

protected:
  virtual void AcceptEvent() override;

};

}

#endif // PREFERENCESDIALOG_H
