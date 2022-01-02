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
#include "ui/colorcoding.h"
#include "ui/icons/icons.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

NodeViewItem::NodeViewItem(Node *node, const QString &input, int element, Node *context, QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(node),
  input_(input),
  element_(element),
  context_(context),
  expanded_(false),
  highlighted_(false),
  flow_dir_(NodeViewCommon::kInvalidDirection),
  arrow_click_(false),
  label_as_output_(false)
{
  //
  // We use font metrics to set all the UI measurements for DPI-awareness
  //

  // Set border width
  node_border_width_ = DefaultItemBorder();

  // Set rect size to default
  SetRectSize();

  // Create connector
  input_connector_ = new NodeViewItemConnector(false, this);
  output_connector_ = new NodeViewItemConnector(true, this);

  connect(node_, &Node::LabelChanged, this, &NodeViewItem::NodeAppearanceChanged);
  connect(node_, &Node::ColorChanged, this, &NodeViewItem::NodeAppearanceChanged);

  if (IsOutputItem()) {
    connect(node_, &Node::InputAdded, this, &NodeViewItem::RepopulateInputs);
    connect(node_, &Node::InputRemoved, this, &NodeViewItem::RepopulateInputs);
    RepopulateInputs();

    // Set flags for this widget
    setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemIsSelectable);

    if (context_) {
      SetNodePosition(context_->GetNodePositionDataInContext(node_));
    }
  } else {
    output_connector_->setVisible(false);

    connect(node_, &Node::InputArraySizeChanged, this, &NodeViewItem::InputArraySizeChanged);
    connect(node_, &Node::InputArraySizeChanged, this, &NodeViewItem::InputArraySizeChanged);
  }

  // This should be set during runtime, but just in case here's a default fallback
  SetFlowDirection(NodeViewCommon::kLeftToRight);
}

NodeViewItem::~NodeViewItem()
{
  Q_ASSERT(edges_.isEmpty());
}

Node::Position NodeViewItem::GetNodePositionData() const
{
  return Node::Position(GetNodePosition(), IsExpanded());
}

QPointF NodeViewItem::GetNodePosition() const
{
  return ScreenToNodePoint(pos(), flow_dir_);
}

void NodeViewItem::SetNodePosition(const QPointF &pos)
{
  cached_node_pos_ = pos;

  UpdateNodePosition();
}

void NodeViewItem::SetNodePosition(const Node::Position &pos)
{
  SetNodePosition(pos.position);
  SetExpanded(pos.expanded);
}

QVector<NodeViewEdge *> NodeViewItem::GetAllEdgesRecursively() const
{
  QVector<NodeViewEdge *> list = edges_;

  foreach (NodeViewItem *item, children_) {
    list.append(item->GetAllEdgesRecursively());
  }

  return list;
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
  return QtUtils::QFontMetricsWidth(QFontMetrics(QFont()), "HHHHHHHHHHHHHHHH");;
}

int NodeViewItem::DefaultItemBorder()
{
  return QFontMetrics(QFont()).height() / 12;
}

QPointF NodeViewItem::NodeToScreenPoint(QPointF p, NodeViewCommon::FlowDirection direction)
{
  switch (direction) {
  case NodeViewCommon::kLeftToRight:
    // NodeGraphs are always left-to-right internally, no need to translate
    break;
  case NodeViewCommon::kRightToLeft:
    // Invert X value
    p.setX(-p.x());
    break;
  case NodeViewCommon::kTopToBottom:
    // Swap X/Y
    p = QPointF(p.y(), p.x());
    break;
  case NodeViewCommon::kBottomToTop:
    // Swap X/Y and invert Y
    p = QPointF(p.y(), -p.x());
    break;
  case NodeViewCommon::kInvalidDirection:
    break;
  }

  // Multiply by item sizes for this direction
  p.setX(p.x() * DefaultItemHorizontalPadding(direction));
  p.setY(p.y() * DefaultItemVerticalPadding(direction));

  return p;
}

