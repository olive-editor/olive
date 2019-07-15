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

#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "ui/icons/icons.h"

const int kNodeViewItemBorderWidth = 2;
const int kNodeViewItemWidth = 250;
const int kNodeViewItemTextPadding = 4;
const int kNodeViewItemIconPadding = 12;

NodeViewItem::NodeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(nullptr),
  expanded_(false)
{
  // Set flags for this widget
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);

  // Use the current default font height to size this widget
  QFont f;
  QFontMetrics fm(f);

  // Set default "collapsed" size
  title_bar_rect_ = QRectF(0, 0, kNodeViewItemWidth, fm.height() + kNodeViewItemTextPadding * 2);
  setRect(title_bar_rect_);

  // Set default node connector size
  node_connector_size_ = fm.height() / 3;

  // FIXME: Magic "number"/magic "color" - allow this to be editable by the user
  SetColor(QColor(32, 32, 128));
}

void NodeViewItem::SetColor(const QColor &color)
{
  color_ = color;

  // Create a light gradient based on this color
  UpdateGradient();

  update();
}

void NodeViewItem::SetNode(Node *n)
{
  node_ = n;

  param_rect_.resize(node_->ParameterCount());

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
      QFont f;
      QFontMetrics fm(f);

      full_size_rect.adjust(0, 0, 0, kNodeViewItemTextPadding*2 + fm.height() * node_->ParameterCount());
    }

    // Store content_rect (the rect without the titlebar)
    content_rect_ = full_size_rect.adjusted(0, title_bar_rect_.height(), 0, 0);

    new_rect = full_size_rect;
  } else {
    new_rect = title_bar_rect_;
  }

  //update();

  setRect(new_rect);
}

const QRectF &NodeViewItem::GetParameterConnectorRect(int index)
{
  return param_rect_.at(index);
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Set up border, which will change color if selected
  // FIXME: Color not configurable?
  QPen border_pen(Qt::black, kNodeViewItemBorderWidth);

  // FIXME: The text is always drawn white assuming the color will be dark - the intention is to provide preset
  //        colors that will always be dark for the user to choose, so this value can stay white.
  QPen text_pen(Qt::white);

  // FIXME: Same as text_pen
  QBrush connector_brush(Qt::white);

  // FIXME: Same as above
  QBrush content_brush(QColor("#181818"));

  painter->setPen(border_pen);

  if (expanded_ && node_ != nullptr) {
    // Draw background rect
    painter->setBrush(content_brush);
    painter->drawRect(rect());

    QRectF text_rect = content_rect_.adjusted(kNodeViewItemTextPadding + node_connector_size_,
                                              kNodeViewItemTextPadding,
                                              -kNodeViewItemTextPadding,
                                              -(kNodeViewItemTextPadding + node_connector_size_));

    // Set pen to draw text
    painter->setPen(text_pen);

    // Draw text and a connector rectangle for each parameter

    // Store the rectangles/points which will steadily increase sa we loop
    QRectF input_connector_rect(rect().x(),
                                text_rect.y() + painter->fontMetrics().height() / 2 - node_connector_size_ / 2,
                                node_connector_size_,
                                node_connector_size_);
    QRectF output_connector_rect = input_connector_rect.translated(rect().width() - node_connector_size_, 0);
    QPointF text_draw_point = text_rect.topLeft() + QPointF(0, painter->fontMetrics().ascent());

    // Loop through all the parameters
    QList<NodeParam*> node_params = node_->parameters();
    for (int i=0;i<node_params.size();i++) {
      NodeParam* param = node_params.at(i);

      // Draw connector square
      if (param->type() == NodeParam::kInput || param->type() == NodeParam::kBidirectional) {
        painter->fillRect(input_connector_rect, connector_brush);

        // FIXME: I don't know how this will work with NodeParam::kBidirectional
        param_rect_[i] = input_connector_rect;
      }
      if (param->type() == NodeParam::kOutput || param->type() == NodeParam::kBidirectional) {
        painter->fillRect(output_connector_rect, connector_brush);

        // FIXME: I don't know how this will work with NodeParam::kBidirectional
        param_rect_[i] = output_connector_rect;
      }

      input_connector_rect.translate(0, painter->fontMetrics().height());
      output_connector_rect.translate(0, painter->fontMetrics().height());

      // Draw text
      painter->drawText(text_draw_point, node_params.at(i)->name());
      text_draw_point += QPointF(0, painter->fontMetrics().height());
    }

    painter->setPen(border_pen);
  }

  // Draw rect
  painter->setBrush(brush());
  painter->drawRect(title_bar_rect_);

  // If selected, draw selection outline
  if (option->state & QStyle::State_Selected) {
    QPen pen = painter->pen();
    pen.setColor(widget->palette().highlight().color());
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
    olive::icon::TriRight.paint(painter, expand_hitbox_.toRect(), Qt::AlignLeft | Qt::AlignVCenter);

    // Draw the text in a rect (the rect is sized around text already in the constructor)
    QRectF text_rect = title_bar_rect_.adjusted(kNodeViewItemIconPadding + expand_hitbox_.width() + kNodeViewItemTextPadding,
                                              kNodeViewItemTextPadding,
                                              -kNodeViewItemTextPadding,
                                              -kNodeViewItemTextPadding);
    painter->drawText(text_rect, Qt::AlignVCenter | Qt::AlignLeft, node_->Name());
  }
}

void NodeViewItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  // Check if we clicked the Expand/Collapse icon
  if (expand_hitbox_.contains(event->pos())) {
    SetExpanded(!IsExpanded());
  }

  QGraphicsRectItem::mouseReleaseEvent(event);
}

void NodeViewItem::UpdateGradient()
{
  /*QLinearGradient grad(QPointF(0, rect().top()), QPointF(0, rect().bottom()));
  grad.setColorAt(0, color_.lighter(175));
  grad.setColorAt(1, color_);
  setBrush(grad);*/
  setBrush(color_);
}
