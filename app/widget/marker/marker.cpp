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

#include <QInputDialog>
#include <QPainter>

#include "common/qtutils.h"
#include "config/config.h"
#include "panel/panelmanager.h"
#include "panel/timeline/timeline.h"
#include "ui/colorcoding.h"
#include "widget/menu/menu.h"
#include "widget/menu/menushared.h"
#include "widget/timeruler/seekablewidget.h"
#include "widget/timelinewidget/timelinewidget.h"

namespace olive {

Marker::Marker(QWidget *parent) :
	QWidget(parent),
    active_(false),
    drag_allowed_(false),
    dragging_(false)
{
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QFontMetrics fm = fontMetrics();
  marker_height_ = fm.height();
  marker_width_ = QtUtils::QFontMetricsWidth(fm, "H");
  resize(marker_width_, marker_height_);


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

  //int text_height = fm.height();
  //int marker_width_ = QtUtils::QFontMetricsWidth(fm, "H");

  int y = marker_height_;

  int half_width = marker_width_ / 2;

  int x = half_width;

  QPainter p(this);
  if (active_) {
    p.setPen(Qt::white);
  } else {
    p.setPen(Qt::black);
  }
  p.setBrush(ColorCoding::GetColor(marker_color_).toQColor());

  p.setRenderHint(QPainter::Antialiasing);

  int half_marker_height = marker_height_ / 3;

  QPoint points[] = {
      QPoint(x, y),
      QPoint(x - half_width, y - half_marker_height),
      QPoint(x - half_width, y - marker_height_),
      QPoint(x + 1 + half_width, y - marker_height_),
      QPoint(x + 1 + half_width, y - half_marker_height),
      QPoint(x + 1, y),
  };

  p.drawPolygon(points, 6);

  if (!name_.isEmpty()) {
    resize(marker_width_ + fm.horizontalAdvance(name_) + fm.horizontalAdvance(" "), marker_height_);
    p.drawText(x + marker_width_, y - half_marker_height, name_);
  } else {
    resize(marker_width_, marker_height_);
  }

  TimelinePanel* timeline = PanelManager::instance()->MostRecentlyFocused<TimelinePanel>();
  if (timeline) {
    timeline->timeline_widget()->Refresh();
  }
}

void Marker::mousePressEvent(QMouseEvent* e)
{
  // Only select if clicking on the icon and not the label
  if (e->pos().x() >  marker_width_) {
    return;
  }
  if (e->button() == Qt::LeftButton || e->button() == Qt::RightButton) {
    static_cast<SeekableWidget*>(parent())->SeekToScreenPoint(this->x() + 4);
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

  click_position_ = e->globalPos();
  marker_start_x_ = x();
  drag_allowed_ = true;
}

void Marker::mouseMoveEvent(QMouseEvent* e)
{
  if (drag_allowed_) {
    dragging_ = true;

    int new_pos = marker_start_x_ + e->globalPos().x() - click_position_.x();

    if (new_pos > -marker_width_ / 2 && new_pos < static_cast<SeekableWidget *>(parent())->width() - marker_width_ / 2) {
      this->move(new_pos-2, this->pos().y());
      repaint();
    }
  }
}

void Marker::mouseReleaseEvent(QMouseEvent* e)
{
  if (dragging_) {
    rational time = static_cast<SeekableWidget *>(parent())->SceneToTime(this->x()+4);
    time = time < 0.0 ? 0.0 : time;
    emit TimeChanged(TimeRange(time, time));

    dragging_ = false;
  }

  drag_allowed_ = false;
}

void Marker::ShowContextMenu()
{
  // Only show context menu if we clicked on the icon and not the label
  QRect marker_icon(0, 0, marker_width_, marker_height_);
  if (!marker_icon.contains(this->mapFromGlobal(QCursor::pos()))) {
    return;
  }

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
  bool ok;
  QString marker_name = QInputDialog::getText(this, tr("Set Marker"), tr("Marker name:"), QLineEdit::Normal, name_, &ok);
  if (ok) {
    emit NameChanged(marker_name);
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

  update();
}

void Marker::SetTime(TimeRange time)
{
  move(static_cast<SeekableWidget *>(parent())->TimeToScene(time.in())-2, y());
}

}  // namespace olive
