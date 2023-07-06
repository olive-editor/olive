#ifndef NODEVIEWCONTEXT_H
#define NODEVIEWCONTEXT_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

#include "node/node.h"
#include "node/nodeundo.h"
#include "nodeviewcommon.h"
#include "nodeviewedge.h"

namespace olive {

class NodeViewContext : public QObject, public QGraphicsRectItem
{
  Q_OBJECT
public:
  NodeViewContext(Node *context, QGraphicsItem *item = nullptr);

  virtual ~NodeViewContext() override;

  Node *GetContext() const
  {
    return context_;
  }

  void UpdateRect();

  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  void SetCurvedEdges(bool e);

  int DeleteSelected(NodeViewDeleteCommand *command);

  void Select(const QVector<Node*> &nodes);

  QVector<NodeViewItem*> GetSelectedItems() const;

  QPointF MapScenePosToNodePosInContext(const QPointF &pos) const;

  NodeViewItem *GetItemFromMap(Node *node) const
  {
    return item_map_.value(node);
  }

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

public slots:
  void AddChild(Node *node);

  void SetChildPosition(Node *node, const QPointF &pos);

  void RemoveChild(Node *node);

  void ChildInputConnected(const NodeOutput &output, const NodeInput& input);

  bool ChildInputDisconnected(const NodeOutput &output, const NodeInput& input);

signals:
  void ItemAboutToBeDeleted(NodeViewItem *item);

protected:
  virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
  void AddNodeInternal(Node *node, NodeViewItem *item);

  void AddEdgeInternal(const NodeOutput &output, const NodeInput& input, NodeViewItem *from, NodeViewItem *to);

  Node *context_;

  QString lbl_;

  NodeViewCommon::FlowDirection flow_dir_;

  bool curved_edges_;

  int last_titlebar_height_;

  QMap<Node*, NodeViewItem*> item_map_;

  QVector<NodeViewEdge*> edges_;

private slots:
  void GroupAddedNode(Node *node);

  void GroupRemovedNode(Node *node);

};

}

#endif // NODEVIEWCONTEXT_H
