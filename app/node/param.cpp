/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "param.h"

#include <QDebug>

#include "node/node.h"
#include "node/input.h"
#include "node/output.h"

NodeParam::NodeParam()
{
}

const QString &NodeParam::name()
{
  return name_;
}

void NodeParam::set_name(const QString &name)
{
  name_ = name;
}

Node *NodeParam::parent()
{
  return static_cast<Node*>(QObject::parent());
}

int NodeParam::index()
{
  return parent()->IndexOfParameter(this);
}

const QVector<NodeEdgePtr> &NodeParam::edges()
{
  return edges_;
}

bool NodeParam::AreDataTypesCompatible(NodeParam *a, NodeParam *b)
{
  // Make sure one is an input and one is an output
  if (a->type() == b->type()) {
    return false;
  }

  NodeInput* input;
  NodeOutput* output;

  // Work out which parameter is which
  if (a->type() == NodeParam::kInput) {
    input = static_cast<NodeInput*>(a);
    output = static_cast<NodeOutput*>(b);
  } else {
    input = static_cast<NodeInput*>(b);
    output = static_cast<NodeOutput*>(a);
  }

  return AreDataTypesCompatible(output->data_type(), input->inputs());
}

bool NodeParam::AreDataTypesCompatible(const NodeParam::DataType &output_type, const NodeParam::DataType &input_type)
{
  if (input_type == output_type) {
    return true;
  }

  if (input_type == kNone) {
    return false;
  }

  if (input_type == kAny) {
    return true;
  }

  // Allow for up-converting integers to floats (but not the other way around)
  if (output_type == kInt && input_type == kFloat) {
    return true;
  }

  return false;
}

bool NodeParam::AreDataTypesCompatible(const DataType &output_type, const QList<DataType>& input_types)
{
  for (int i=0;i<input_types.size();i++) {
    if (AreDataTypesCompatible(output_type, input_types.at(i))) {
      return true;
    }
  }

  return false;
}

NodeEdgePtr NodeParam::ConnectEdge(NodeOutput *output, NodeInput *input)
{
  // If the input can only accept one input (the default) and has one already, disconnect it
  DisconnectForNewOutput(input);

  // Make sure it's not a duplicate of an edge that already exists
  foreach (NodeEdgePtr existing, input->edges()) {
    if (existing->output() == output) {
      return nullptr;
    }
  }

  NodeEdgePtr edge = std::make_shared<NodeEdge>(output, input);

  output->edges_.append(edge);
  input->edges_.append(edge);

  // Emit a signal than an edge was added (only one signal needs emitting)
  emit input->EdgeAdded(edge);

  return edge;
}

void NodeParam::DisconnectEdge(NodeEdgePtr edge)
{
  NodeOutput* output = edge->output();
  NodeInput* input = edge->input();

  output->edges_.removeAll(edge);
  input->edges_.removeAll(edge);

  emit input->EdgeRemoved(edge);
}

void NodeParam::DisconnectEdge(NodeOutput *output, NodeInput *input)
{
  for (int i=0;i<output->edges_.size();i++) {
    NodeEdgePtr edge = output->edges_.at(i);
    if (edge->input() == input) {
      DisconnectEdge(edge);
      break;
    }
  }
}

NodeEdgePtr NodeParam::DisconnectForNewOutput(NodeInput *input)
{
  // If the input can only accept one input (the default) and has one already, disconnect it
  if (!input->edges_.isEmpty() && !input->can_accept_multiple_inputs()) {
    NodeEdgePtr edge = input->edges_.first();

    DisconnectEdge(edge);

    return edge;
  }

  return nullptr;
}

QString NodeParam::GetDefaultDataTypeName(const DataType& type)
{
  switch (type) {
  case kNone: return tr("None");
  case kInt: return tr("Integer");
  case kFloat: return tr("Float");
  case kColor: return tr("Color");
  case kString: return tr("String");
  case kBoolean: return tr("Boolean");
  case kFont: return tr("Font");
  case kFile: return tr("File");
  case kTexture: return tr("Texture");
  case kMatrix: return tr("Matrix");
  case kBlock: return tr("Block");
  case kFootage: return tr("Footage");
  case kAny: return tr("Any");
  }

  return QString();
}
