#ifndef BLOCK_H
#define BLOCK_H

#include "node/node.h"

class Block : public Node
{
  Q_OBJECT
public:
  Block();

  virtual QString Category() override;

public slots:
//  virtual void Process(const rational &time) override;


};

#endif // BLOCK_H
