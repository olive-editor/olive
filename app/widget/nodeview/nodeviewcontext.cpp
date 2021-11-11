#include "nodeviewcontext.h"

#include <QBrush>
#include <QCoreApplication>
#include <QGraphicsScene>
#include <QPen>
#include <QStyleOptionGraphicsItem>

#include "node/block/block.h"
#include "node/output/track/track.h"
#include "nodeviewitem.h"
#include "ui/colorcoding.h"

namespace olive {

#define super QGraphicsRectItem

NodeViewContext::NodeViewContext(QGraphicsItem *item) :
  super(item)
{
  setFlag(ItemIsMovable);
  setFlag(ItemIsSelectable);

  // Set default label text
  SetContext(nullptr);
}

void NodeViewContext::SetContext(Node *node)
{
  context_ = node;

  if (context_) {
    if (Block *block = dynamic_cast<Block*>(node)) {
      lbl_ = QCoreApplication::translate("NodeViewContext",
                                                "%1 [%2] :: %3 - %4").arg(block->GetLabelAndName(),
                                                                     Track::Reference::TypeToTranslatedString(block->track()->type()),
                                                                     block->in().toString(),
                                                                     block->out().toString());
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
  NodeViewItem *item = new NodeViewItem(this);
  item->SetNode(node);
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

  QRectF rect = childrenBoundingRect();
  int pad = NodeViewItem::DefaultItemHeight();
  rect.adjust(-pad, - lbl_offset*2 - fm.height() - pad, pad, pad);
  setRect(rect);
}

void NodeViewContext::SetFlowDirection(NodeViewCommon::FlowDirection dir)
{
  auto children = childItems();
  foreach (auto child, children) {
    if (NodeViewItem *item = dynamic_cast<NodeViewItem*>(child)) {
      item->SetFlowDirection(dir);
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

  painter->setPen(context_ ? ColorCoding::GetUISelectorColor(context_->color()) : Qt::white);

  int offset = GetTextOffset(painter->fontMetrics());

  QRectF text_rect = rect();
  text_rect.adjust(offset, offset, -offset, -offset);
  painter->drawText(text_rect, lbl_);
}

QVariant NodeViewContext::itemChange(GraphicsItemChange change, const QVariant &value)
{
  return super::itemChange(change, value);
}

}