QPointF NodeViewItem::ScreenToNodePoint(QPointF p, NodeViewCommon::FlowDirection direction)
{
  // Divide by item sizes for this direction
  p.setX(p.x() / DefaultItemHorizontalPadding(direction));
  p.setY(p.y() / DefaultItemVerticalPadding(direction));

  switch (direction) {
  case NodeViewCommon::kLeftToRight:
    // NodeGraphs are always left-to-right internally, no need to translate
    break;
  case NodeViewCommon::kRightToLeft:
    // Invert X value
    p.setX(-p.x());
    break;
  case NodeViewCommon::kTopToBottom:
    // Swap X/Y
    p = QPointF(p.y(), p.x());
    break;
  case NodeViewCommon::kBottomToTop:
    // Swap X/Y and invert Y
    p = QPointF(-p.y(), p.x());
    break;
  case NodeViewCommon::kInvalidDirection:
    break;
  }

  return p;
}

qreal NodeViewItem::DefaultItemHorizontalPadding(NodeViewCommon::FlowDirection dir)
{
  if (NodeViewCommon::GetFlowOrientation(dir) == Qt::Horizontal) {
    return DefaultItemWidth() * 1.5;
  } else {
    return DefaultItemWidth() * 1.25;
  }
}

qreal NodeViewItem::DefaultItemVerticalPadding(NodeViewCommon::FlowDirection dir)
{
  if (NodeViewCommon::GetFlowOrientation(dir) == Qt::Horizontal) {
    return DefaultItemHeight() * 1.5;
  } else {
    return DefaultItemHeight() * 2.0;
  }
}

qreal NodeViewItem::DefaultItemHorizontalPadding() const
{
  return DefaultItemHorizontalPadding(flow_dir_);
}

qreal NodeViewItem::DefaultItemVerticalPadding() const
{
  return DefaultItemVerticalPadding(flow_dir_);
}

void NodeViewItem::AddEdge(NodeViewEdge *edge)
{
  edges_.append(edge);
}

void NodeViewItem::RemoveEdge(NodeViewEdge *edge)
{
  edges_.removeOne(edge);
}

void NodeViewItem::SetExpanded(bool e, bool hide_titlebar)
{
  if (!CanBeExpanded() || (expanded_ == e)) {
    return;
  }

  expanded_ = e;

  if (context_) {
    context_->SetNodeExpandedInContext(node_, e);
  }

  if (IsOutputItem()) {
    // We don't have to check has_connectable_inputs_ here because we did it at the top
    input_connector_->setVisible(!expanded_);
  }

  if (expanded_) {
    node_->Retranslate();

    if (IsOutputItem()) {
      // Create items for each input of the node
      int i = 1;
      foreach (const QString &input, node_->inputs()) {
        if (IsInputValid(input)) {
          NodeViewItem *item = new NodeViewItem(node_, input, -1, context_, this);
          children_.append(item);
          i++;
        }
      }

      QVector<NodeViewEdge*> edges = edges_;
      for (auto it=edges.cbegin(); it!=edges.cend(); it++) {
        if ((*it)->to_item() == this) {
          (*it)->set_to_item(GetItemForInput((*it)->input()));
        }
      }
    } else {
      // Create items for each element of the input array
      int arr_sz = node_->InputArraySize(input_);
      children_.resize(arr_sz);
      for (int i=0; i<arr_sz; i++) {
        NodeViewItem *item = new NodeViewItem(node_, input_, i, context_, this);
        children_[i] = item;
      }

      QVector<NodeViewEdge*> edges = edges_;
      for (auto it=edges.cbegin(); it!=edges.cend(); it++) {
        if ((*it)->to_item() == this) {
          (*it)->set_to_item(GetItemForInput((*it)->input()));
        }
      }
    }
  } else {
    foreach (NodeViewItem *child, children_) {
      QVector<NodeViewEdge*> child_edges = child->edges();
      foreach (NodeViewEdge *edge, child_edges) {
        edge->set_to_item(this);
      }
      delete child;
    }
    children_.clear();
  }

  UpdateChildrenPositions();

  if (flow_dir_ == NodeViewCommon::kTopToBottom) {
    UpdateOutputConnectorPosition();
  }

  ReadjustAllEdges();

  UpdateContextRect();

  update();
}

