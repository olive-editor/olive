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

#include "node.h"

#include <QApplication>
#include <QDebug>
#include <QFile>

#include "common/xmlutils.h"

OLIVE_NAMESPACE_ENTER

Node::Node() :
  can_be_deleted_(true)
{
  output_ = new NodeOutput("node_out");
  AddParameter(output_);
}

Node::~Node()
{
  // We delete in the Node destructor rather than relying on the QObject system because the parameter may need to
  // perform actions on this Node object and we want them to be done before the Node object is fully destroyed
  foreach (NodeParam* param, params_) {

    // We disconnect input signals because these will try to send invalidate cache signals that may involve the derived
    // class (which is now destroyed). Any node that this is connected to will handle cache invalidation so it's a waste
    // of time anyway.
    if (param->type() == NodeParam::kInput) {
      DisconnectInput(static_cast<NodeInput*>(param));
    }

    delete param;
  }
}

void Node::Load(QXmlStreamReader *reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled)
{
  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("input") || reader->name() == QStringLiteral("output")) {
      QString param_id;

      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("id")) {
          param_id = attr.value().toString();

          break;
        }
      }

      if (param_id.isEmpty()) {
        qDebug() << "Found parameter with no ID";
        continue;
      }

      NodeParam* param;

      if (reader->name() == QStringLiteral("input")) {
        param = GetInputWithID(param_id);
      } else {
        param = GetOutputWithID(param_id);
      }

      if (!param) {
        qDebug() << "No parameter in" << id() << "with parameter" << param_id;
        continue;
      }

      param->Load(reader, xml_node_data, cancelled);
    } else {
      LoadInternal(reader, xml_node_data);
    }
  }
}

void Node::Save(QXmlStreamWriter *writer, const QString &custom_name) const
{
  writer->writeStartElement(custom_name.isEmpty() ? QStringLiteral("node") : custom_name);

  writer->writeAttribute(QStringLiteral("id"), id());

  writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(this)));

  writer->writeAttribute(QStringLiteral("pos"),
                         QStringLiteral("%1:%2").arg(QString::number(GetPosition().x()),
                                                     QString::number(GetPosition().y())));

  foreach (NodeParam* param, parameters()) {
    param->Save(writer);
  }

  SaveInternal(writer);

  writer->writeEndElement(); // node
}

QString Node::Category() const
{
  // Return an empty category for any nodes that don't use one
  return QString();
}

QString Node::Description() const
{
  // Return an empty string by default
  return QString();
}

void Node::Retranslate()
{
}

void Node::AddParameter(NodeParam *param)
{
  // Ensure no other param with this ID has been added to this Node (since that defeats the purpose)
  Q_ASSERT(!HasParamWithID(param->id()));

  if (params_.contains(param)) {
    return;
  }

  param->setParent(this);

  // Keep main output as the last parameter, assume if there are no parameters that this is the output parameter
  if (params_.isEmpty()) {
    params_.append(param);
  } else {
    params_.insert(params_.size()-1, param);
  }

  connect(param, &NodeParam::EdgeAdded, this, &Node::EdgeAdded);
  connect(param, &NodeParam::EdgeRemoved, this, &Node::EdgeRemoved);

  if (param->type() == NodeParam::kInput) {
    ConnectInput(static_cast<NodeInput*>(param));
  }
}

NodeValueTable Node::Value(NodeValueDatabase &value) const
{
  return value.Merge();
}

void Node::InvalidateCache(const TimeRange &range, NodeInput *from, NodeInput *source)
{
  Q_UNUSED(from)

  SendInvalidateCache(range, source);
}

void Node::InvalidateVisible(NodeInput *from, NodeInput* source)
{
  Q_UNUSED(from)

  foreach (NodeParam* param, params_) {
    if (param->type() == NodeParam::kOutput) {
      foreach (NodeEdgePtr edge, param->edges()) {
        edge->input()->parentNode()->InvalidateVisible(edge->input(), source);
      }
    }
  }
}

