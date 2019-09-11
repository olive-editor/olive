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
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "common/qtversionabstraction.h"
#include "core.h"
#include "nodeview.h"
#include "nodeviewundo.h"
#include "ui/icons/icons.h"
#include "undo/undostack.h"
#include "window/mainwindow/mainwindow.h"

NodeViewItem::NodeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(nullptr),
  font_metrics(font),
  dragging_edge_(nullptr),
  drag_expanded_item_(nullptr),
  expanded_(false),
  standard_click_(false),
  node_edge_change_command_(nullptr)
{
  // Set flags for this widget
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);

  //
  // We use font metrics to set all the UI measurements for DPI-awareness
  //

  // Not particularly great way of using text scaling to set the width (DPI-awareness, etc.)
  int widget_width = QFontMetricsWidth(&font_metrics, "HHHHHHHHHHHHHHHH");

  // Set border width
  node_border_width_ = font_metrics.height() / 12;

  // Set default node connector size
  node_connector_size_ = font_metrics.height() / 3;

  // Set text and icon padding
  node_text_padding_ = font_metrics.height() / 6;

  // FIXME: Revise icon sizing algorithm (share with NodeParamViewItem)
  node_icon_padding_ = node_text_padding_ * 3;

  // Use the current default font height to size this widget
  // Set default "collapsed" size
  title_bar_rect_ = QRectF(0, 0, widget_width, font_metrics.height() + node_text_padding_ * 2);
  setRect(title_bar_rect_);
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
  if (expanded_ == e) {
    return;
  }

  expanded_ = e;

  QRectF new_rect;

  if (expanded_) {
    QRectF full_size_rect = title_bar_rect_;

    // If a node is connected, use its parameter count to set the height
    if (node_ != nullptr) {
      full_size_rect.adjust(0, 0, 0, node_text_padding_*2 + font_metrics.height() * node_->ParameterCount());
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
                        content_rect_.y() + node_text_padding_ + font_metrics.height() / 2 - node_connector_size_ / 2,
                        node_connector_size_,
                        node_connector_size_);

  if (index > 0) {
    connector_rect.translate(0, font_metrics.height() * index);
  }

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

  if (param->type() == NodeParam::kOutput) {
    return content_rect_.topRight() + QPointF(-(node_connector_size_ + node_text_padding_),
                                              node_text_padding_ + font_metrics.ascent() + font_metrics.height()*index);
  } else {
    return content_rect_.topLeft() + QPointF(node_connector_size_ + node_text_padding_,
                                             node_text_padding_ + font_metrics.ascent() + font_metrics.height()*index);
  }
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
  // HACK for getting the main QWidget palette color (the `widget`'s palette uses the NodeView color instead which we
  // don't want here)
  QPalette app_pal = olive::core.main_window()->palette();

  // Set up border, which will change color if selected
  QPen border_pen(css_proxy_.BorderColor(), node_border_width_);

  QPen text_pen(app_pal.color(QPalette::Text));

  QBrush connector_brush(app_pal.color(QPalette::Text));

  painter->setPen(border_pen);

  if (expanded_ && node_ != nullptr) {

    // Use main widget color for node contents
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

      if (param->type() == NodeParam::kOutput) {
        text_pt -= QPointF(QFontMetricsWidth(&font_metrics, param->name()), 0);
      }

      painter->drawText(text_pt, param->name());
    }

    painter->setPen(border_pen);
  }

  // Draw rect
  painter->setBrush(css_proxy_.TitleBarColor());
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
    expand_hitbox_ = title_bar_rect_.adjusted(node_icon_padding_,
                                              node_icon_padding_,
                                              -node_icon_padding_,
                                              -node_icon_padding_);

    // Make the icon rect a square
    expand_hitbox_.setWidth(expand_hitbox_.height());

    // Draw the icon
    if (IsExpanded()) {
      olive::icon::TriDown.paint(painter, expand_hitbox_.toRect(), Qt::AlignLeft | Qt::AlignVCenter);
    } else {
      olive::icon::TriRight.paint(painter, expand_hitbox_.toRect(), Qt::AlignLeft | Qt::AlignVCenter);
    }

    // Draw the text in a rect (the rect is sized around text already in the constructor)
    QRectF text_rect = title_bar_rect_.adjusted(node_icon_padding_ + expand_hitbox_.width() + node_text_padding_,
                                                node_text_padding_,
                                                -node_text_padding_,
                                                -node_text_padding_);
    painter->drawText(text_rect, static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft), node_->Name());
  }
}

