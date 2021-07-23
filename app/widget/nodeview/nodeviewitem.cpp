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

NodeViewItem::NodeViewItem(QGraphicsItem *parent) :
  QGraphicsRectItem(parent),
  node_(nullptr),
  flow_dir_(NodeViewCommon::kLeftToRight),
  prevent_removing_(false)
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

  setRect(QRectF(-widget_width/2, -widget_height/2, widget_width, widget_height));
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

void NodeViewItem::SetNode(Node *n)
{
  if (node_) {
    disconnect(node_, &Node::InputAdded, this, &NodeViewItem::InputAdded);
    disconnect(node_, &Node::InputRemoved, this, &NodeViewItem::InputRemoved);
    disconnect(node_, &Node::OutputAdded, this, &NodeViewItem::OutputAdded);
    disconnect(node_, &Node::OutputRemoved, this, &NodeViewItem::OutputRemoved);

    // Clear connector objects
    qDeleteAll(input_connectors_);
    input_connectors_.clear();
    qDeleteAll(output_connectors_);
    output_connectors_.clear();
  }

  node_ = n;

  if (node_) {
    connect(node_, &Node::InputAdded, this, &NodeViewItem::InputAdded);
    connect(node_, &Node::InputRemoved, this, &NodeViewItem::InputRemoved);
    connect(node_, &Node::OutputAdded, this, &NodeViewItem::OutputAdded);
    connect(node_, &Node::OutputRemoved, this, &NodeViewItem::OutputRemoved);

    for (const QString &input : node_->inputs()) {
      InputAdded(input);
    }

    for (const QString &output : node_->outputs()) {
      OutputAdded(output);
    }
  }

  update();
}

void NodeViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *)
{
  // Use main window palette since the palette passed in `widget` is the NodeView palette which
  // has been slightly modified
  QPalette app_pal = Core::instance()->main_window()->palette();

  // Draw the titlebar
  if (node_) {
    painter->setPen(Qt::black);
    painter->setBrush(node_->brush(rect().top(), rect().bottom()));

    painter->drawRect(rect());

    painter->setPen(app_pal.color(QPalette::Text));

    QString node_label = node_->GetLabel();
    QString node_shortname = node_->ShortName();

    if (node_label.isEmpty()) {
      // If this is a track and has no user-supplied label, generate an automatic one
      Track* track_cast_test = dynamic_cast<Track*>(node_);
      if (track_cast_test) {
        node_label = Track::GetDefaultTrackName(track_cast_test->type(), track_cast_test->Index());
      }
    }

    int icon_size = painter->fontMetrics().height()/2;

    if (node_label.isEmpty()) {
      // Draw shortname only
      DrawNodeTitle(painter, node_shortname, rect(), Qt::AlignVCenter, icon_size, true);
    } else {
      int text_pad = DefaultTextPadding()/2;
      QRectF safe_label_bounds = rect().adjusted(text_pad, text_pad, -text_pad, -text_pad);
      QFont f;
      qreal font_sz = f.pointSizeF();
      f.setPointSizeF(font_sz * 0.8);
      painter->setFont(f);
      DrawNodeTitle(painter, node_label, safe_label_bounds, Qt::AlignTop, icon_size, true);
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
}

QVariant NodeViewItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionHasChanged && node_) {
    ReadjustAllEdges();
  }

  return QGraphicsItem::itemChange(change, value);
}

void NodeViewItem::ReadjustAllEdges()
{
  foreach (NodeViewEdge* edge, edges_) {
    edge->Adjust();
  }
}

