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

#ifndef RESIZABLESCROLLBAR_H
#define RESIZABLESCROLLBAR_H

#include <QScrollBar>

class ResizableScrollBar : public QScrollBar
{
    Q_OBJECT
public:
    ResizableScrollBar(QWidget * parent = 0);
    bool is_resizing();
signals:
    void resize_move(double i);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
private:
    bool resize_init;
    bool resize_proc;
    int resize_start;
    bool resize_top;

    int resize_start_max;
    int resize_start_width;
};

#endif // RESIZABLESCROLLBAR_H