void NodeViewItem::ToggleExpanded()
{
  SetExpanded(!IsExpanded());
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
  // Use main window palette since the palette passed in `widget` is the NodeView palette which
  // has been slightly modified
  QPalette app_pal = Core::instance()->main_window()->palette();

  // We only draw a single unit's worth
  QRectF single_unit_rect = rect();
  single_unit_rect.setHeight(DefaultItemHeight());

  if (IsOutputItem()) {
    // Set output item colors
    painter->setPen(Qt::black);
    painter->setBrush(node_->brush(single_unit_rect.top(), single_unit_rect.bottom()));
  } else {
    // Set input item colors
    painter->setPen(Qt::NoPen);
    painter->setBrush(element_ == -1 ? app_pal.color(QPalette::Window) : app_pal.color(QPalette::Base));
  }

  painter->drawRect(single_unit_rect);

  // Draw highlight if applicable
  if (highlighted_) {
    QColor highlight_col = app_pal.color(QPalette::Text);
    highlight_col.setAlpha(64);
    painter->setBrush(highlight_col);
    painter->drawRect(rect());
  }

  // Determine what text to draw and whether to draw an arrow
  QString node_label, node_name;

  if (IsOutputItem()) {
    if (label_as_output_) {
      node_name = QCoreApplication::translate("NodeViewItem", "Output");
    } else {
      node_label = node_->GetLabel();
      node_name = node_->ShortName();
    }
  } else {
    if (element_ == -1) {
      node_name = node_->GetInputName(input_);
    } else {
      node_name = QString::number(element_);
    }
  }

  // Draw arrow if necessary
  int arrow_size = CanBeExpanded() ? DrawExpandArrow(painter) : 0;

  if (IsOutputItem()) {
    // Determine the text color (automatically calculate from node background color)
    painter->setPen(ColorCoding::GetUISelectorColor(node_->color()));
  } else {
    // Just use text item
    painter->setPen(app_pal.text().color());
  }

  if (node_label.isEmpty()) {
    // Draw name only
    DrawNodeTitle(painter, node_name, single_unit_rect, Qt::AlignVCenter, arrow_size);
  } else {
    int text_pad = DefaultTextPadding()/2;
    QRectF safe_label_bounds = single_unit_rect.adjusted(text_pad, text_pad, -text_pad, -text_pad);
    QFont f;
    qreal font_sz = f.pointSizeF();

    // Draw label as larger/upper text
    f.setPointSizeF(font_sz * 0.8);
    painter->setFont(f);
    DrawNodeTitle(painter, node_label, safe_label_bounds, Qt::AlignTop, arrow_size);

    // Draw node name as smaller/lower text
    f.setPointSizeF(font_sz * 0.6);
    painter->setFont(f);
    DrawNodeTitle(painter, node_name, safe_label_bounds, Qt::AlignBottom, arrow_size);
  }

  // Draw final border (output only)
  if (IsOutputItem()) {
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
}

void NodeViewItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  if (last_arrow_rect_.contains(event->pos().toPoint())) {
    arrow_click_ = true;
    ToggleExpanded();
    return;
  }

  event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

  QGraphicsRectItem::mousePressEvent(event);
}

void NodeViewItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (arrow_click_) {
    return;
  }

  event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

  QGraphicsRectItem::mouseMoveEvent(event);
}

void NodeViewItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  if (arrow_click_) {
    arrow_click_ = false;
    return;
  }

  event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

  QGraphicsRectItem::mouseReleaseEvent(event);
}

QVariant NodeViewItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionHasChanged && node_) {
    ReadjustAllEdges();

    UpdateContextRect();
  }

  return QGraphicsItem::itemChange(change, value);
}

void NodeViewItem::ReadjustAllEdges()
{
  foreach (NodeViewEdge* edge, edges_) {
    if (NodeViewItem *to_item = edge->to_item()) {
      static_cast<NodeViewItem*>(to_item->parentItem())->UpdateFlowDirectionOfInputItem(to_item);
    }

    edge->Adjust();
  }
  foreach (NodeViewItem *child, children_) {
    child->ReadjustAllEdges();
  }
}

void NodeViewItem::UpdateContextRect()
{
  QGraphicsItem *item = parentItem();

  while (item) {
    if (NodeViewContext *ctx = dynamic_cast<NodeViewContext*>(item)) {
      ctx->UpdateRect();
      break;
    }

    item = item->parentItem();
  }
}

