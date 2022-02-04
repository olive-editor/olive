/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef CODECSTACK_H
#define CODECSTACK_H

#include <QStackedWidget>

namespace olive {

class CodecStack : public QStackedWidget
{
  Q_OBJECT
public:
  explicit CodecStack(QWidget *parent = nullptr);

  void addWidget(QWidget *widget);

signals:

private slots:
  void OnChange(int index);

};

}

#endif // CODECSTACK_H
