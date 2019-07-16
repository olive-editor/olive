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

#include "nodeviewitem.h"

#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "core.h"
#include "ui/icons/icons.h"
#include "window/mainwindow/mainwindow.h"

const int kNodeViewItemBorderWidth = 2;
const int kNodeViewItemWidth = 250;
const int kNodeViewItemTextPadding = 4;
const int kNodeViewItemIconPadding = 12;

NodeViewItem::NodeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(nullptr),
  font_metrics(font),
  expanded_(false),
  standard_click_(false)
{
  // Set flags for this widget
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);

  // Use the current default font height to size this widget
  // Set default "collapsed" size
  title_bar_rect_ = QRectF(0, 0, kNodeViewItemWidth, font_metrics.height() + kNodeViewItemTextPadding * 2);
  setRect(title_bar_rect_);

  // Set default node connector size
  node_connector_size_ = font_metrics.height() / 3;
}

void NodeViewItem::SetNode(Node *n)
{
  node_ = n;

  update();
}

Node *NodeViewItem::node()
{
  return node_;
}

bool NodeViewItem::IsExpanded()
{
  return expanded_;
}

void NodeViewItem::SetExpanded(bool e)
{
  expanded_ = e;

  QRectF new_rect;

  if (expanded_) {
    QRectF full_size_rect = title_bar_rect_;

    // If a node is connected, use its parameter count to set the height
    if (node_ != nullptr) {
      full_size_rect.adjust(0, 0, 0, kNodeViewItemTextPadding*2 + font_metrics.height() * node_->ParameterCount());
    }

    // Store content_rect (the rect without the titlebar)
    content_rect_ = full_size_rect.adjusted(0, title_bar_rect_.height(), 0, 0);

    new_rect = full_size_rect;
  } else {
    new_rect = title_bar_rect_;
  }

  update();

  setRect(new_rect);
}

QRectF NodeViewItem::GetParameterConnectorRect(int index)
{
  if (node_ == nullptr) {
    return QRectF();
  }

  NodeParam* param = node_->ParamAt(index);

  QRectF connector_rect(rect().x(),
                        content_rect_.y() + kNodeViewItemTextPadding + font_metrics.height() / 2 - node_connector_size_ / 2,
                        node_connector_size_,
                        node_connector_size_);

  // FIXME: I don't know how this will work with NodeParam::kBidirectional
  if (param->type() == NodeParam::kOutput) {
    connector_rect.translate(rect().width() - node_connector_size_, 0);
  }

  return connector_rect;
}

