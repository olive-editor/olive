#ifndef NODEBLOCK_H
#define NODEBLOCK_H

#include "global/rational.h"
#include "nodes/node.h"

/**
 * @brief The NodeBlock class
 *
 * A "Block" node is a node with a timed in-point and out-point that can appear on the Timeline.
 */
class NodeBlock : public Node
{
  Q_OBJECT
public:
  NodeBlock(NodeGraph* graph);

  const olive::rational& track_in();
  const olive::rational& track_out();

  const olive::rational& length();
  void set_length(const olive::rational& l);

  virtual QString name() override;
  virtual QString id() override;

  NodeBlock* previous_block();
  NodeBlock* next_block();

private:
  olive::rational length_;

  NodeIO* previous_block_;
  NodeIO* next_block_;
};

#endif // NODEBLOCK_H