void NodeViewItem::DrawNodeTitle(QPainter* painter, QString text, const QRectF& rect, Qt::Alignment vertical_align, int icon_size, bool draw_arrow)
{
  QFontMetrics fm = painter->fontMetrics();

  painter->setRenderHint(QPainter::SmoothPixmapTransform);

  // Draw right or down arrow based on expanded state
  int icon_padding = this->rect().height() / 2 - icon_size / 2;
  int icon_full_size = icon_size + icon_padding * 2;
  /*if (draw_arrow) {
    const QIcon& expand_icon = IsExpanded() ? icon::TriDown : icon::TriRight;
    int icon_size_scaled = icon_size * painter->transform().m11();
    painter->drawPixmap(QRect(rect().x() + icon_padding,
                              rect().y() + icon_padding,
                              icon_size,
                              icon_size), expand_icon.pixmap(QSize(icon_size_scaled, icon_size_scaled)));
  }*/

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

QPointF NodeViewItem::GetInputPoint(const QString &input, int element) const
{
  for (NodeViewConnector *connector : input_connectors_) {
    if (connector->GetParam() == input) {
      return pos() + connector->pos() - QPointF(0, connector->boundingRect().height());
    }
  }

  return QPointF();
}

QPointF NodeViewItem::GetOutputPoint(const QString& output) const
{
  for (NodeViewConnector *connector : output_connectors_) {
    if (connector->GetParam() == output) {
      return pos() + connector->pos();
    }
  }

  return QPointF();
}

void NodeViewItem::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  flow_dir_ = dir;

  for (NodeViewConnector *connector : qAsConst(input_connectors_)) {
    connector->SetFlowDirection(flow_dir_);
  }
  UpdateInputPositions();

  for (NodeViewConnector *connector : qAsConst(output_connectors_)) {
    connector->SetFlowDirection(flow_dir_);
  }
  UpdateOutputPositions();

  UpdateNodePosition();
}

void NodeViewItem::UpdateNodePosition()
{
  setPos(NodeToScreenPoint(cached_node_pos_, flow_dir_));
}

void NodeViewItem::UpdateInputPositions()
{
  UpdateConnectorPositions(&input_connectors_, true);
}

void NodeViewItem::UpdateOutputPositions()
{
  UpdateConnectorPositions(&output_connectors_, false);
}

void NodeViewItem::AddConnector(QVector<NodeViewConnector *> *connectors, const QString &id, int index, bool is_input)
{
  // Ignore if we already have this input
  for (NodeViewConnector *connector : qAsConst(*connectors)) {
    if (connector->GetParam() == id) {
      return;
    }
  }

  // We don't have this connector yet, create it now
  NodeViewConnector *connector = new NodeViewConnector(this);

  connector->SetFlowDirection(flow_dir_);
  connector->SetParam(node_, id, is_input);

  if (index == -1) {
    connectors->append(connector);
  } else {
    connectors->insert(index, connector);
  }
}

void NodeViewItem::RemoveConnector(QVector<NodeViewConnector *> *connectors, const QString &id)
{
  for (int i=0; i<connectors->size(); i++) {
    if (connectors->at(i)->GetParam() == id) {
      delete connectors->takeAt(i);
      break;
    }
  }
}

void NodeViewItem::UpdateConnectorPositions(QVector<NodeViewConnector *> *connectors, bool top)
{
  for (int i=0; i<connectors->size(); i++) {
    NodeViewConnector *connector = connectors->at(i);
    qreal x = rect().left() + double(i+1)/double(connectors->size()+1) * rect().width();
    qreal y = top ? rect().top() : rect().bottom() + connector->boundingRect().height();
    connector->setPos(x, y);
  }
}

void NodeViewItem::InputAdded(const QString &id, int index)
{
  // Ignore any input that isn't connectable
  if (!node_->IsInputConnectable(id)) {
    return;
  }

  AddConnector(&input_connectors_, id, index, true);

  UpdateInputPositions();
}

void NodeViewItem::InputRemoved(const QString &id)
{
  RemoveConnector(&input_connectors_, id);

  UpdateInputPositions();
}

void NodeViewItem::OutputAdded(const QString &id, int index)
{
  AddConnector(&output_connectors_, id, index, false);

  UpdateOutputPositions();
}

void NodeViewItem::OutputRemoved(const QString &id)
{
  RemoveConnector(&output_connectors_, id);

  UpdateOutputPositions();
}

}
