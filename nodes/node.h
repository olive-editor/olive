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

  void AddParameter(NodeIO* row);
  int IndexOfParameter(NodeIO* row);
  NodeIO* Parameter(int i);
  int ParameterCount();

private:
  QVector<NodeIO*> parameters_;
};

#endif // NODE_H