#include "node/block/block.h"

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

      if (GetParameterConnectorRect(i).contains(event->pos())) { // See if the cursor is in the rect

        NodeParam* param = node_->ParamAt(i);

        // Create draggable object
        dragging_edge_ = new NodeViewEdge();

        // Set up a QUndoCommand to make this action undoable
        node_edge_change_command_ = new QUndoCommand();

        if (param->type() == NodeParam::kOutput || param->edges().isEmpty()) {
          // For an output param (or an input param with no connections), we default to creating a new edge
          drag_source_ = this;
          drag_src_param_ = param;

          // Set the starting position to the current param's connector
          dragging_edge_start_ = mapToScene(GetParameterConnectorRect(i).center());

        } else if (param->type() == NodeParam::kInput) {
          // For an input param, we default to moving an existing edge
          // (here we use the last one, which will usually also be the first)
          NodeEdgePtr edge = param->edges().last();

          // The starting position will be the OPPOSING parameter's rectangle

          // Get the opposing parameter
          drag_src_param_ = edge->output();

          // Get its Node UI object
          drag_source_ = NodeView::NodeToUIObject(scene(), drag_src_param_->parent());

          // Get the opposing parameter's rect center using the line's current coordinates
          // (we use the current coordinates because a complex formula is used for the line's coords if the opposing
          //  node is collapsed, therefore it's easier to just retrieve it from line itself)
          NodeViewEdge* existing_edge_ui = NodeView::EdgeToUIObject(scene(), edge);
          QLineF existing_edge_line = existing_edge_ui->line();
          if (existing_edge_ui->contains(existing_edge_line.p1())) {
            dragging_edge_start_ = existing_edge_line.p1();
          } else {
            dragging_edge_start_ = existing_edge_line.p2();
          }


          // Remove old edge
          NodeEdgeRemoveCommand* remove_command = new NodeEdgeRemoveCommand(edge->output(),
                                                                            edge->input(),
                                                                            node_edge_change_command_);
          remove_command->redo();

        }

        // Add it to the scene
        scene()->addItem(dragging_edge_);

        // Trigger initial line setting
        mouseMoveEvent(event);

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
  // Check if an edge drag was initiated
  if (dragging_edge_ != nullptr) {
    QPointF end_point = event->scenePos();
    drag_dest_param_ = nullptr;

    // See if the mouse is currently inside a node
    NodeViewItem* drop_item = dynamic_cast<NodeViewItem*>(scene()->itemAt(event->scenePos(), sceneTransform()));

    // If we expanded an item below but are no longer dragging over it, re-collapse it
    if (drag_expanded_item_ != nullptr && drop_item != drag_expanded_item_) {
      drag_expanded_item_->SetExpanded(false);
      drag_expanded_item_ = nullptr;
    }

    if (drop_item != nullptr && drop_item != drag_source_) {

      // If the item we're dragging over is collapsed, expand it
      if (!drop_item->IsExpanded()) {
        drag_expanded_item_ = drop_item;
        drop_item->SetExpanded(true);
      }

      // See if the mouse is currently inside a connector rect
      for (int i=0;i<drop_item->node()->ParameterCount();i++) {

        // Make a larger "hitbox" rect to make it easier to drag into
        QRectF param_hitbox = drop_item->GetParameterConnectorRect(i).adjusted(-node_connector_size_,
                                                                               -node_connector_size_,
                                                                               node_connector_size_,
                                                                               node_connector_size_);

        // Get the parameter we're dragging into
        NodeParam* comp_param = drop_item->node()->ParamAt(i);

        if (param_hitbox.contains(drop_item->mapFromScene(event->scenePos())) // See if we're dragging inside the hitbox
            && NodeParam::AreDataTypesCompatible(drag_src_param_, comp_param)) { // Make sure the types are compatible

          // Prevent circular dependency - check if the Node we'll be outputting to already outputs to this Node
          Node* outputting_node;
          Node* receiving_node;

          // Determine which Node will be "submitting output" and which node will be "receiving input"
          if (drag_src_param_->type() == NodeParam::kInput) {
            receiving_node = drag_src_param_->parent();
            outputting_node = drop_item->node();
          } else {
            receiving_node = drop_item->node();
            outputting_node = drag_src_param_->parent();
          }

          // Ensure the receiving node doesn't output to the outputting node
          if (!receiving_node->OutputsTo(outputting_node)) {
            drag_dest_param_ = comp_param;
            end_point = drop_item->mapToScene(drop_item->GetParameterConnectorRect(i).center());
          }

          break;
        }
      }
    }

    dragging_edge_->SetConnected(drag_dest_param_ != nullptr);

    dragging_edge_->setLine(QLineF(dragging_edge_start_, end_point));

    return;
  }

  if (standard_click_) {
    QGraphicsRectItem::mouseMoveEvent(event);
  }
}

void NodeViewItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  // Check if an edge drag was initiated
  if (dragging_edge_ != nullptr) {

    // Remove the drag object
    scene()->removeItem(dragging_edge_);

    // If we expanded an item in the drag, re-collapse it now
    if (drag_expanded_item_ != nullptr) {
      drag_expanded_item_->SetExpanded(false);
      drag_expanded_item_ = nullptr;
    }

    if (drag_dest_param_ != nullptr) {
      // We dragged to somewhere, so we'll make a new connection

      NodeEdgePtr new_edge;

      NodeOutput* output;
      NodeInput* input;

      // Connecting will automatically add an edge UI object through the signal/slot system
      if (drag_dest_param_->type() == NodeParam::kOutput) {
        output = static_cast<NodeOutput*>(drag_dest_param_);
        input = static_cast<NodeInput*>(drag_src_param_);
      } else {
        output = static_cast<NodeOutput*>(drag_src_param_);
        input = static_cast<NodeInput*>(drag_dest_param_);
      }

      // Use a command to make Node connecting undoable
      new NodeEdgeAddCommand(output,
                             input,
                             node_edge_change_command_);
    }

    dragging_edge_ = nullptr;

    if (node_edge_change_command_->childCount() > 0) {
      olive::undo_stack.push(node_edge_change_command_);
    } else {
      delete node_edge_change_command_;
    }
    node_edge_change_command_ = nullptr;
    return;
  }

  // Check if we clicked the Expand/Collapse icon
  if (expand_hitbox_.contains(event->pos())) {
    SetExpanded(!IsExpanded());
  }

  if (standard_click_) {
    QGraphicsRectItem::mouseReleaseEvent(event);
  }
}
