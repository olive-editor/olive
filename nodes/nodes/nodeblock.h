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

  rational track_in();
  rational track_out();

  const rational& length();
  void set_length(const rational& l);

  virtual QString name() override;
  virtual QString id() override;

  /**
   * @brief Retrieves the NodeBlock connected to the "Previous" parameter
   */
  NodeBlock* previous_block();
  void set_previous_block(NodeBlock* p);

  /**
   * @brief Retrieves the NodeBlock connected to the "Next" parameter
   */
  NodeBlock* next_block();
  void set_next_block(NodeBlock* p);



private:
  rational length_;

  NodeIO* previous_block_;
  NodeIO* next_block_;
};

#endif // NODEBLOCK_H
