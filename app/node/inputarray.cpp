#include "inputarray.h"

#include "node.h"

NodeInputArray::NodeInputArray(const QString &id) :
  NodeInput(id)
{

}

int NodeInputArray::GetSize() const
{
  return sub_params_.size();
}

void NodeInputArray::SetSize(int size)
{
  int old_size = GetSize();

  if (size == old_size) {
    return;
  }

  if (size < old_size) {
    // If the new size is less, delete all extraneous parameters
    for (int i=size;i<sub_params_.size();i++) {
      delete sub_params_.at(i);
    }
  }

  sub_params_.resize(size);

  if (size > old_size) {
    // If the new size is greater, create valid parameters for each slot
    for (int i=old_size;i<size;i++) {
      QString sub_id = id();
      sub_id.append(QString::number(i));

      Q_ASSERT(!parentNode()->HasParamWithID(sub_id));

      NodeInput* new_param = new NodeInput(sub_id);
      new_param->setParent(this);
      sub_params_.replace(i, new_param);

      connect(new_param, SIGNAL(ValueChanged(rational, rational)), this, SIGNAL(ValueChanged(rational, rational)));
      connect(new_param, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SIGNAL(EdgeAdded(NodeEdgePtr)));
      connect(new_param, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SIGNAL(EdgeRemoved(NodeEdgePtr)));
    }
  }
}

NodeInput *NodeInputArray::ParamAt(int index)
{
  return sub_params_.at(index);
}
