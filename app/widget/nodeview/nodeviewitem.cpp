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

#include "common/flipmodifiers.h"
#include "common/qtutils.h"
#include "config/config.h"
#include "core.h"
#include "nodeview.h"
#include "nodeviewscene.h"
#include "nodeviewundo.h"
#include "ui/icons/icons.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

NodeViewItem::NodeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(nullptr),
  dragging_edge_(nullptr),
  cached_drop_item_(nullptr),
  cached_drop_item_expanded_(false),
  expanded_(false),
  hide_titlebar_(false),
  standard_click_(false),
  highlighted_index_(-1),
  node_edge_change_command_(nullptr),
  flow_dir_(NodeViewCommon::kLeftToRight)
{
  // Set flags for this widget
  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  //
  // We use font metrics to set all the UI measurements for DPI-awareness
  //

  // Set border width
  node_border_width_ = DefaultItemBorder();

  int widget_width = DefaultItemWidth();
  int widget_height = DefaultItemHeight();

  title_bar_rect_ = QRectF(-widget_width/2, -widget_height/2, widget_width, widget_height);
  setRect(title_bar_rect_);
}

QPointF NodeViewItem::GetNodePosition() const
{
  QPointF node_pos;

  qreal adjusted_x = pos().x() / DefaultItemHorizontalPadding();
  qreal adjusted_y = pos().y() / DefaultItemVerticalPadding();

  switch (flow_dir_) {
  case NodeViewCommon::kLeftToRight:
    node_pos.setX(adjusted_x);
    node_pos.setY(adjusted_y);
    break;
  case NodeViewCommon::kRightToLeft:
    node_pos.setX(-adjusted_x);
    node_pos.setY(adjusted_y);
    break;
  case NodeViewCommon::kTopToBottom:
    node_pos.setX(adjusted_y);
    node_pos.setY(adjusted_x);
    break;
  case NodeViewCommon::kBottomToTop:
    node_pos.setX(-adjusted_y);
    node_pos.setY(adjusted_x);
    break;
  }

  return node_pos;
}

void NodeViewItem::SetNodePosition(const QPointF &pos)
{
  switch (flow_dir_) {
  case NodeViewCommon::kLeftToRight:
    setPos(pos.x() * DefaultItemHorizontalPadding(),
           pos.y() * DefaultItemVerticalPadding());
    break;
  case NodeViewCommon::kRightToLeft:
    setPos(-pos.x() * DefaultItemHorizontalPadding(),
           pos.y() * DefaultItemVerticalPadding());
    break;
  case NodeViewCommon::kTopToBottom:
    setPos(pos.y() * DefaultItemHorizontalPadding(),
           pos.x() * DefaultItemVerticalPadding());
    break;
  case NodeViewCommon::kBottomToTop:
    setPos(pos.y() * DefaultItemHorizontalPadding(),
           -pos.x() * DefaultItemVerticalPadding());
    break;
  }
}

int NodeViewItem::DefaultTextPadding()
{
  return QFontMetrics(QFont()).height() / 4;
}

int NodeViewItem::DefaultItemHeight()
{
  return QFontMetrics(QFont()).height() + DefaultTextPadding() * 2;
}

int NodeViewItem::DefaultItemWidth()
{
  return QFontMetricsWidth(QFontMetrics(QFont()), "HHHHHHHHHH");;
}

int NodeViewItem::DefaultMaximumTextWidth()
{
  return QFontMetricsWidth(QFontMetrics(QFont()), "HHHHHHHH");;
}

int NodeViewItem::DefaultItemBorder()
{
  return QFontMetrics(QFont()).height() / 12;
}

qreal NodeViewItem::DefaultItemHorizontalPadding() const
{
  if (NodeViewCommon::GetFlowOrientation(flow_dir_) == Qt::Horizontal) {
    return DefaultItemWidth() * 1.5;
  } else {
    return DefaultItemWidth() * 1.25;
  }
}

qreal NodeViewItem::DefaultItemVerticalPadding() const
{
  if (NodeViewCommon::GetFlowOrientation(flow_dir_) == Qt::Horizontal) {
    return DefaultItemHeight() * 1.5;
  } else {
    return DefaultItemHeight() * 2.0;
  }
}

