#ifndef SOLIDGENERATOR_H
#define SOLIDGENERATOR_H

#include "node/node.h"

class SolidGenerator : public Node
{
public:
  SolidGenerator(QObject* parent = nullptr);

  virtual void Process(const rational &time) override;

private:
  NodeOutput* texture_output_;
};

#endif // SOLIDGENERATOR_H
