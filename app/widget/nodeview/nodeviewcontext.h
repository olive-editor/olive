#ifndef NODEVIEWCONTEXT_H
#define NODEVIEWCONTEXT_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

#include "node/node.h"
#include "nodeviewcommon.h"
#include "nodeviewedge.h"

namespace olive {

class NodeViewContext : public QObject, public QGraphicsRectItem
{
  Q_OBJECT
public:
  NodeViewContext(Node *context, QGraphicsItem *item = nullptr);

  void UpdateRect();

  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  void SetCurvedEdges(bool e);

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

public slots:
  void AddChild(Node *node);

  void SetChildPosition(Node *node, const QPointF &pos);

  void RemoveChild(Node *node);

  void ChildInputConnected(Node *output, const NodeInput& input);

  bool ChildInputDisconnected(Node *output, const NodeInput& input);

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

  QMap<Node*, NodeViewItem*> item_map_;

  QVector<NodeViewEdge*> edges_;

};

}

#endif // NODEVIEWCONTEXT_H
