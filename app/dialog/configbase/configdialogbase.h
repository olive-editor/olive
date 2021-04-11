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

#ifndef CONFIGBASE_H
#define CONFIGBASE_H

#include <QDialog>
#include <QListWidget>
#include <QStackedWidget>

#include "configdialogbasetab.h"

namespace olive {

class ConfigDialogBase : public QDialog
{
  Q_OBJECT
public:
  ConfigDialogBase(QWidget* parent = nullptr);

private slots:
  /**
   * @brief Override of accept to save preferences to Config.
   */
  virtual void accept() override;

protected:
  void AddTab(ConfigDialogBaseTab* tab, const QString& title);

private:
  QListWidget* list_widget_;

  QStackedWidget* preference_pane_stack_;

  QList<ConfigDialogBaseTab*> tabs_;

};

}

#endif // CONFIGBASE_H
