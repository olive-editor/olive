#ifndef NODE_H
#define NODE_H

#include "nodes/nodeio.h"
#include "global/rational.h"

class NodeGraph;

class Node : public QObject
{
  Q_OBJECT
public:
  Node(NodeGraph* parent);

  virtual QString name();
  virtual QString id();

  virtual void Process(rational time);

  void AddParameter(NodeIO* row);
  int IndexOfParameter(NodeIO* row);
  NodeIO* Parameter(int i);
  int ParameterCount();

  NodeGraph* ParentGraph();

  const QPointF& pos();

protected:
  virtual void GLContextChangeEvent();

public slots:
  void SetPos(const QPointF& pos);

private:
  QVector<NodeIO*> parameters_;
  QPointF pos_;
};

#endif // NODE_H
