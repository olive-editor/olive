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

#ifndef MARKER_H
#define MARKER_H

#include <QWidget>
#include <QPushButton>

#include "common/define.h"
#include "widget/colorlabelmenu/colorlabelmenu.h"

namespace olive {

class Marker : public QWidget {
  Q_OBJECT
 public:
  Marker(QWidget* parent = nullptr);

 protected:
  void paintEvent(QPaintEvent* event) override;
  //virtual void mouseReleaseEvent(QMouseEvent* event) override;
  //virtual void mouseDoubleClickEvent(QMouseEvent* event) override;

 signals:
  //void MouseClicked();
  //void MouseDoubleClicked();

 private:

  int marker_color_;

 private slots:
  void ShowContextMenu();
  void SetColor(int c);
};

}  // namespace olive

#endif  // MARKER_H
