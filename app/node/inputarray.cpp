#include "inputarray.h"

#include <QApplication>

#include "common/xmlutils.h"
#include "node.h"

NodeInputArray::NodeInputArray(const QString &id, const DataType &type, const QVariant &default_value) :
  NodeInput(id, type, default_value),
  default_value_(default_value)
{
}

bool NodeInputArray::IsArray()
{
  return true;
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
      NodeInput* sub_param = sub_params_.at(i);
      delete sub_param;
    }
  }

  sub_params_.resize(size);

  if (size > old_size) {
    // If the new size is greater, create valid parameters for each slot
    for (int i=old_size;i<size;i++) {
      QString sub_id = id();
      sub_id.append(QString::number(i));

      Q_ASSERT(!parentNode()->HasParamWithID(sub_id));

      NodeInput* new_param = new NodeInput(sub_id, data_type(), default_value_);
      new_param->setParent(this);
      sub_params_.replace(i, new_param);

      connect(new_param, &NodeInput::ValueChanged, this, &NodeInput::ValueChanged);
      connect(new_param, &NodeInput::EdgeAdded, this, &NodeInput::EdgeAdded);
      connect(new_param, &NodeInput::EdgeRemoved, this, &NodeInput::EdgeRemoved);
    }
  }

  emit SizeChanged(size);
}

bool NodeInputArray::ContainsSubParameter(NodeInput *input) const
{
  return sub_params_.contains(input);
}

int NodeInputArray::IndexOfSubParameter(NodeInput *input) const
{
  return sub_params_.indexOf(input);
}

NodeInput *NodeInputArray::At(int index) const
{
  return sub_params_.at(index);
}

NodeInput *NodeInputArray::First() const
{
  return sub_params_.first();
}

NodeInput *NodeInputArray::Last() const
{
  return sub_params_.last();
}

const QVector<NodeInput *> &NodeInputArray::sub_params()
{
  return sub_params_;
}

void NodeInputArray::InsertAt(int index)
{
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
  SetSize(GetSize() + 1);
}

void NodeInputArray::RemoveLast()
{
  SetSize(GetSize() - 1);
}

void NodeInputArray::RemoveAt(int index)
{
  // Shift all connections from index down
  for (int i=index;i<sub_params_.size();i++) {
    NodeInput* this_param = sub_params_.at(i);

    if (this_param->IsConnected()) {
      // Disconnect current edge
      NodeParam::DisconnectEdge(this_param->edges().first());
    }

    if (i < sub_params_.size() - 1) {
      NodeInput* next_param = sub_params_.at(i + 1);
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
  }

  RemoveLast();
}

void NodeInputArray::LoadInternal(QXmlStreamReader *reader, QHash<quintptr, NodeOutput*>& param_ptrs, QList<SerializedConnection> &input_connections, QList<FootageConnection>& footage_connections, const QAtomicInt* cancelled)
{
  if (reader->name() == QStringLiteral("subparameters")) {
    while (XMLReadNextStartElement(reader)) {
      if (reader->name() == QStringLiteral("input")) {
        Append();
        At(GetSize() - 1)->Load(reader, param_ptrs, input_connections, footage_connections, cancelled);
      } else {
        reader->skipCurrentElement();
      }
    }
  } else {
    NodeInput::Load(reader, param_ptrs, input_connections, footage_connections, cancelled);
  }
}

void NodeInputArray::SaveInternal(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("subparameters");

  foreach (NodeInput* sub, sub_params_) {
    sub->Save(writer);
  }

  writer->writeEndElement(); // subparameters
}
