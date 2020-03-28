#ifndef MATHNODE_H
#define MATHNODE_H

#include "node/node.h"

class MathNode : public Node
{
public:
  MathNode();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

};

#endif // MATHNODE_H