TimeRange Node::InputTimeAdjustment(NodeInput *, const TimeRange &input_time) const
{
  // Default behavior is no time adjustment at all
  return input_time;
}

TimeRange Node::OutputTimeAdjustment(NodeInput *, const TimeRange &input_time) const
{
  // Default behavior is no time adjustment at all
  return input_time;
}

void Node::SendInvalidateCache(const TimeRange &range, NodeInput *source)
{
  // Loop through all parameters (there should be no children that are not NodeParams)
  foreach (NodeParam* param, params_) {
    // If the Node is an output, relay the signal to any Nodes that are connected to it
    if (param->type() == NodeParam::kOutput) {

      QVector<NodeEdgePtr> edges = param->edges();

      foreach (NodeEdgePtr edge, edges) {
        NodeInput* connected_input = edge->input();
        Node* connected_node = connected_input->parentNode();

        // Send clear cache signal to the Node
        connected_node->InvalidateCache(range, connected_input, source);
      }
    }
  }
}

void Node::LoadInternal(QXmlStreamReader *reader, XMLNodeData &)
{
  reader->skipCurrentElement();
}

void Node::SaveInternal(QXmlStreamWriter *) const
{
}

QString Node::ReadFileAsString(const QString &filename)
{
  QFile f(filename);
  QString file_data;
  if (f.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream text_stream(&f);
    file_data = text_stream.readAll();
    f.close();
  }
  return file_data;
}

void GetInputsIncludingArraysInternal(NodeInputArray* array, QList<NodeInput *>& list)
{
  foreach (NodeInput* input, array->sub_params()) {
    list.append(input);

    if (input->IsArray()) {
      GetInputsIncludingArraysInternal(static_cast<NodeInputArray*>(input), list);
    }
  }
}

QList<NodeInput *> Node::GetInputsIncludingArrays() const
{
  QList<NodeInput *> inputs;

  foreach (NodeParam* param, params_) {
    if (param->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(param);

      inputs.append(input);

      if (input->IsArray()) {
        GetInputsIncludingArraysInternal(static_cast<NodeInputArray*>(input), inputs);
      }
    }
  }

  return inputs;
}

QList<NodeOutput *> Node::GetOutputs() const
{
  // The current design only uses one output per node. This function returns a list just in case that changes.
  return {output_};
}

void Node::CopyInputs(Node *source, Node *destination, bool include_connections)
{
  Q_ASSERT(source->id() == destination->id());

  const QList<NodeParam*>& src_param = source->params_;
  const QList<NodeParam*>& dst_param = destination->params_;

  for (int i=0;i<src_param.size();i++) {
    NodeParam* p = src_param.at(i);

    if (p->type() == NodeParam::kInput) {
      NodeInput* src = static_cast<NodeInput*>(p);

      NodeInput* dst = static_cast<NodeInput*>(dst_param.at(i));

      NodeInput::CopyValues(src, dst, include_connections);
    }
  }
}

bool Node::CanBeDeleted() const
{
  return can_be_deleted_;
}

void Node::SetCanBeDeleted(bool s)
{
  can_be_deleted_ = s;
}

bool Node::IsBlock() const
{
  return false;
}

bool Node::IsTrack() const
{
  return false;
}

const QList<NodeParam *>& Node::parameters() const
{
  return params_;
}

int Node::IndexOfParameter(NodeParam *param) const
{
  return params_.indexOf(param);
}

/**
 * @brief Recursively collects dependencies of Node `n` and appends them to QList `list`
 *
 * @param traverse
 *
 * TRUE to recursively traverse each node for a complete dependency graph. FALSE to return only the immediate
 * dependencies.
 */
QList<Node*> Node::GetDependenciesInternal(bool traverse, bool exclusive_only) const {
  QList<NodeInput*> inputs = GetInputsIncludingArrays();
  QList<Node*> list;

  foreach (NodeInput* i, inputs) {
    i->GetDependencies(list, traverse, exclusive_only);
  }

  return list;
}

