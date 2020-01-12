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

#include <QDebug>
#include <QFile>

#include "common/xmlreadloop.h"

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

void Node::Load(QXmlStreamReader *reader)
{
  XMLReadLoop(reader, "node") {
    if (reader->isStartElement()) {
      if (reader->name() == "input" || reader->name() == "output") {
        QString param_id;

        XMLAttributeLoop(reader, attr) {
          if (attr.name() == "id") {
            param_id = attr.value().toString();

            break;
          }
        }

        if (param_id.isEmpty()) {
          qDebug() << "Found parameter with no ID";
          continue;
        }

        NodeParam* param = GetParameterWithID(param_id);

        if (!param) {
          qDebug() << "No parameter in" << id() << "with parameter" << param_id;
          continue;
        }

        param->Load(reader);
      }
    }
  }
}

void Node::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("node");

  writer->writeAttribute("id", id());

  foreach (NodeParam* param, parameters()) {
    param->Save(writer);
  }

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

  connect(param, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SIGNAL(EdgeAdded(NodeEdgePtr)));
  connect(param, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SIGNAL(EdgeRemoved(NodeEdgePtr)));

  if (param->type() == NodeParam::kInput) {
    ConnectInput(static_cast<NodeInput*>(param));
  }
}

NodeValueTable Node::Value(const NodeValueDatabase &value) const
{
  return value.Merge();
}

void Node::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Q_UNUSED(from)

  SendInvalidateCache(start_range, end_range);
}

TimeRange Node::InputTimeAdjustment(NodeInput *input, const TimeRange &input_time) const
{
  // Default behavior is no time adjustment at all
  Q_UNUSED(input)
  return input_time;
}

void Node::SendInvalidateCache(const rational &start_range, const rational &end_range)
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
        connected_node->InvalidateCache(start_range, end_range, connected_input);
      }
    }
  }
}

void Node::DependentEdgeChanged(NodeInput *from)
{
  Q_UNUSED(from)

  foreach (NodeParam* p, params_) {
    if (p->type() == NodeParam::kOutput && p->IsConnected()) {
      NodeOutput* out = static_cast<NodeOutput*>(p);

      foreach (NodeEdgePtr edge, out->edges()) {
        NodeInput* connected_input = edge->input();
        Node* connected_node = connected_input->parentNode();

        connected_node->DependentEdgeChanged(connected_input);
      }
    }
  }
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

void Node::LockUserInput()
{
  user_input_lock_.lock();
}

void Node::UnlockUserInput()
{
  user_input_lock_.unlock();
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

      NodeInput::CopyValues(src, dst, include_connections, true);
    }
  }
}

void DuplicateConnectionsBetweenListsInternal(const QList<Node *> &source, const QList<Node *> &destination, NodeInput* source_input, NodeInput* dest_input)
{
  if (source_input->IsConnected()) {
    // Get this input's connected outputs
    NodeOutput* source_output = source_input->get_connected_output();
    Node* source_output_node = source_output->parentNode();

    // Find equivalent in destination list
    Node* dest_output_node = destination.at(source.indexOf(source_output_node));

    Q_ASSERT(dest_output_node->id() == source_output_node->id());

    NodeOutput* dest_output = static_cast<NodeOutput*>(dest_output_node->GetParameterWithID(source_output->id()));

    NodeParam::ConnectEdge(dest_output, dest_input);
  }

  // If inputs are arrays, duplicate their connections too
  if (source_input->IsArray()) {
    NodeInputArray* source_array = static_cast<NodeInputArray*>(source_input);
    NodeInputArray* dest_array = static_cast<NodeInputArray*>(dest_input);

    for (int i=0;i<source_array->GetSize();i++) {
      DuplicateConnectionsBetweenListsInternal(source, destination, source_array->At(i), dest_array->At(i));
    }
  }
}

