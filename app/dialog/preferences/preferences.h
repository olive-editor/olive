/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

/**
 * @brief The PreferencesDialog class
 *
 * A dialog for the global application settings. Mostly an interface for Config. Can be loaded from any part of the
 * application.
 */
class PreferencesDialog : public ConfigDialogBase
{
  Q_OBJECT

public:
  /**
   * @brief PreferencesDialog Constructor
   *
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   */
  PreferencesDialog(QWidget *parent, QMenuBar* main_menu_bar);

};

}

#endif // PREFERENCESDIALOG_H
