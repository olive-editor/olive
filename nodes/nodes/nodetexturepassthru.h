#ifndef NODEIMAGEOUTPUT_H
#define NODEIMAGEOUTPUT_H

#include "nodes/node.h"

class NodeTexturePassthru : public Node
{
public:
  NodeTexturePassthru(NodeGraph* c);

  virtual QString name() override;
  virtual QString id() override;

  virtual void Process(const rational& time) override;

  NodeIO* texture_input();
  NodeIO* texture_output();

private:
  NodeIO* texture_input_;
  NodeIO* texture_output_;
};

#endif // NODEIMAGEOUTPUT_H