void NodeViewItem::SetNode(Node *n)
{
  node_ = n;

  node_inputs_.clear();

  if (node_) {
    node_->Retranslate();

    foreach (NodeParam* p, node_->parameters()) {
      if (p->type() == NodeParam::kInput) {
        NodeInput* input = static_cast<NodeInput*>(p);

        if (input->IsConnectable()) {
          node_inputs_.append(input);
        }
      }
    }

    SetNodePosition(node_->GetPosition());
  }

  update();
}

Node *NodeViewItem::GetNode() const
{
  return node_;
}

bool NodeViewItem::IsExpanded() const
{
  return expanded_;
}

void NodeViewItem::SetExpanded(bool e, bool hide_titlebar)
{
  if (node_inputs_.isEmpty()
      || (expanded_ == e && hide_titlebar_ == hide_titlebar)) {
    return;
  }

  expanded_ = e;
  hide_titlebar_ = hide_titlebar;

  if (expanded_ && !node_inputs_.isEmpty()) {
    // Create new rect
    QRectF new_rect = title_bar_rect_;

    if (hide_titlebar_) {
      new_rect.setHeight(new_rect.height() * node_inputs_.size());
    } else {
      new_rect.setHeight(new_rect.height() * (node_inputs_.size() + 1));
    }

    setRect(new_rect);
  } else {
    setRect(title_bar_rect_);
  }

  update();
}

void NodeViewItem::ToggleExpanded()
{
  SetExpanded(!IsExpanded());
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
  // HACK for getting the main QWidget palette color (the `widget`'s palette uses the NodeView color instead which we
  // don't want here)
  QPalette app_pal = Core::instance()->main_window()->palette();

  // Draw background rect if expanded
  if (IsExpanded()) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(app_pal.color(QPalette::Window));

    painter->drawRect(rect());

    painter->setPen(app_pal.color(QPalette::Text));

    for (int i=0;i<node_inputs_.size();i++) {
      QRectF input_rect = GetInputRect(i);

      if (highlighted_index_ == i) {
        painter->fillRect(input_rect, QColor(255, 255, 255, 64));
      }

      painter->drawText(input_rect, Qt::AlignCenter, node_inputs_.at(i)->name());
    }
  }

  // Draw the titlebar
  if (!hide_titlebar_ && node_) {

    Color node_color = Config::Current()[QStringLiteral("NodeCatColor%1")
        .arg(node_->Category().first())].value<Color>();

    painter->setPen(Qt::black);
    painter->setBrush(node_color.toQColor());

    painter->drawRect(title_bar_rect_);

    painter->setPen(app_pal.color(QPalette::Text));

    QString node_label;

    if (node_->GetLabel().isEmpty()) {
      node_label = node_->ShortName();
    } else {
      node_label = node_->GetLabel();
    }

    {
      QFont f;
      QFontMetrics fm(f);

      int max_text_width = DefaultMaximumTextWidth();

      if (QFontMetricsWidth(fm, node_label) > max_text_width) {
        QString concatenated;

        do {
          node_label.chop(1);
          concatenated = QCoreApplication::translate("NodeViewItem", "%1...").arg(node_label);
        } while (QFontMetricsWidth(fm, concatenated) > max_text_width);

        node_label = concatenated;
      }
    }

    if (node_color.GetRoughLuminance() > 0.66) {
      painter->setPen(Qt::black);
    } else {
      painter->setPen(Qt::white);
    }

    // Draw the text in a rect (the rect is sized around text already in the constructor)
    painter->drawText(title_bar_rect_,
                      Qt::AlignCenter,
                      node_label);

  }

  // Draw final border
  QPen border_pen;
  border_pen.setWidth(node_border_width_);

  if (option->state & QStyle::State_Selected) {
    border_pen.setColor(app_pal.color(QPalette::Highlight));
  } else {
    border_pen.setColor(Qt::black);
  }

  painter->setPen(border_pen);
  painter->setBrush(Qt::NoBrush);

  painter->drawRect(rect());
}

void NodeViewItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  // We override standard mouse behavior in some cases. In these cases, we don't want the standard "move" and "release"
  // events to trigger if we haven't already triggered the "press" event. We use this variable to determine whether
  // base class behavior is valid here.
  standard_click_ = false;

  // If CTRL is held, we start a new edge
  if (node_ && event->modifiers() & Qt::ControlModifier) {

    NodeParam* param = node_->output();

    // Create draggable object
    dragging_edge_ = new NodeViewEdge();
    dragging_edge_->SetFlowDirection(flow_dir_);

    // Set up a QUndoCommand to make this action undoable
    node_edge_change_command_ = new QUndoCommand();

    if (param->type() == NodeParam::kOutput || param->edges().isEmpty()) {
      // For an output param (or an input param with no connections), we default to creating a new edge
      drag_source_ = this;
      drag_src_param_ = param;

      // Set the starting position to the current param's connector
      dragging_edge_start_ = GetParamPoint(param, QPointF());

    } else if (param->type() == NodeParam::kInput) {
      // For an input param, we default to moving an existing edge
      // (here we use the last one, which will usually also be the first)
      NodeEdgePtr edge = param->edges().last();

      // The starting position will be the OPPOSING parameter's rectangle

      // Get the opposing parameter
      drag_src_param_ = edge->output();

      // Get its Node UI object
      drag_source_ = static_cast<NodeViewScene*>(scene())->NodeToUIObject(drag_src_param_->parentNode());

      // Get the opposing parameter's rect center using the line's current coordinates
      // (we use the current coordinates because a complex formula is used for the line's coords if
      // the opposing node is collapsed, therefore it's easier to just retrieve it from line itself)
      NodeViewEdge* existing_edge_ui = static_cast<NodeViewScene*>(scene())->EdgeToUIObject(edge);
      QPainterPath existing_edge_line = existing_edge_ui->path();
      QPointF edge_start = existing_edge_line.pointAtPercent(0);
      QPointF edge_end = existing_edge_line.pointAtPercent(1);
      if (existing_edge_ui->contains(edge_start)) {
        dragging_edge_start_ = edge_start;
      } else {
        dragging_edge_start_ = edge_end;
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

  } else {

    // We aren't using any override behaviors, switch back to standard click behavior
    standard_click_ = true;

    event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

    QGraphicsRectItem::mousePressEvent(event);

  }
}

void NodeViewItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  // Check if an edge drag was initiated
  if (dragging_edge_ != nullptr) {
    QPointF end_point = event->scenePos();
    drag_dest_param_ = nullptr;

    if (!cached_drop_item_
        || !cached_drop_item_->contains(cached_drop_item_->mapFromScene(event->scenePos()))) {

      // Cursor has left last entered item, we need to find a new one
      if (cached_drop_item_) {

        // If we expanded an item below but are no longer dragging over it, re-collapse it
        if (cached_drop_item_expanded_) {
          cached_drop_item_->SetExpanded(false);
        }

        cached_drop_item_->SetHighlightedIndex(-1);

        cached_drop_item_->setZValue(0);
        cached_drop_item_ = nullptr;
      }

      NodeViewItem* cursor_item  = dynamic_cast<NodeViewItem*>(scene()->itemAt(event->scenePos(), sceneTransform()));

      if (cursor_item
          && cursor_item != drag_source_
          && !cursor_item->node_inputs_.isEmpty()) {
        cached_drop_item_ = cursor_item;

        // If the item we're dragging over is collapsed, expand it
        cached_drop_item_expanded_ = !cached_drop_item_->IsExpanded();

        if (cached_drop_item_expanded_) {
          cached_drop_item_->SetExpanded(true, true);
        }

        cached_drop_item_->setZValue(1);
      } else {
        cached_drop_item_ = nullptr;
      }

    }

    if (cached_drop_item_) {

      int highlight_their_index = -1;

      // See if the mouse is currently inside a connector rect
      for (int i=0;i<cached_drop_item_->node_inputs_.size();i++) {

        // Get the parameter we're dragging into
        NodeParam* comp_param = cached_drop_item_->node_inputs_.at(i);

        // See if cursor is inside its UI
        if (cached_drop_item_->GetInputRect(i).contains(cached_drop_item_->mapFromScene(event->scenePos()))) { // See if we're dragging inside the hitbox

          // Attempt to prevent circular dependency - ensure the receiving node doesn't output to the outputting node
          if (!cached_drop_item_->GetNode()->OutputsTo(node_, true)) {
            drag_dest_param_ = comp_param;
            highlight_their_index = i;

            QPointF end_point_local = cached_drop_item_->GetInputPoint(i, pos());
            end_point = cached_drop_item_->mapToScene(end_point_local);
          }

          break;
        }
      }

      cached_drop_item_->SetHighlightedIndex(highlight_their_index);
    }

    dragging_edge_->SetConnected(drag_dest_param_);

    dragging_edge_->SetPoints(dragging_edge_start_,
                              end_point,
                              cached_drop_item_ ? cached_drop_item_->IsExpanded() : false);

    return;
  }

  if (standard_click_) {
    event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

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
    if (cached_drop_item_) {
      if (cached_drop_item_expanded_) {
        cached_drop_item_->SetExpanded(false);
      }

      cached_drop_item_->SetHighlightedIndex(-1);
      cached_drop_item_->setZValue(0);

      cached_drop_item_ = nullptr;
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

    Core::instance()->undo_stack()->pushIfHasChildren(node_edge_change_command_);
    node_edge_change_command_ = nullptr;
    return;
  }

  if (standard_click_) {
    event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

    QGraphicsRectItem::mouseReleaseEvent(event);
  }
}

void NodeViewItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsRectItem::mouseDoubleClickEvent(event);

  SetExpanded(!IsExpanded());
}