void Node::DuplicateConnectionsBetweenLists(const QList<Node *> &source, const QList<Node *> &destination)
{
  Q_ASSERT(source.size() == destination.size());

  for (int i=0;i<source.size();i++) {
    Node* source_input_node = source.at(i);
    Node* dest_input_node = destination.at(i);

    Q_ASSERT(source_input_node->id() == dest_input_node->id());

    for (int j=0;j<source_input_node->params_.size();j++) {
      NodeParam* source_param = source_input_node->params_.at(j);

      if (source_param->type() == NodeInput::kInput) {
        NodeInput* source_input = static_cast<NodeInput*>(source_param);
        NodeInput* dest_input = static_cast<NodeInput*>(dest_input_node->params_.at(j));

        DuplicateConnectionsBetweenListsInternal(source, destination, source_input, dest_input);
      }
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

void Node::TraverseInputInternal(QList<Node*>& list, NodeInput* input, bool traverse) {
  Node* connected = input->get_connected_node();

  if (connected != nullptr && !list.contains(connected)) {
    list.append(connected);

    if (traverse) {
      GetDependenciesInternal(connected, list, traverse);
    }
  }

  if (input->IsArray()) {
    NodeInputArray* input_array = static_cast<NodeInputArray*>(input);

    for (int i=0;i<input_array->GetSize();i++) {
      TraverseInputInternal(list, input_array->At(i), traverse);
    }
  }
}

/**
 * @brief Recursively collects dependencies of Node `n` and appends them to QList `list`
 *
 * @param traverse
 *
 * TRUE to recursively traverse each node for a complete dependency graph. FALSE to return only the immediate
 * dependencies.
 */
void Node::GetDependenciesInternal(const Node* n, QList<Node*>& list, bool traverse) {
  foreach (NodeParam* p, n->parameters()) {
    if (p->type() == NodeParam::kInput) {
      NodeInput* input = static_cast<NodeInput*>(p);

      TraverseInputInternal(list, input, traverse);
    }
  }
}

QList<Node *> Node::GetDependencies() const
{
  QList<Node *> node_list;

  GetDependenciesInternal(this, node_list, true);

  return node_list;
}

QList<Node *> Node::GetExclusiveDependencies() const
{
  QList<Node*> dependency_tree = GetDependencies();

  QList<Node*> exclusive_deps;

  // See if any nodes in the list output somewhere else
  foreach (Node* dep, dependency_tree) {
    bool is_exclusive = true;

    foreach (NodeParam* param, dep->parameters()) {
      if (param->type() == NodeParam::kOutput) {
        foreach (NodeEdgePtr edge, param->edges()) {
          Node* node_param_outputs_to = edge->input()->parentNode();
          if (node_param_outputs_to != this && !dependency_tree.contains(node_param_outputs_to)) {
            is_exclusive = false;
            break;
          }
        }
      }

      if (!is_exclusive) {
        break;
      }
    }

    if (is_exclusive) {
      exclusive_deps.append(dep);
    }
  }

  return exclusive_deps;
}

QList<Node *> Node::GetImmediateDependencies() const
{
  QList<Node *> node_list;

  GetDependenciesInternal(this, node_list, false);

  return node_list;
}

bool Node::IsAccelerated() const
{
  return false;
}

QString Node::AcceleratedCodeVertex() const
{
  return QString();
}

QString Node::AcceleratedCodeFragment() const
{
  return QString();
}

int Node::AcceleratedCodeIterations() const
{
  return 1;
}

NodeInput *Node::AcceleratedCodeIterativeInput() const
{
  return nullptr;
}

NodeInput* Node::ProcessesSamplesFrom() const
{
  return nullptr;
}

void Node::ProcessSamples(const NodeValueDatabase *values, const AudioRenderingParams &params, const float *input, float *output, int index) const
{
  Q_UNUSED(values)
  Q_UNUSED(params)
  Q_UNUSED(input)
  Q_UNUSED(output)
  Q_UNUSED(index)
}

NodeParam *Node::GetParameterWithID(const QString &id) const
{
  foreach (NodeParam* param, params_) {
    if (param->id() == id) {
      return param;
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

QVariant Node::InputValueFromTable(NodeInput *input, const NodeValueTable &table) const
{
  NodeParam::DataType find_data_type = input->data_type();

  // Exception for Footage types (try to get a Texture instead)
  if (find_data_type == NodeParam::kFootage) {
    find_data_type = NodeParam::kTexture;
  }

  // Try to get a value from it
  return table.Get(find_data_type);
}

const QPointF &Node::GetPosition()
{
  return position_;
}

void Node::SetPosition(const QPointF &pos)
{
  position_ = pos;
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
  connect(input, SIGNAL(ValueChanged(rational, rational)), this, SLOT(InputChanged(rational, rational)));
  connect(input, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(InputConnectionChanged(NodeEdgePtr)));
  connect(input, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(InputConnectionChanged(NodeEdgePtr)));
}

void Node::DisconnectInput(NodeInput *input)
{
  disconnect(input, SIGNAL(ValueChanged(rational, rational)), this, SLOT(InputChanged(rational, rational)));
  disconnect(input, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(InputConnectionChanged(NodeEdgePtr)));
  disconnect(input, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(InputConnectionChanged(NodeEdgePtr)));
}

void Node::InputChanged(rational start, rational end)
{
  InvalidateCache(start, end, static_cast<NodeInput*>(sender()));
}

void Node::InputConnectionChanged(NodeEdgePtr edge)
{
  DependentEdgeChanged(edge->input());

  InvalidateCache(RATIONAL_MIN, RATIONAL_MAX, static_cast<NodeInput*>(sender()));
}
