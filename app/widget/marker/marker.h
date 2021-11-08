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
#include <QMouseEvent>
#include <QPushButton>

#include "common/define.h"
#include "common/timerange.h"
#include "widget/colorlabelmenu/colorlabelmenu.h"

namespace olive {

class SeekableWidget;

class Marker : public QWidget {
  Q_OBJECT
 public:
  Marker(QWidget* parent = nullptr);

  bool active();

  void Rename();

  SeekableWidget* SeekableParent();

 public slots:
  void SetColor(int c);
  void SetActive(bool active);
  void SetName(QString s);
  void SetTime(TimeRange t);

 protected:
  void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

 signals:
  //void MouseClicked();
  //void MouseDoubleClicked();
  void ColorChanged(int c);
  void markerSelected(Marker* marker);
  void ActiveChanged(bool active);
  void NameChanged(QString name);
  void TimeChanged(TimeRange time);

 private:

  int marker_color_;

  bool active_;

  QString name_;

  QPointF click_position_;

  int marker_start_x_;

  bool drag_allowed_;

  bool dragging_;

  int marker_height_;

  int marker_width_;

 private slots:
  void ShowContextMenu();
};

}  // namespace olive

#endif  // MARKER_H
