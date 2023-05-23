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

#include "valueswizzlewidget.h"

#include <QDebug>
#include <QMouseEvent>
#include <QScrollBar>

#include "config/config.h"

namespace olive {

#define super QGraphicsView

ValueSwizzleWidget::ValueSwizzleWidget(QWidget *parent) :
  super{parent}
{
  setFrameShape(QFrame::NoFrame);
  setFrameShadow(QFrame::Plain);
  setRenderHint(QPainter::Antialiasing);
  setBackgroundRole(QPalette::Base);
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
  setDragMode(RubberBandDrag);

  scene_ = new QGraphicsScene(this);
  setScene(scene_);

  new_item_ = nullptr;
  //labels_ = kRGBALabels;
  labels_ = kXYZWLabels;

  channel_height_ = this->fontMetrics().height() * 3 / 2;
  channel_width_ = channel_height_ * 2;
  from_count_ = 0;
  to_count_ = 0;

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

bool ValueSwizzleWidget::delete_selected()
{
  auto sel = scene_->selectedItems();
  if (sel.isEmpty()) {
    return false;
  }

  auto map = get_map_from_connectors();

  for (auto s : sel) {
    auto conn = static_cast<SwizzleConnectorItem*>(s);
    map.remove(conn->to());
  }

  set(map);
  emit value_changed(map);

  return true;
}

void ValueSwizzleWidget::set(const SwizzleMap &map)
{
  clear_all();

  cached_map_ = map;

  if (cached_map_.empty()) {
    // Connect everything directly for empty maps
    size_t lim = std::min(from_count_, to_count_);
    for (size_t i = 0; i < lim; i++) {
      make_item(i, i);
    }
  } else {
    for (auto it = cached_map_.cbegin(); it != cached_map_.cend(); it++) {
      make_item(it->second, it->first);
    }
  }

  adjust_all();
}

void ValueSwizzleWidget::set_channels(size_t from, size_t to)
{
  from_count_ = from;
  to_count_ = to;
  setFixedHeight(std::max(from, to) * channel_height_);

  set(cached_map_);
}

void ValueSwizzleWidget::drawBackground(QPainter *p, const QRectF &r)
{
  for (size_t i = 0; i < from_count_; i++) {
    draw_channel(p, i, 0);
  }

  for (size_t i = 0; i < to_count_; i++) {
    draw_channel(p, i, width() - channel_width_ - 1);
  }
}

void ValueSwizzleWidget::mousePressEvent(QMouseEvent *e)
{
  if (is_inside_bounds(e->x())) {
    drag_from_ = channel_is_from(e->x());
    drag_index_ = get_channel_index_from_y(e->y());
    drag_start_ = get_connect_point_of_channel(drag_from_, drag_index_);
    new_item_connected_ = false;

    if (!drag_from_) {
      // "to"s can only have one connection, so if there's already a connection here, we'll remove it
      for (auto it = connectors_.cbegin(); it != connectors_.cend(); it++) {
        SwizzleConnectorItem *s = *it;
        if (s->to() == drag_index_) {
          // Grab this item
          new_item_ = s;

          // Flip so we're dragging the connector from its old position
          drag_from_ = true;
          drag_index_ = new_item_->from();
          drag_start_ = get_connect_point_of_channel(true, drag_index_);
          new_item_connected_ = true;

          // Remove from list because we'll delete this later
          connectors_.erase(it);
          break;
        }
      }
    }

    if (!new_item_) {
      new_item_ = new SwizzleConnectorItem();
      if (drag_from_) {
        new_item_->set_from(drag_index_);
      } else {
        new_item_->set_to(drag_index_);
      }
      scene_->addItem(new_item_);
    }
  } else {
    super::mousePressEvent(e);
  }
}

void ValueSwizzleWidget::mouseMoveEvent(QMouseEvent *e)
{
  if (new_item_) {
    QPoint end_point;

    new_item_connected_ = false;

    if (is_inside_bounds(e->x())) {
      bool is_from = channel_is_from(e->x());
      if (drag_from_ != is_from) {
        size_t index = get_channel_index_from_y(e->y());
        if ((is_from && index < from_count_) || (!is_from && index < to_count_)) {
          end_point = get_connect_point_of_channel(is_from, index);
          new_item_connected_ = true;
          if (drag_from_) {
            new_item_->set_to(index);
          } else {
            new_item_->set_from(index);
          }
        }
      }
    }

    if (!new_item_connected_) {
      end_point = e->pos();
    }
    new_item_->SetConnected(new_item_connected_);

    new_item_->SetPoints(drag_start_, end_point);
  } else {
    super::mouseMoveEvent(e);
  }
}

void ValueSwizzleWidget::mouseReleaseEvent(QMouseEvent *e)
{
  if (new_item_) {
    SwizzleMap map = get_map_from_connectors();

    if (new_item_connected_) {
      map.insert(new_item_->to(), new_item_->from());
    }

    delete new_item_;
    new_item_ = nullptr;

    set(map);
    emit value_changed(map);
  } else {
    super::mouseReleaseEvent(e);
  }
}

void ValueSwizzleWidget::resizeEvent(QResizeEvent *e)
{
  this->setSceneRect(this->viewport()->rect());

  adjust_all();

  super::resizeEvent(e);
}

void ValueSwizzleWidget::draw_channel(QPainter *p, size_t i, int x)
{
  static const size_t kChannelColorCount = 4;
  static const QColor kDefaultColor = QColor(32, 32, 32);
  const QColor kChannelColors[kChannelColorCount] = {
    QColor(160, 32, 32),
    QColor(32, 160, 32),
    QColor(32, 32, 160),
    QColor(64, 64, 64)
  };

  QRect r(x, i * channel_height_, channel_width_, channel_height_);

  const QColor &main_col = i < kChannelColorCount ? kChannelColors[i] : kDefaultColor;
  if (OLIVE_CONFIG("UseGradients").toBool()) {
    QLinearGradient lg(r.topLeft(), r.bottomLeft());

    lg.setColorAt(0.0, main_col.lighter());
    lg.setColorAt(1.0, main_col);

    p->setBrush(lg);
  } else {
    p->setBrush(main_col);
  }

  p->setPen(Qt::black);
  p->drawRect(r);

  p->setPen(Qt::white);
  p->drawText(r, Qt::AlignCenter, get_label_text(i));
}

QPoint ValueSwizzleWidget::get_connect_point_of_channel(bool from, size_t index)
{
  QPoint p;

  if (from) {
    p.setX(channel_width_);
  } else {
    p.setX(get_right_channel_bound());
  }
  p.setY(index * channel_height_ + channel_height_ / 2);

  return p;
}

void ValueSwizzleWidget::adjust_all()
{
  for (auto it = connectors_.cbegin(); it != connectors_.cend(); it++) {
    SwizzleConnectorItem *s = *it;
    s->SetPoints(get_connect_point_of_channel(true, s->from()), get_connect_point_of_channel(false, s->to()));
  }
}

void ValueSwizzleWidget::make_item(size_t from, size_t to)
{
  SwizzleConnectorItem *s = new SwizzleConnectorItem();
  s->set_from(from);
  s->set_to(to);
  s->SetConnected(true);
  scene_->addItem(s);
  connectors_.push_back(s);
}

void ValueSwizzleWidget::clear_all()
{
  qDeleteAll(connectors_);
  connectors_.clear();
}

SwizzleMap ValueSwizzleWidget::get_map_from_connectors() const
{
  SwizzleMap map;
  for (auto it = connectors_.cbegin(); it != connectors_.cend(); it++) {
    SwizzleConnectorItem *s = *it;
    map.insert(s->to(), s->from());
  }
  return map;
}

QString ValueSwizzleWidget::get_label_text(size_t index) const
{
  switch (labels_) {
  case kNumberLabels:
    break;
  case kXYZWLabels:
    switch (index) {
    case 0: return tr("X");
    case 1: return tr("Y");
    case 2: return tr("Z");
    case 3: return tr("W");
    }
    break;
  case kRGBALabels:
    switch (index) {
    case 0: return tr("R");
    case 1: return tr("G");
    case 2: return tr("B");
    case 3: return tr("A");
    }
    break;
  }

  return QString::number(index);
}

SwizzleConnectorItem::SwizzleConnectorItem(QGraphicsItem *parent) :
  CurvedConnectorItem(parent)
{
}

}
