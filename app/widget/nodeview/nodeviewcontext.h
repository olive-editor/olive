#ifndef NODEVIEWCONTEXT_H
#define NODEVIEWCONTEXT_H

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>

#include "node/node.h"
#include "nodeviewcommon.h"

namespace olive {

class NodeViewContext : public QGraphicsRectItem
{
public:
  NodeViewContext(QGraphicsItem *item = nullptr);

  void SetContext(Node *node);

  void AddChild(Node *node);

  void UpdateRect();

  void SetFlowDirection(NodeViewCommon::FlowDirection dir);

  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
  virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) override;

private:
  Node *context_;

  QString lbl_;

};

}

#endif // NODEVIEWCONTEXT_H
