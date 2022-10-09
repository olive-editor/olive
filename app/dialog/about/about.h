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

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QCheckBox>
#include <QDialog>

#include "common/define.h"

namespace olive {

/**
 * @brief The AboutDialog class
 *
 * The About dialog (accessible through Help > About). Contains license and version information. This can be run from
 * anywhere
 */
class AboutDialog : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief AboutDialog Constructor
   *
   * Creates About dialog.
   *
   * @param parent
   *
   * QWidget parent object. Usually this will be MainWindow.
   */
  explicit AboutDialog(bool welcome_dialog, QWidget *parent = nullptr);

public slots:
  virtual void accept() override;

private:
  QCheckBox *dont_show_again_checkbox_;

};

}

#endif // ABOUTDIALOG_H
