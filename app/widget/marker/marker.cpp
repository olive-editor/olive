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
#include "ui/colorcoding.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"

namespace olive {

Marker::Marker(QWidget *parent) :
	QWidget(parent),
    marker_color_(7) //green FIXME: add default color to config
{
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setMinimumSize(20, 20);
  setContextMenuPolicy(Qt::CustomContextMenu);

  connect(this, &Marker::customContextMenuRequested, this, &Marker::ShowContextMenu);
}

void Marker::paintEvent(QPaintEvent *event)
{
  QFontMetrics fm = fontMetrics();

  int text_height = fm.height();
  int marker_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  int y = text_height; //13

  int half_width = marker_width_ / 2;

  int x = half_width; //3

  if (x + half_width < 0 || x - half_width > width()) {
    return;
  }

  QPainter p(this);
  p.setPen(Qt::black);
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

  p.setRenderHint(QPainter::Antialiasing, false);
}

void Marker::ShowContextMenu() {
  Menu m(this);

  // Color menu
  ColorLabelMenu color_coding_menu;
  connect(&color_coding_menu, &ColorLabelMenu::ColorSelected, this, &Marker::SetColor);
  m.addMenu(&color_coding_menu);

  m.exec(QCursor::pos());
}

void Marker::SetColor(int c)
{
  marker_color_ = c;

  update();
}

}  // namespace olive