QVariant NodeViewItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionHasChanged && node_) {
    node_->blockSignals(true);
    node_->SetPosition(GetNodePosition());
    node_->blockSignals(false);
  }

  return QGraphicsItem::itemChange(change, value);
}

void NodeViewItem::SetHighlightedIndex(int index)
{
  if (highlighted_index_ == index) {
    return;
  }

  highlighted_index_ = index;

  update();
}

QRectF NodeViewItem::GetInputRect(int index) const
{
  QRectF r = title_bar_rect_;

  if (!hide_titlebar_) {
    index++;
  }

  if (IsExpanded()) {
    r.translate(0, r.height() * index);
  }

  return r;
}

QPointF NodeViewItem::GetParamPoint(NodeParam *param, const QPointF& source_pos) const
{
  if (param->type() == NodeParam::kOutput) {

    switch (flow_dir_) {
    case NodeViewCommon::kLeftToRight:
    default:
      return pos() + QPointF(rect().right(), rect().center().y());
    case NodeViewCommon::kRightToLeft:
      return pos() + QPointF(rect().left(), rect().center().y());
    case NodeViewCommon::kTopToBottom:
      return pos() + QPointF(rect().center().x(), rect().bottom());
    case NodeViewCommon::kBottomToTop:
      return pos() + QPointF(rect().center().x(), rect().top());
    }

  } else {
    NodeInput* input = static_cast<NodeInput*>(param);

    // Resolve NodeInputArray elements
    while (input->parentNode() != input->parent()) {
      input = static_cast<NodeInput*>(input->parent());
    }

    return pos() + GetInputPoint(node_inputs_.indexOf(input), source_pos);
  }
}

void NodeViewItem::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  flow_dir_ = dir;
}

QPointF NodeViewItem::GetInputPoint(int index, const QPointF& source_pos) const
{
  QRectF input_rect = GetInputRect(index);

  Qt::Orientation flow_orientation = NodeViewCommon::GetFlowOrientation(flow_dir_);

  if (flow_orientation == Qt::Horizontal || IsExpanded()) {
    if (flow_dir_ == NodeViewCommon::kLeftToRight
        || (flow_orientation == Qt::Vertical && source_pos.x() < pos().x())) {
      return QPointF(input_rect.left(), input_rect.center().y());
    } else {
      return QPointF(input_rect.right(), input_rect.center().y());
    }
  } else {
    if (flow_dir_ == NodeViewCommon::kTopToBottom) {
      return QPointF(input_rect.center().x(), input_rect.top());
    } else {
      return QPointF(input_rect.center().x(), input_rect.bottom());
    }
  }
}

OLIVE_NAMESPACE_EXIT
