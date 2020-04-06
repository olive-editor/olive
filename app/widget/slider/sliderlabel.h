/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#ifndef SLIDERLABEL_H
#define SLIDERLABEL_H

#include <QLabel>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class SliderLabel : public QLabel
{
  Q_OBJECT
public:
  SliderLabel(QWidget* parent);

protected:
  virtual void mousePressEvent(QMouseEvent *ev) override;

  virtual void mouseMoveEvent(QMouseEvent *ev) override;

  virtual void mouseReleaseEvent(QMouseEvent *ev) override;

  virtual void focusInEvent(QFocusEvent *event) override;

signals:
  void dragged(int x);

  void drag_start();

  void drag_stop();

  void focused();

private:
  QPoint drag_start_;

};

OLIVE_NAMESPACE_EXIT

#endif // SLIDERLABEL_H
