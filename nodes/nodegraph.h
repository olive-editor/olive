#ifndef NODEGRAPH_H
#define NODEGRAPH_H

#include "effects/effect.h"

class NodeGraph
{
public:
  NodeGraph();

  /**
   * @brief Add a node to this graph
   *
   * The graph takes ownership of the node.
   *
   * @param node
   */
  void AddNode(EffectPtr node);

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
  void SetOutputNode(EffectPtr node);

  /**
   * @brief Returns the currently set output node
   */
  Effect* OutputNode();

private:  
  EffectPtr output_node_;
};

#endif // NODEGRAPH_H
