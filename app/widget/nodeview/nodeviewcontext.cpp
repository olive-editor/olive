#include "nodeviewcontext.h"

#include <QBrush>
#include <QCoreApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QStyleOptionGraphicsItem>

#include "core.h"
#include "node/block/block.h"
#include "node/graph.h"
#include "node/output/track/track.h"
#include "node/project/sequence/sequence.h"
#include "nodeviewitem.h"
#include "ui/colorcoding.h"

namespace olive {

#define super QGraphicsRectItem

NodeViewContext::NodeViewContext(QGraphicsItem *item) :
  super(item)
{
  // Set default label text
  SetContext(nullptr);
}

void NodeViewContext::SetContext(Node *node)
{
  context_ = node;

  if (context_) {
    if (Block *block = dynamic_cast<Block*>(node)) {
      rational timebase = block->track()->sequence()->GetVideoParams().frame_rate_as_time_base();
      lbl_ = QCoreApplication::translate("NodeViewContext",
                                         "%1 [%2] :: %3 - %4").arg(block->GetLabelAndName(),
                                                                   Track::Reference::TypeToTranslatedString(block->track()->type()),
                                                                   Timecode::time_to_timecode(block->in(), timebase, Core::instance()->GetTimecodeDisplay()),
                                                                   Timecode::time_to_timecode(block->out(), timebase, Core::instance()->GetTimecodeDisplay()));
    } else {
      lbl_ = node->GetLabelAndName();
    }

    Color c = node->color();
    setPen(QPen(c.toQColor(), 2));

    c.set_alpha(0.5f);
    setBrush(c.toQColor());
  } else {
    lbl_ = QCoreApplication::translate("NodeViewContext", "(None)");
  }
}

void NodeViewContext::AddChild(Node *node)
{
  if (!context_) {
    return;
  }

  NodeViewItem *item = new NodeViewItem(this);
  item->SetNode(node);
  item->SetNodePosition(context_->parent()->GetNodesForContext(context_).value(node));
  item->SetFlowDirection(flow_dir_);

  if (node == context_) {
    item->SetLabelAsOutput(true);
  }

  for (auto it=node->output_connections().cbegin(); it!=node->output_connections().cend(); it++) {
    foreach (auto child, childItems()) {
      if (NodeViewItem *other_item = dynamic_cast<NodeViewItem*>(child)) {
        if (other_item->GetNode() == it->second.node()) {
          AddEdgeInternal(node, it->second, item, other_item);
        }
      }
    }
  }

  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    foreach (auto child, childItems()) {
      if (NodeViewItem *other_item = dynamic_cast<NodeViewItem*>(child)) {
        if (it->second == other_item->GetNode()) {
          AddEdgeInternal(it->second, it->first, other_item, item);
        }
      }
    }
  }
}

qreal GetTextOffset(const QFontMetricsF &fm)
{
  return fm.height()/2;
}

void NodeViewContext::UpdateRect()
{
  QFont f;
  QFontMetricsF fm(f);
  qreal lbl_offset = GetTextOffset(fm);

  QRectF cbr = childrenBoundingRect();
  QRectF rect = cbr;
  int pad = NodeViewItem::DefaultItemHeight();
  rect.adjust(-pad, - lbl_offset*2 - fm.height() - pad, pad, pad);
  setRect(rect);

  last_titlebar_height_ = rect.y() + (cbr.y() - rect.y());
}

void NodeViewContext::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  flow_dir_ = dir;

  foreach (auto child, childItems()) {
    if (NodeViewItem *item = dynamic_cast<NodeViewItem*>(child)) {
      item->SetFlowDirection(dir);
    }
  }

  foreach (auto child, childItems()) {
    if (NodeViewEdge *edge = dynamic_cast<NodeViewEdge*>(child)) {
      edge->SetFlowDirection(dir);
    }
  }
}

void NodeViewContext::SetCurvedEdges(bool e)
{
  curved_edges_ = e;

  const QList<QGraphicsItem*> &children = childItems();
  foreach (auto child, children) {
    if (NodeViewEdge *edge = dynamic_cast<NodeViewEdge*>(child)) {
      edge->SetCurved(e);
    }
  }
}

void NodeViewContext::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  QPen p = pen();

  if (option->state & QStyle::State_Selected) {
    p.setStyle(Qt::DotLine);
  }

  painter->setPen(p);
  painter->setBrush(brush());

  int rounded = painter->fontMetrics().height();
  painter->drawRoundedRect(rect(), rounded, rounded);

  painter->setPen(widget->palette().text().color());

  int offset = GetTextOffset(painter->fontMetrics());

  QRectF text_rect = rect();
  text_rect.adjust(offset, offset, -offset, -offset);
  painter->drawText(text_rect, lbl_);
}

QVariant NodeViewContext::itemChange(GraphicsItemChange change, const QVariant &value)
{
  return super::itemChange(change, value);
}

void NodeViewContext::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  bool clicked_inside_titlebar = (event->pos().y() < last_titlebar_height_);

  setFlag(ItemIsMovable, clicked_inside_titlebar);
  setFlag(ItemIsSelectable, clicked_inside_titlebar);

  super::mousePressEvent(event);
}

NodeViewEdge* NodeViewContext::AddEdgeInternal(Node *output, const NodeInput& input, NodeViewItem *from, NodeViewItem *to)
{
  NodeViewEdge* edge_ui = new NodeViewEdge(output, input, from, to, this);

  edge_ui->SetFlowDirection(flow_dir_);
  edge_ui->SetCurved(curved_edges_);

  from->AddEdge(edge_ui);
  to->AddEdge(edge_ui);

  return edge_ui;
}

}