void NodeViewItem::DrawNodeTitle(QPainter* painter, QString text, const QRectF& rect, Qt::Alignment vertical_align, int icon_full_size)
{
  QFontMetrics fm = painter->fontMetrics();

  // Calculate how much space we have for text
  int item_width = this->rect().width();
  int max_text_width = item_width - DefaultTextPadding() * 2 - icon_full_size;
  int label_width = QtUtils::QFontMetricsWidth(fm, text);

  // Concatenate text if necessary (adds a "..." to the end and removes characters until the
  // string fits in the bounds)
  if (label_width > max_text_width) {
    QString concatenated;

    do {
      text.chop(1);
      concatenated = QCoreApplication::translate("NodeViewItem", "%1...").arg(text);
    } while ((label_width = QtUtils::QFontMetricsWidth(fm, concatenated)) > max_text_width);

    text = concatenated;
  }

  // Determine X position (favors horizontal centering unless it'll overrun the arrow)
  QRectF text_rect = rect;
  Qt::Alignment text_align = Qt::AlignHCenter | vertical_align;
  int likely_x = item_width / 2 - label_width / 2;
  if (likely_x < icon_full_size) {
    text_rect.adjust(icon_full_size, 0, 0, 0);
    text_align = Qt::AlignLeft | vertical_align;
  }

  // Draw the text in a rect (the rect is sized around text already in the constructor)
  painter->drawText(text_rect,
                    text_align,
                    text);
}

int NodeViewItem::DrawExpandArrow(QPainter *painter)
{
  // Draw right or down arrow based on expanded state
  int icon_size = painter->fontMetrics().height()/2;
  int icon_padding = DefaultItemHeight() / 2 - icon_size / 2;
  int icon_full_size = icon_size + icon_padding * 2;

  painter->setRenderHint(QPainter::SmoothPixmapTransform);

  const QIcon& expand_icon = IsExpanded() ? icon::TriDown : icon::TriRight;
  int icon_size_scaled = icon_size * painter->transform().m11();

  last_arrow_rect_ = QRect(this->rect().x() + icon_padding,
                           this->rect().y() + icon_padding,
                           icon_size,
                           icon_size);

  painter->drawPixmap(last_arrow_rect_, expand_icon.pixmap(QSize(icon_size_scaled, icon_size_scaled)));

  return icon_full_size;
}

void NodeViewItem::SetLabelAsOutput(bool e)
{
  label_as_output_ = e;
  output_connector_->setVisible(!e);
  update();
}

QPointF NodeViewItem::GetInputPoint() const
{
  return input_connector_->scenePos();
}

QPointF NodeViewItem::GetOutputPoint() const
{
  QPointF p = output_connector_->scenePos();
  QRectF r = output_connector_->boundingRect();

  switch (flow_dir_) {
  case NodeViewCommon::kLeftToRight:
  default:
    p.setX(p.x() + r.width());
    break;
  case NodeViewCommon::kRightToLeft:
    p.setX(p.x() - r.width());
    break;
  case NodeViewCommon::kTopToBottom:
    p.setY(p.y() + r.height());
    break;
  case NodeViewCommon::kBottomToTop:
    p.setY(p.y() - r.height());
    break;
  }

  return p;
}

void NodeViewItem::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  if (flow_dir_ != dir) {
    flow_dir_ = dir;

    input_connector_->SetFlowDirection(dir);
    output_connector_->SetFlowDirection(dir);

    UpdateInputConnectorPosition();
    UpdateOutputConnectorPosition();

    if (IsOutputItem()) {
      UpdateNodePosition();
    }

    ReadjustAllEdges();
  }
}

void NodeViewItem::UpdateNodePosition()
{
  setPos(NodeToScreenPoint(cached_node_pos_, flow_dir_));
}

void NodeViewItem::UpdateInputConnectorPosition()
{
  QRectF output_rect = input_connector_->boundingRect();

  NodeViewCommon::FlowDirection using_flow_dir = flow_dir_;

  if (IsExpanded() && !NodeViewCommon::IsFlowHorizontal(flow_dir_)) {
    if (edges_.isEmpty() || edges_.first()->from_item()->x() < this->x()) {
      using_flow_dir = NodeViewCommon::kLeftToRight;
    } else {
      using_flow_dir = NodeViewCommon::kRightToLeft;
    }
  }

  // Input connector flow directions change conditionally
  switch (using_flow_dir) {
  case NodeViewCommon::kLeftToRight:
    input_connector_->setPos(rect().left() - output_rect.width(), 0);
    break;
  case NodeViewCommon::kRightToLeft:
    input_connector_->setPos(rect().right() + output_rect.width(), 0);
    break;
  case NodeViewCommon::kTopToBottom:
    input_connector_->setPos(rect().center().x(), rect().top() - output_rect.height());
    break;
  case NodeViewCommon::kBottomToTop:
    input_connector_->setPos(rect().center().x(), rect().bottom() + output_rect.height());
    break;
  case NodeViewCommon::kInvalidDirection:
    break;
  }
}

