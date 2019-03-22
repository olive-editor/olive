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

#ifndef DEMONOTICE_H
#define DEMONOTICE_H

#include <QDialog>

/**
 * @brief The DemoNotice class
 *
 * Simple dialog shown on startup to introduce Olive as alpha software (in release builds). Can be run from anywhere,
 * but there should be no reason to create it outside of the application launch.
 *
 * To be phased out as Olive gains maturity.
 */
class DemoNotice : public QDialog
{
  Q_OBJECT
public:
  /**
   * @brief DemoNotice Constructor
   * @param parent
   *
   * QWidget parent. Usually MainWindow.
   */
  explicit DemoNotice(QWidget *parent = nullptr);
};

#endif // DEMONOTICE_H
