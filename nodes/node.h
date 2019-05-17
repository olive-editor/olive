#ifndef NODE_H
#define NODE_H

#include <memory>

#include "nodes/nodeio.h"

class NodeGraph;

class Node : public QObject
{
  Q_OBJECT
public:
  Node(NodeGraph* parent);

  virtual QString name();
  virtual QString id();

  void AddParameter(NodeIO* row);
  int IndexOfParameter(NodeIO* row);
  NodeIO* Parameter(int i);
  int ParameterCount();

  NodeGraph* ParentGraph();
  double Time();

  const QPointF& pos();

public slots:
  void SetPos(const QPointF& pos);

private:
  QVector<NodeIO*> parameters_;
  QPointF pos_;
};

#endif // NODE_H