QList<Node *> Node::GetDependencies() const
{
  return GetDependenciesInternal(true, false);
}

QList<Node *> Node::GetExclusiveDependencies() const
{
  return GetDependenciesInternal(true, true);
}

QList<Node *> Node::GetImmediateDependencies() const
{
  return GetDependenciesInternal(false, false);
}

Node::Capabilities Node::GetCapabilities(const NodeValueDatabase &) const
{
  return kNormal;
}

QString Node::ShaderID(const NodeValueDatabase &) const
{
  return id();
}

QString Node::ShaderVertexCode(const NodeValueDatabase &) const
{
  return QString();
}

QString Node::ShaderFragmentCode(const NodeValueDatabase&) const
{
  return QString();
}

int Node::ShaderIterations() const
{
  return 1;
}

NodeInput *Node::ShaderIterativeInput() const
{
  return nullptr;
}

NodeInput* Node::ProcessesSamplesFrom(const NodeValueDatabase &) const
{
  return nullptr;
}

void Node::ProcessSamples(const NodeValueDatabase &, const AudioRenderingParams&, const SampleBufferPtr, SampleBufferPtr, int) const
{
}

NodeInput *Node::GetInputWithID(const QString &id) const
{
  QList<NodeInput*> inputs = GetInputsIncludingArrays();

  foreach (NodeInput* i, inputs) {
    if (i->id() == id) {
      return i;
    }
  }

  return nullptr;
}

NodeOutput *Node::GetOutputWithID(const QString &id) const
{
  foreach (NodeParam* p, params_) {
    if (p->type() == NodeParam::kOutput
        && p->id() == id) {
      return static_cast<NodeOutput*>(p);
    }
  }

  return nullptr;
}

bool Node::OutputsTo(Node *n) const
{
  foreach (NodeParam* param, params_) {
    if (param->type() == NodeParam::kOutput) {
      QVector<NodeEdgePtr> edges = param->edges();

      foreach (NodeEdgePtr edge, edges) {
        if (edge->input()->parent() == n) {
          return true;
        }
      }
    }
  }

  return false;
}

bool Node::HasInputs() const
{
  return HasParamOfType(NodeParam::kInput, false);
}

bool Node::HasOutputs() const
{
  return HasParamOfType(NodeParam::kOutput, false);
}

bool Node::HasConnectedInputs() const
{
  return HasParamOfType(NodeParam::kInput, true);
}

bool Node::HasConnectedOutputs() const
{
  return HasParamOfType(NodeParam::kOutput, true);
}

void Node::DisconnectAll()
{
  foreach (NodeParam* param, params_) {
    param->DisconnectAll();
  }
}

QList<TimeRange> Node::TransformTimeTo(const TimeRange &time, Node *target, NodeParam::Type direction)
{
  QList<TimeRange> paths_found;

  if (direction == NodeParam::kInput) {
    // Get list of all inputs
    QList<NodeInput *> inputs = GetInputsIncludingArrays();

    // If this input is connected, traverse it to see if we stumble across the specified `node`
    foreach (NodeInput* input, inputs) {
      if (input->IsConnected()) {
        TimeRange input_adjustment = InputTimeAdjustment(input, time);
        Node* connected = input->get_connected_node();

        if (connected == target) {
          // We found the target, no need to keep traversing
          if (!paths_found.contains(input_adjustment)) {
            paths_found.append(input_adjustment);
          }
        } else {
          // We did NOT find the target, traverse this
          paths_found.append(connected->TransformTimeTo(input_adjustment, target, direction));
        }
      }
    }
  } else {
    // Get list of all outputs
    QList<NodeOutput*> outputs = GetOutputs();

    // If this input is connected, traverse it to see if we stumble across the specified `node`
    foreach (NodeOutput* output, outputs) {
      if (output->IsConnected()) {
        foreach (NodeEdgePtr edge, output->edges()) {
          Node* input_node = edge->input()->parentNode();

          TimeRange output_adjustment = input_node->OutputTimeAdjustment(edge->input(), time);

          if (input_node == target) {
            paths_found.append(output_adjustment);
          } else {
            paths_found.append(input_node->TransformTimeTo(output_adjustment, target, direction));
          }
        }
      }
    }
  }

  return paths_found;
}

