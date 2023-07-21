/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef DRAGBUTTON_H
#define DRAGBUTTON_H

#include <QPushButton>

#include "common/define.h"

namespace olive {

class DragButton : public QPushButton
{
  Q_OBJECT
public:
  DragButton(QWidget* parent = nullptr);

signals:
  void DragStarted();

protected:
  virtual void mousePressEvent(QMouseEvent* event) override;

  virtual void mouseMoveEvent(QMouseEvent* event) override;

  virtual void mouseReleaseEvent(QMouseEvent* event) override;

private:
  bool dragging_;

};

}

#endif // DRAGBUTTON_H
