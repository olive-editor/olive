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

NodeViewItem::NodeViewItem(Node* n, Node *context, QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(n),
  context_(context),
  expanded_(false),
  hide_titlebar_(false),
  highlighted_index_(-1),
  flow_dir_(NodeViewCommon::kLeftToRight),
  label_as_output_(false)
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

  output_connector_ = new NodeViewItemConnector(true, this);

  // Set up node
  node_->Retranslate();

  foreach (const QString& input, node_->inputs()) {
    if (node_->IsInputConnectable(input) && !node_->IsInputHidden(input)) {
      node_inputs_.append(input);
    }
  }

  UpdateInputConnectors();

  connect(node_, &Node::LabelChanged, this, &NodeViewItem::NodeAppearanceChanged);
  connect(node_, &Node::ColorChanged, this, &NodeViewItem::NodeAppearanceChanged);

  if (context_) {
    SetNodePosition(context_->GetNodePositionInContext(node_));
    SetExpanded(context_->IsNodeExpandedInContext(node_));
  }
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

int NodeViewItem::GetIndexAt(QPointF pt) const
{
  pt -= this->scenePos();

  for (int i=0; i<node_inputs_.size(); i++) {
    if (GetInputRect(i).contains(pt)) {
      return i;
    }
  }

  return -1;
}

void NodeViewItem::SetExpanded(bool e, bool hide_titlebar)
{
  if (node_inputs_.isEmpty()
      || (expanded_ == e && hide_titlebar_ == hide_titlebar)) {
    return;
  }

  expanded_ = e;
  hide_titlebar_ = hide_titlebar;

  if (context_) {
    context_->SetNodeExpandedInContext(node_, e);
  }

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

  UpdateInputConnectorFlowDirections();

  UpdateConnectorPositions();

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

  // Draw background rect if expanded
  if (IsExpanded()) {
    painter->setPen(Qt::NoPen);
    painter->setBrush(app_pal.color(QPalette::Window));

    painter->drawRect(rect());

    painter->setPen(app_pal.color(QPalette::Text));

    for (int i=0;i<node_inputs_.size();i++) {
      QRectF input_rect = GetInputRect(i);

      if (highlighted_index_ == i) {
        QColor highlight_col = app_pal.color(QPalette::Text);
        highlight_col.setAlpha(64);
        painter->fillRect(input_rect, highlight_col);
      }

      painter->drawText(input_rect, Qt::AlignCenter, node_->GetInputName(node_inputs_.at(i)));
    }
  }

  // Draw the titlebar
  if (!hide_titlebar_ && node_) {

    painter->setPen(Qt::black);
    painter->setBrush(node_->brush(title_bar_rect_.top(), title_bar_rect_.bottom()));

    painter->drawRect(title_bar_rect_);

    painter->setPen(app_pal.color(QPalette::Text));

    QString node_label, node_shortname;

    if (label_as_output_) {
      node_shortname = QCoreApplication::translate("NodeViewItem", "Output");
    } else {
      node_label = node_->GetLabel();
      node_shortname = node_->ShortName();
    }

    int icon_size = painter->fontMetrics().height()/2;

    bool draw_arrow = !node_inputs_.isEmpty();

    if (node_label.isEmpty()) {
      // Draw shortname only
      DrawNodeTitle(painter, node_shortname, title_bar_rect_, Qt::AlignVCenter, icon_size, draw_arrow);
    } else {
      int text_pad = DefaultTextPadding()/2;
      QRectF safe_label_bounds = title_bar_rect_.adjusted(text_pad, text_pad, -text_pad, -text_pad);
      QFont f;
      qreal font_sz = f.pointSizeF();
      f.setPointSizeF(font_sz * 0.8);
      painter->setFont(f);
      DrawNodeTitle(painter, node_label, safe_label_bounds, Qt::AlignTop, icon_size, draw_arrow);
      f.setPointSizeF(font_sz * 0.6);
      painter->setFont(f);
      DrawNodeTitle(painter, node_shortname, safe_label_bounds, Qt::AlignBottom, icon_size, false);
    }

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
  event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

  QGraphicsRectItem::mousePressEvent(event);
}

void NodeViewItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

  QGraphicsRectItem::mouseMoveEvent(event);
}

void NodeViewItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  event->setModifiers(FlipControlAndShiftModifiers(event->modifiers()));

  QGraphicsRectItem::mouseReleaseEvent(event);
}

void NodeViewItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
  QGraphicsRectItem::mouseDoubleClickEvent(event);

  if (!(event->modifiers() & Qt::ControlModifier)) {
    SetExpanded(!IsExpanded());
  }
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
  UpdateInputConnectorFlowDirections();
  UpdateInputConnectorPositions();
  foreach (NodeViewEdge* edge, edges_) {
    edge->Adjust();
  }
}

void NodeViewItem::UpdateContextRect()
{
  if (NodeViewContext *ctx = dynamic_cast<NodeViewContext*>(parentItem())) {
    ctx->UpdateRect();
  }
}