QVariant Node::PtrToValue(void *ptr)
{
  return reinterpret_cast<quintptr>(ptr);
}

bool Node::HasParamWithID(const QString &id) const
{
  foreach (NodeParam* p, params_)
  {
    if (p->id() == id)
    {
      return true;
    }
  }

  return false;
}

NodeOutput *Node::output() const
{
  return output_;
}

NodeValue Node::InputValueFromTable(NodeInput *input, NodeValueDatabase &db, bool take) const
{
  NodeParam::DataType find_data_type = input->data_type();

  // Exception for Footage types (try to get a Texture instead)
  if (find_data_type == NodeParam::kFootage) {
    find_data_type = NodeParam::kTexture;
  }

  // Try to get a value from it
  if (take) {
    return db[input].TakeWithMeta(find_data_type);
  } else {
    return db[input].GetWithMeta(find_data_type);
  }
}

const QPointF &Node::GetPosition() const
{
  return position_;
}

void Node::SetPosition(const QPointF &pos)
{
  position_ = pos;

  emit PositionChanged(position_);
}

void Node::AddInput(NodeInput *input)
{
  AddParameter(input);
}

bool Node::HasParamOfType(NodeParam::Type type, bool must_be_connected) const
{
  foreach (NodeParam* p, params_) {
    if (p->type() == type
        && (p->IsConnected() || !must_be_connected)) {
      return true;
    }
  }

  return false;
}

void Node::ConnectInput(NodeInput *input)
{
  connect(input, &NodeInput::ValueChanged, this, &Node::InputChanged);
  connect(input, &NodeInput::EdgeAdded, this, &Node::InputConnectionChanged);
  connect(input, &NodeInput::EdgeRemoved, this, &Node::InputConnectionChanged);

  if (input->IsArray()) {
    NodeInputArray* array = static_cast<NodeInputArray*>(input);

    connect(array, &NodeInputArray::SubParamEdgeAdded, this, &Node::InputConnectionChanged);
    connect(array, &NodeInputArray::SubParamEdgeRemoved, this, &Node::InputConnectionChanged);
    connect(array, &NodeInputArray::SubParamEdgeAdded, this, &Node::EdgeAdded);
    connect(array, &NodeInputArray::SubParamEdgeRemoved, this, &Node::EdgeRemoved);
  }
}

void Node::DisconnectInput(NodeInput *input)
{
  if (input->IsArray()) {
    NodeInputArray* array = static_cast<NodeInputArray*>(input);

    disconnect(array, &NodeInputArray::SubParamEdgeAdded, this, &Node::InputConnectionChanged);
    disconnect(array, &NodeInputArray::SubParamEdgeRemoved, this, &Node::InputConnectionChanged);
    disconnect(array, &NodeInputArray::SubParamEdgeAdded, this, &Node::EdgeAdded);
    disconnect(array, &NodeInputArray::SubParamEdgeRemoved, this, &Node::EdgeRemoved);
  }

  disconnect(input, &NodeInput::ValueChanged, this, &Node::InputChanged);
  disconnect(input, &NodeInput::EdgeAdded, this, &Node::InputConnectionChanged);
  disconnect(input, &NodeInput::EdgeRemoved, this, &Node::InputConnectionChanged);
}

void Node::InputChanged(const TimeRange& range)
{
  InvalidateCache(range, static_cast<NodeInput*>(sender()), static_cast<NodeInput*>(sender()));
}

void Node::InputConnectionChanged(NodeEdgePtr edge)
{
  InvalidateCache(TimeRange(RATIONAL_MIN, RATIONAL_MAX), edge->input(), edge->input());
}

OLIVE_NAMESPACE_EXIT
