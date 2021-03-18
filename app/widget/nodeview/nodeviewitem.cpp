/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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
  expanded_(false),
  hide_titlebar_(false),
  highlighted_index_(-1),
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
  return QtUtils::QFontMetricsWidth(QFontMetrics(QFont()), "HHHHHHHHHHHH");;
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
  pt -= pos();

  for (int i=0; i<node_inputs_.size(); i++) {
    if (GetInputRect(i).contains(pt)) {
      return i;
    }
  }

  return -1;
}

void NodeViewItem::SetNode(Node *n)
{
  node_ = n;

  node_inputs_.clear();

  if (node_) {
    node_->Retranslate();

    foreach (const QString& input, node_->inputs()) {
      if (node_->IsInputConnectable(input)) {
        node_inputs_.append(input);
      }
    }

    SetNodePosition(node_->GetPosition());
  }

  update();
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

  ReadjustAllEdges();
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

    QString node_label = node_->GetLabel();
    QString node_shortname = node_->ShortName();

    if (node_label.isEmpty()) {
      // If this is a track and has no user-supplied label, generate an automatic one
      Track* track_cast_test = dynamic_cast<Track*>(node_);
      if (track_cast_test) {
        node_label = Track::GetDefaultTrackName(track_cast_test->type(), track_cast_test->Index());
      }
    }

    if (node_label.isEmpty()) {
      // Draw shortname only
      DrawNodeTitle(painter, node_shortname, title_bar_rect_, Qt::AlignVCenter);
    } else {
      int text_pad = DefaultTextPadding()/2;
      QRectF safe_label_bounds = title_bar_rect_.adjusted(text_pad, text_pad, -text_pad, -text_pad);
      QFont f;
      qreal font_sz = f.pointSizeF();
      f.setPointSizeF(font_sz * 0.8);
      painter->setFont(f);
      DrawNodeTitle(painter, node_label, safe_label_bounds, Qt::AlignTop);
      f.setPointSizeF(font_sz * 0.6);
      painter->setFont(f);
      DrawNodeTitle(painter, node_shortname, safe_label_bounds, Qt::AlignBottom);
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
    node_->blockSignals(true);
    node_->SetPosition(GetNodePosition());
    node_->blockSignals(false);

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

void NodeViewItem::DrawNodeTitle(QPainter* painter, QString text, const QRectF& rect, Qt::Alignment vertical_align)
{
  QFontMetrics fm = painter->fontMetrics();

  // Draw right or down arrow based on expanded state
  int icon_size = fm.height() / 2;
  int icon_padding = title_bar_rect_.height() / 2 - icon_size / 2;
  int icon_full_size = icon_size + icon_padding * 2;
  const QIcon& expand_icon = IsExpanded() ? icon::TriDown : icon::TriRight;
  expand_icon.paint(painter, QRect(title_bar_rect_.x() + icon_padding,
                                   title_bar_rect_.y() + icon_padding,
                                   icon_size,
                                   icon_size));

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

QPointF NodeViewItem::GetInputPoint(const QString &input, int element, const QPointF& source_pos) const
{
  return pos() + GetInputPointInternal(node_inputs_.indexOf(input), source_pos);
}

QPointF NodeViewItem::GetOutputPoint(const QString& output) const
{
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
}

void NodeViewItem::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  flow_dir_ = dir;
}

QPointF NodeViewItem::GetInputPointInternal(int index, const QPointF& source_pos) const
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

}
