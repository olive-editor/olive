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

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

#include "common/define.h"

namespace olive {

class ClickableLabel : public QLabel
{
  Q_OBJECT
public:
  ClickableLabel(const QString& text, QWidget* parent = nullptr);
  ClickableLabel(QWidget* parent = nullptr);

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

signals:
  void MouseClicked();
  void MouseDoubleClicked();

};

}

#endif // CLICKABLELABEL_H