QPointF NodeViewItem::GetParameterTextPoint(int index)
{
  if (node_ == nullptr) {
    return QPointF();
  }

  NodeParam* param = node_->ParamAt(index);

  // FIXME: I don't know how this will work with NodeParam::kBidirectional
  if (param->type() == NodeParam::kOutput) {
    return content_rect_.topRight() + QPointF(-(node_connector_size_ + kNodeViewItemTextPadding),
                                             kNodeViewItemTextPadding + font_metrics.ascent() + font_metrics.height()*index);
  } else {
    return content_rect_.topLeft() + QPointF(node_connector_size_ + kNodeViewItemTextPadding,
                                             kNodeViewItemTextPadding + font_metrics.ascent() + font_metrics.height()*index);
  }
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
  // HACK for getting the main QWidget palette color (the `widget`'s palette uses the NodeView color instead which we
  // don't want here)
  QPalette app_pal = olive::core.main_window()->palette();

  // Set up border, which will change color if selected
  // FIXME: Color not configurable?
  QPen border_pen(Qt::black, kNodeViewItemBorderWidth);

  QPen text_pen(app_pal.color(QPalette::Text));

  QBrush connector_brush(app_pal.color(QPalette::Text));

  painter->setPen(border_pen);

  if (expanded_ && node_ != nullptr) {


    painter->setBrush(app_pal.window());

    // Draw background rect
    painter->drawRect(rect());

    // Set pen to draw text
    painter->setPen(text_pen);

    // Draw text and a connector rectangle for each parameter

    // Store the text points which will steadily increase sa we loop

    // Loop through all the parameters
    QList<NodeParam*> node_params = node_->parameters();
    for (int i=0;i<node_params.size();i++) {
      NodeParam* param = node_params.at(i);

      // Draw connector square
      painter->fillRect(GetParameterConnectorRect(i), connector_brush);

      // Draw text
      QPointF text_pt = GetParameterTextPoint(i);

      // FIXME: I don't know how this will work for kBidirectional
      if (param->type() == NodeParam::kOutput) {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
        text_pt -= QPointF(font_metrics.width(param->name()), 0);
#else
        text_pt -= QPointF(font_metrics.horizontalAdvance(param->name()), 0);
#endif
      }

      painter->drawText(text_pt, param->name());
    }

    painter->setPen(border_pen);
  }

  // Draw rect
  painter->setBrush(obj_proxy_.TitleBarColor());
  painter->drawRect(title_bar_rect_);

  // If selected, draw selection outline
  if (option->state & QStyle::State_Selected) {
    QPen pen = painter->pen();
    pen.setColor(app_pal.color(QPalette::Highlight));
    painter->setPen(pen);

    painter->setBrush(Qt::transparent);

    painter->drawRect(rect());
  }

  // Draw text
  if (node_ != nullptr) {
    painter->setPen(text_pen);

    // Draw the expand icon
    expand_hitbox_ = title_bar_rect_.adjusted(kNodeViewItemIconPadding,
                                              kNodeViewItemIconPadding,
                                              -kNodeViewItemIconPadding,
                                              -kNodeViewItemIconPadding);

    // Make the icon rect a square
    expand_hitbox_.setWidth(expand_hitbox_.height());

    // Draw the icon
    if (IsExpanded()) {
      olive::icon::TriDown.paint(painter, expand_hitbox_.toRect(), Qt::AlignLeft | Qt::AlignVCenter);
    } else {
      olive::icon::TriRight.paint(painter, expand_hitbox_.toRect(), Qt::AlignLeft | Qt::AlignVCenter);
    }

    // Draw the text in a rect (the rect is sized around text already in the constructor)
    QRectF text_rect = title_bar_rect_.adjusted(kNodeViewItemIconPadding + expand_hitbox_.width() + kNodeViewItemTextPadding,
                                                kNodeViewItemTextPadding,
                                                -kNodeViewItemTextPadding,
                                                -kNodeViewItemTextPadding);
    painter->drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, node_->Name());
  }
}

void NodeViewItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  // We override standard mouse behavior in some cases. In these cases, we don't want the standard "move" and "release"
  // events to trigger if we haven't already triggered the "press" event. We use this variable to determine whether
  // base class behavior is valid here.
  standard_click_ = false;

  // Don't initiate a drag if we clicked the expand hitbox
  if (expand_hitbox_.contains(event->pos())) {
    return;
  }

  // See if the mouse click was on a parameter connector
  if (IsExpanded() // This is only possible if the node is expanded
      && node_ != nullptr) { // We can only loop through a node's parameters if a valid node is attached
    for (int i=0;i<node_->ParameterCount();i++) {
      if (GetParameterConnectorRect(i).contains(event->pos())) {
        return;
      }
    }
  }

  // We aren't using any override behaviors, switch back to standard click behavior
  standard_click_ = true;
  QGraphicsRectItem::mousePressEvent(event);
}

void NodeViewItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (standard_click_) {
    QGraphicsRectItem::mouseMoveEvent(event);
  }
}

void NodeViewItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  // Check if we clicked the Expand/Collapse icon
  if (expand_hitbox_.contains(event->pos())) {
    SetExpanded(!IsExpanded());
  }

  if (standard_click_) {
    QGraphicsRectItem::mouseReleaseEvent(event);
  }
}

NodeViewItemWidgetProxy::NodeViewItemWidgetProxy()
{
}

QColor NodeViewItemWidgetProxy::TitleBarColor()
{
  return title_bar_color_;
}

void NodeViewItemWidgetProxy::SetTitleBarColor(QColor color)
{
  title_bar_color_ = color;
}
