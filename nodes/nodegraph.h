#ifndef NODEGRAPH_H
#define NODEGRAPH_H

#include <QVector>
#include <QObject>

#include "nodes/node.h"

class NodeGraph : public QObject
{
  Q_OBJECT
public:
  NodeGraph();

  /**
   * @brief Process the graph
   *
   * Using the output node set by SetOutputNode(), this function will work backwards and perform every action in the
   * node hierarchy necessary to eventually process the output node. After this function returns, the output node should
   * contain valid values ready for accessing.
   *
   * Each node will cache its output for optimized playback and rendering. This will happen transparently and under most
   * circumstances, this function will be able to return immediately.
   */
  void Process();

  /**
   * @brief Set the output node for this node graph
   *
   * This node will be presented as the final node that all other nodes converge on. This node can be used to retrieve
   * the result of the graph.
   *
   * @param node
   *
   * The node to set as the output node. The graph takes ownership of the node and the user cannot delete it.
   */
  void SetOutputNode(Node* node);

  /**
   * @brief Returns the currently set output node
   */
  Node* OutputNode();

signals:
  void NodeGraphChanged();

protected:
  virtual void childEvent(QChildEvent *event) override;

private:  
  Node* output_node_;
};

#endif // NODEGRAPH_H
