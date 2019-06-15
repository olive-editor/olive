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

#ifndef CHECKBOXEX_H
#define CHECKBOXEX_H

#include <QCheckBox>

class CheckboxEx : public QCheckBox
{
  Q_OBJECT
public:
  CheckboxEx(QWidget* parent = 0);
private slots:
  void checkbox_command();
};

#endif // CHECKBOXEX_H
