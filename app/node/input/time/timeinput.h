#ifndef TIMEINPUT_H
#define TIMEINPUT_H

#include "node/node.h"

class TimeInput : public Node
{
    Q_OBJECT
public:
  TimeInput();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QString Category() const override;
  virtual QString Description() const override;

  virtual NodeValueTable Value(const NodeValueDatabase& value) const override;

};

#endif // TIMEINPUT_H
