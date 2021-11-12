#ifndef NODEVIEWCONTEXT_H
#define NODEVIEWCONTEXT_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

#include "node/node.h"
#include "nodeviewcommon.h"
#include "nodeviewedge.h"

namespace olive {

class NodeViewContext : public QGraphicsRectItem
{
public:
  NodeViewContext(QGraphicsItem *item = nullptr);

  void SetContext(Node *node);

  void AddChild(Node *node);

  void UpdateRect();

  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  void SetCurvedEdges(bool e);

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
  virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
  NodeViewEdge *AddEdgeInternal(Node *output, const NodeInput& input, NodeViewItem *from, NodeViewItem *to);

  Node *context_;

  QString lbl_;

  NodeViewCommon::FlowDirection flow_dir_;

  bool curved_edges_;

  int last_titlebar_height_;

};

}

#endif // NODEVIEWCONTEXT_H
