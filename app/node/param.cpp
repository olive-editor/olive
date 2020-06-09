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
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "node/node.h"
#include "node/input.h"
#include "node/output.h"
#include "render/color.h"

OLIVE_NAMESPACE_ENTER

NodeParam::NodeParam(const QString &id) :
  id_(id),
  connectable_(true)
{
  Q_ASSERT(!id_.isEmpty());
}

NodeParam::~NodeParam()
{
  // Clear all connected edges
  while (!edges_.isEmpty()) {
    DisconnectEdge(edges_.last());
  }
}

const QString NodeParam::id() const
{
  return id_;
}

QString NodeParam::name()
{
  if (name_.isEmpty()) {
    return tr("Value");
  }

  return name_;
}

void NodeParam::set_name(const QString &name)
{
  name_ = name;
}

Node *NodeParam::parentNode() const
{
  QObject* p = parent();

  while (p) {
    Node* cast_test = dynamic_cast<Node*>(p);
    if (cast_test) {
      return cast_test;
    } else {
      p = p->parent();
    }
  }

  return nullptr;
}

int NodeParam::index()
{
  return parentNode()->IndexOfParameter(this);
}

bool NodeParam::is_connected() const
{
  return !edges_.isEmpty();
}

bool NodeParam::is_connectable() const
{
  return connectable_;
}

void NodeParam::set_connectable(bool connectable)
{
  connectable_ = connectable;
}

const QVector<NodeEdgePtr> &NodeParam::edges()
{
  return edges_;
}

void NodeParam::DisconnectAll()
{
  while (!edges_.isEmpty()) {
    DisconnectEdge(edges_.first());
  }
}

NodeEdgePtr NodeParam::ConnectEdge(NodeOutput *output, NodeInput *input)
{
  if (!input->is_connectable()) {
    return nullptr;
  }

  // If the input can only accept one input (the default) and has one already, disconnect it
  DisconnectForNewOutput(input);

  // Make sure it's not a duplicate of an edge that already exists
  foreach (NodeEdgePtr existing, input->edges()) {
    if (existing->output() == output) {
      return nullptr;
    }
  }

  // Ensure both nodes are in the same graph
  Q_ASSERT(output->parentNode()->parent() == input->parentNode()->parent());

  NodeEdgePtr edge = std::make_shared<NodeEdge>(output, input);

  // The nodes should never be the same, and since we lock both nodes here, this can lead to a entire program freeze
  // that's difficult to diagnose. This makes that issue very clear.
  Q_ASSERT(output->parentNode() != input->parentNode());

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

  output->edges_.removeOne(edge);
  input->edges_.removeOne(edge);

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
  if (!input->edges_.isEmpty()) {
    NodeEdgePtr edge = input->edges_.first();

    DisconnectEdge(edge);

    return edge;
  }

  return nullptr;
}

QByteArray NodeParam::ValueToBytes(const NodeParam::DataType &type, const QVariant &value)
{
  switch (type) {
  case kInt: return ValueToBytesInternal<int>(value);
  case kFloat: return ValueToBytesInternal<float>(value);
  case kColor: return ValueToBytesInternal<Color>(value);
  case kText: return ValueToBytesInternal<QString>(value);
  case kBoolean: return ValueToBytesInternal<bool>(value);
  case kFont: return ValueToBytesInternal<QString>(value); // FIXME: This should probably be a QFont?
  case kFile: return ValueToBytesInternal<QString>(value);
  case kMatrix: return ValueToBytesInternal<QMatrix4x4>(value);
  case kRational: return ValueToBytesInternal<rational>(value);
  case kVec2: return ValueToBytesInternal<QVector2D>(value);
  case kVec3: return ValueToBytesInternal<QVector3D>(value);
  case kVec4: return ValueToBytesInternal<QVector4D>(value);
  case kCombo: return ValueToBytesInternal<int>(value);

  // These types have no persistent input
  case kNone:
  case kFootage:
  case kTexture:
  case kSamples:
  case kDecimal:
  case kNumber:
  case kString:
  case kBuffer:
  case kVector:
  case kAny:
    break;
  }

  return QByteArray();
}

NodeParam::DataType NodeParam::StringToDataType(const QString &s)
{
  QString type_id = s.toLower();

  if (type_id == QStringLiteral("float")) {
    return kFloat;
  } else if (type_id == QStringLiteral("int")) {
    return kInt;
  } else if (type_id == QStringLiteral("rational")) {
    return kRational;
  } else if (type_id == QStringLiteral("bool")) {
    return kBoolean;
  } else if (type_id == QStringLiteral("color")) {
    return kColor;
  } else if (type_id == QStringLiteral("matrix")) {
    return kMatrix;
  } else if (type_id == QStringLiteral("text")) {
    return kText;
  } else if (type_id == QStringLiteral("texture")) {
    return kTexture;
  } else if (type_id == QStringLiteral("vec2")) {
    return kVec2;
  } else if (type_id == QStringLiteral("vec3")) {
    return kVec3;
  } else if (type_id == QStringLiteral("vec4")) {
    return kVec4;
  } else if (type_id == QStringLiteral("combo")) {
    return kCombo;
  }

  return kAny;
}

template<typename T>
QByteArray NodeParam::ValueToBytesInternal(const QVariant &v)
{
  QByteArray bytes;

  int size_of_type = sizeof(T);

  bytes.resize(size_of_type);
  T raw_val = v.value<T>();
  memcpy(bytes.data(), &raw_val, static_cast<size_t>(size_of_type));

  return bytes;
}

OLIVE_NAMESPACE_EXIT