void NodeViewItem::DrawNodeTitle(QPainter* painter, QString text, const QRectF& rect, Qt::Alignment vertical_align, int icon_size, bool draw_arrow)
{
  QFontMetrics fm = painter->fontMetrics();

  painter->setRenderHint(QPainter::SmoothPixmapTransform);

  // Draw right or down arrow based on expanded state
  int icon_padding = title_bar_rect_.height() / 2 - icon_size / 2;
  int icon_full_size = icon_size + icon_padding * 2;
  if (draw_arrow) {
    const QIcon& expand_icon = IsExpanded() ? icon::TriDown : icon::TriRight;
    int icon_size_scaled = icon_size * painter->transform().m11();
    painter->drawPixmap(QRect(title_bar_rect_.x() + icon_padding,
                              title_bar_rect_.y() + icon_padding,
                              icon_size,
                              icon_size), expand_icon.pixmap(QSize(icon_size_scaled, icon_size_scaled)));
  }

  // Calculate how much space we have for text
  int item_width = title_bar_rect_.width();
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

  // Determine the text color (automatically calculate from node background color)
  painter->setPen(ColorCoding::GetUISelectorColor(node_->color()));

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

NodeViewEdge *NodeViewItem::GetEdgeFromInputIndex(int index)
{
  foreach (NodeViewEdge *edge, edges_) {
    if (edge->input().input() == node_inputs_.at(index)) {
      return edge;
    }
  }

  return nullptr;
}

void NodeViewItem::SetHighlightedIndex(int index)
{
  if (highlighted_index_ == index) {
    return;
  }

  highlighted_index_ = index;

  update();
}

void NodeViewItem::SetLabelAsOutput(bool e)
{
  label_as_output_ = e;
  output_connector_->setVisible(!e);
  update();
}

NodeInput NodeViewItem::GetInputFromInputConnector(NodeViewItemConnector *connector)
{
  for (int i=0; i<int(input_connectors_.size()); i++) {
    if (input_connectors_[i].get() == connector) {
      return NodeInput(node_, node_inputs_.at(i));
    }
  }

  return NodeInput();
}

NodeViewEdge *NodeViewItem::GetEdgeFromInputConnector(NodeViewItemConnector *connector)
{
  int index = -1;
  for (int i=0; i<int(input_connectors_.size()); i++) {
    if (input_connectors_[i].get() == connector) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    return nullptr;
  } else {
    return GetEdgeFromInputIndex(index);
  }
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

QPointF NodeViewItem::GetInputPoint(const QString &input, int element) const
{
  int index = node_inputs_.indexOf(input);

  if (index < 0 || index >= int(input_connectors_.size())) {
    return pos();
  }

  return input_connectors_[index]->scenePos();
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
  flow_dir_ = dir;

  UpdateInputConnectorFlowDirections();
  output_connector_->SetFlowDirection(dir);

  UpdateConnectorPositions();
  UpdateNodePosition();
}

void NodeViewItem::UpdateNodePosition()
{
  setPos(NodeToScreenPoint(cached_node_pos_, flow_dir_));
}

void NodeViewItem::UpdateInputConnectors()
{
  int old_sz = input_connectors_.size();

  input_connectors_.resize(node_inputs_.size());
  for (size_t i=old_sz; i<input_connectors_.size(); i++) {
    input_connectors_[i] = std::make_unique<NodeViewItemConnector>(false, this);
  }
}

void NodeViewItem::UpdateConnectorPositions()
{
  UpdateInputConnectorPositions();

  switch (flow_dir_) {
  case NodeViewCommon::kLeftToRight:
    output_connector_->setPos(rect().right(), title_bar_rect_.center().y());
    break;
  case NodeViewCommon::kRightToLeft:
    output_connector_->setPos(rect().left(), title_bar_rect_.center().y());
    break;
  case NodeViewCommon::kTopToBottom:
    output_connector_->setPos(rect().center().x(), rect().bottom());
    break;
  case NodeViewCommon::kBottomToTop:
    output_connector_->setPos(rect().center().x(), rect().top());
    break;
  }
}

void NodeViewItem::UpdateInputConnectorPositions()
{
  QRectF output_rect = output_connector_->boundingRect();

  // Input connector flow directions change conditionally
  for (size_t i=0; i<input_connectors_.size(); i++) {
    switch (GetFlowDirectionForInput(i)) {
    case NodeViewCommon::kLeftToRight:
      input_connectors_[i]->setPos(rect().left() - output_rect.width(), GetInputRect(i).center().y());
      break;
    case NodeViewCommon::kRightToLeft:
      input_connectors_[i]->setPos(rect().right() + output_rect.width(), GetInputRect(i).center().y());
      break;
    case NodeViewCommon::kTopToBottom:
      input_connectors_[i]->setPos(rect().center().x(), rect().top() - output_rect.height());
      break;
    case NodeViewCommon::kBottomToTop:
      input_connectors_[i]->setPos(rect().center().x(), rect().bottom() + output_rect.height());
      break;
    }
  }
}

void NodeViewItem::UpdateInputConnectorFlowDirections()
{
  for (size_t i=0; i<input_connectors_.size(); i++) {
    input_connectors_[i]->SetFlowDirection(GetFlowDirectionForInput(i));
  }
}

NodeViewCommon::FlowDirection NodeViewItem::GetFlowDirectionForInput(int index)
{
  if (!expanded_ || NodeViewCommon::IsFlowHorizontal(flow_dir_)) {
    return flow_dir_;
  } else {
    NodeViewEdge *edge = GetEdgeFromInputIndex(index);

    if (!edge || edge->from_item()->x() < this->x()) {
      return NodeViewCommon::kLeftToRight;
    } else {
      return NodeViewCommon::kRightToLeft;
    }
  }
}

void NodeViewItem::NodeAppearanceChanged()
{
  update();
}

}
