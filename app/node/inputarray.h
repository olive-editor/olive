#ifndef INPUTARRAY_H
#define INPUTARRAY_H

#include "input.h"

class NodeInputArray : public NodeInput
{
public:
  NodeInputArray(const QString &id);

  int GetSize() const;
  void SetSize(int size);

  NodeInput* ParamAt(int index);

private:
  QVector<NodeInput*> sub_params_;

};

#endif // INPUTARRAY_H
