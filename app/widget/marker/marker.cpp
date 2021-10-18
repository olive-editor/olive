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

#include "marker.h"

#include <QPainter>

#include "common/qtutils.h"
#include "config/config.h"
#include "dialog/text/text.h"
#include "ui/colorcoding.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "widget/timeruler/seekablewidget.h"
#include "widget/timelinewidget/timelinewidget.h"

namespace olive {

Marker::Marker(QWidget *parent) :
	QWidget(parent),
    active_(false)
{
  //setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setMinimumSize(8, 20);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &Marker::customContextMenuRequested, this, &Marker::ShowContextMenu);
}

void Marker::SetActive(bool active)
{
  active_ = active;

  // Feels very hacky, might it be better to write some access methods?
  if (active) {
    TimelineWidget *timeline = dynamic_cast<TimelineWidget *>(parent()->parent());
    if (timeline) {
      static_cast<TimelineWidget *>(parent()->parent())->DeselectAll();
    }
  }

  update();
}

bool Marker::active()
{
  return active_;
}

void Marker::paintEvent(QPaintEvent *event)
{
  QFontMetrics fm = fontMetrics();

  int text_height = fm.height();
  int marker_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  int y = text_height; //13

  int half_width = marker_width_ / 2;

  int x = half_width; //3

  QPainter p(this);
  if (active_) {
    p.setPen(Qt::white);
  } else {
    p.setPen(Qt::black);
  }
  p.setBrush(ColorCoding::GetColor(marker_color_).toQColor());

  p.setRenderHint(QPainter::Antialiasing);

  int half_text_height = text_height / 3;

  QPoint points[] = {
      QPoint(x, y),
      QPoint(x - half_width, y - half_text_height),
      QPoint(x - half_width, y - text_height),
      QPoint(x + 1 + half_width, y - text_height),
      QPoint(x + 1 + half_width, y - half_text_height),
      QPoint(x + 1, y),
  };

  p.drawPolygon(points, 6);

  if (!name_.isEmpty()) {
    p.drawText(x + marker_width_, y - half_text_height, name_);
  }
}

void Marker::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::LeftButton) {
    static_cast<SeekableWidget*>(parent())->SeekToScreenPoint(e->pos().x() + this->x());
    if (!active_) {
      if (e->modifiers() != Qt::ShiftModifier) {
        static_cast<SeekableWidget *>(parent())->DeselectAllMarkers();
      }
      emit ActiveChanged(true);
    } else {
      if (e->modifiers() == Qt::ShiftModifier) {
        emit ActiveChanged(false);
      }
    }

    update();
  }
}

void Marker::ShowContextMenu() {
  Menu m(this);

  // Color menu
  ColorLabelMenu color_coding_menu;
  connect(&color_coding_menu, &ColorLabelMenu::ColorSelected, this, &Marker::ColorChanged);
  m.addMenu(&color_coding_menu);

  m.addSeparator();
  MenuShared::instance()->AddItemsForEditMenu(&m, false);

  m.addSeparator();
  QAction rename;
  rename.setText(tr("Rename"));
  m.addAction(&rename);
  connect(&rename, &QAction::triggered, this, &Marker::Rename);

  m.exec(QCursor::pos());
}

void Marker::Rename()
{
  TextDialog d(this->name_, this);
  if (d.exec() == QDialog::Accepted) {
    QString s = d.text();
    emit NameChanged(s);
  }
}

void Marker::SetColor(int c)
{
  marker_color_ = c;

  update();
}

void Marker::SetName(QString s)
{
  name_ = s;
  qDebug() << name_;

  update();
}

}  // namespace olive
