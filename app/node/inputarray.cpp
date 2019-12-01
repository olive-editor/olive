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

void NodeInputArray::Prepend()
{
  InsertAt(0);
}

void NodeInputArray::SetSize(int size)
{
  int old_size = GetSize();

  if (size == old_size) {
    return;
  }

  if (size < old_size) {
    // If the new size is less, delete all extraneous parameters
    for (int i=size;i<old_size;i++) {
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

  emit SizeChanged(size);
}

int NodeInputArray::IndexOfSubParameter(NodeInput *input) const
{
  return sub_params_.indexOf(input);
}

NodeInput *NodeInputArray::ParamAt(int index) const
{
  return sub_params_.at(index);
}

void NodeInputArray::InsertAt(int index)
{
  qDebug() << "Inserted at";

  // Add another input at the end
  Append();

  // Shift all connections from index down
  for (int i=sub_params_.size()-1;i>index;i--) {
    NodeInput* this_param = sub_params_.at(i);
    NodeInput* prev_param = sub_params_.at(i-1);

    if (this_param->IsConnected()) {
      // Disconnect whatever is at this parameter (presumably its connection has already been copied so we can just remove it)
      NodeParam::DisconnectEdge(this_param->edges().first());
    }

    if (prev_param->IsConnected()) {
      // Get edge here (only one since it's an input)
      NodeEdgePtr edge = prev_param->edges().first();

      // Disconnect it
      NodeParam::DisconnectEdge(edge);

      // Create a new edge between it and the next one down
      NodeParam::ConnectEdge(edge->output(),
                             this_param);
    }
  }
}

void NodeInputArray::Append()
{
  qDebug() << "Appended";

  SetSize(GetSize() + 1);
}

void NodeInputArray::RemoveLast()
{
  qDebug() << "Removed last";

  SetSize(GetSize() - 1);
}

void NodeInputArray::RemoveAt(int index)
{
  qDebug() << "Removed at";

  int limit = sub_params_.size() - 1;

  // Shift all connections from index down
  for (int i=index;i<limit;i++) {
    NodeInput* this_param = sub_params_.at(i);
    NodeInput* next_param = sub_params_.at(i + 1);

    if (this_param->IsConnected()) {
      // Disconnect current edge
      NodeParam::DisconnectEdge(this_param->edges().first());
    }

    if (next_param->IsConnected()) {
      // Get edge from next param
      NodeEdgePtr edge = next_param->edges().first();

      // Disconnect it
      NodeParam::DisconnectEdge(edge);

      // Reconnect it to this param
      NodeParam::ConnectEdge(edge->output(),
                             this_param);
    }
  }

  RemoveLast();
}
