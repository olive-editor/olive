#ifndef NODE_H
#define NODE_H

#include <QObject>
#include <QVector>

class NodeInput {
public:
  NodeInput();

private:
  QString name_;

};

class Node : public QObject
{
  Q_OBJECT
public:
  Node();

private:
  int max_inputs_;
  QVector<Node*> inputs_;

  int max_outputs_;
  QVector<Node*> outputs_;

};

#endif // NODE_H