void NodeViewItem::UpdateOutputConnectorPosition()
{
  switch (flow_dir_) {
  case NodeViewCommon::kLeftToRight:
    output_connector_->setPos(rect().right(), 0);
    break;
  case NodeViewCommon::kRightToLeft:
    output_connector_->setPos(rect().left(), 0);
    break;
  case NodeViewCommon::kTopToBottom:
    output_connector_->setPos(rect().center().x(), rect().bottom());
    break;
  case NodeViewCommon::kBottomToTop:
    output_connector_->setPos(rect().center().x(), rect().top());
    break;
  case NodeViewCommon::kInvalidDirection:
    break;
  }
}

bool NodeViewItem::IsInputValid(const QString &input)
{
  return node_->IsInputConnectable(input) && !node_->IsInputHidden(input);
}

void NodeViewItem::SetRectSize(int height_units)
{
  // Set rect
  int widget_width = DefaultItemWidth();
  int widget_height = DefaultItemHeight();

  setRect(QRectF(-widget_width/2, -widget_height/2, widget_width, widget_height * height_units));
}

bool NodeViewItem::CanBeExpanded() const
{
  if (IsOutputItem()) {
    return has_connectable_inputs_;
  } else {
    return node_->GetInputFlags(input_) & kInputFlagArray && element_ == -1 && !node_->IsInputConnected(input_);
  }
}

void NodeViewItem::UpdateChildrenPositions()
{
  int y = 1;
  int h = DefaultItemHeight();

  foreach (NodeViewItem *c, children_) {
    c->setPos(QPointF(0, y * h));

    y += c->GetLogicalHeightWithChildren();
  }

  SetRectSize(y);

  if (NodeViewItem *p = dynamic_cast<NodeViewItem*>(parentItem())) {
    p->UpdateChildrenPositions();
  }
}

int NodeViewItem::GetLogicalHeightWithChildren() const
{
  int h = 1;

  foreach (NodeViewItem *c, children_) {
    h += c->GetLogicalHeightWithChildren();
  }

  return h;
}

void NodeViewItem::UpdateFlowDirectionOfInputItem(NodeViewItem *child)
{
  if (!child->IsOutputItem()) {
    if (NodeViewCommon::IsFlowVertical(flow_dir_)) {
      if (!child->edges().isEmpty() && child->edges().first()->from_item()->scenePos().x() > child->scenePos().x()) {
        child->SetFlowDirection(NodeViewCommon::kRightToLeft);
      } else {
        child->SetFlowDirection(NodeViewCommon::kLeftToRight);
      }
    } else {
      child->SetFlowDirection(flow_dir_);
    }
  }
}

void NodeViewItem::RepopulateInputs()
{
  if (IsOutputItem()) {
    has_connectable_inputs_ = false;

    foreach (const QString& input, node_->inputs()) {
      if (IsInputValid(input)) {
        has_connectable_inputs_ = true;
        break;
      }
    }

    input_connector_->setVisible(has_connectable_inputs_);
  }

  if (IsExpanded() && (IsOutputItem() || element_ == -1)) {
    // Create or remove inputs when necessary
    // NOTE: This is not the most efficient thing in the world, but it does work
    SetExpanded(false);
    SetExpanded(true);
  }
}

void NodeViewItem::InputArraySizeChanged(const QString &input)
{
  if (input == input_) {
    RepopulateInputs();
  }
}

void NodeViewItem::NodeAppearanceChanged()
{
  update();
}

void NodeViewItem::SetHighlighted(bool e)
{
  highlighted_ = e;
  update();
}

NodeViewItem *NodeViewItem::GetItemForInput(NodeInput input)
{
  if (NodeGroup *group = dynamic_cast<NodeGroup*>(node_)) {
    if (input.node() != group) {
      // Translate input to group input
      QString id = NodeGroup::GetGroupInputIDFromInput(input);
      input.set_node(group);
      input.set_input(id);
    }
  }

  if (IsExpanded()) {
    if (input_.isEmpty()) {
      // Look for the input in our children
      foreach (NodeViewItem *i, children_) {
        if (i->input_ == input.input()) {
          return i->GetItemForInput(input);
        }
      }
    } else {
      // Look for element in our children
      if (input.element() >= 0 && input.element() < children_.size()) {
        return children_.at(input.element())->GetItemForInput(input);
      }
    }
  }

  // Fallback to this object
  return this;
}

}
