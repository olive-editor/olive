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

Node::Node() :
  last_processed_time_(-1),
  can_be_deleted_(true)
{
}

QString Node::Category()
{
  // Return an empty category for any nodes that don't use one
  return QString();
}

QString Node::Description()
{
  // Return an empty string by default
  return QString();
}

void Node::Release()
{
}

void Node::Retranslate()
{
}

void Node::AddParameter(NodeParam *param)
{
  // Ensure no other param with this ID has been added to this Node (since that defeats the purpose)
  Q_ASSERT(!HasParamWithID(param->id()));

  param->setParent(this);
  params_.append(param);

  connect(param, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SIGNAL(EdgeAdded(NodeEdgePtr)));
  connect(param, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SIGNAL(EdgeRemoved(NodeEdgePtr)));

  if (param->type() == NodeParam::kInput) {
    connect(param, SIGNAL(ValueChanged(rational, rational)), this, SLOT(InputChanged(rational, rational)));
    connect(param, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SLOT(InputConnectionChanged(NodeEdgePtr)));
    connect(param, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SLOT(InputConnectionChanged(NodeEdgePtr)));
  }
}

void Node::RemoveParameter(NodeParam *param)
{
  params_.removeAll(param);
  delete param;
}

QVariant Node::Value(NodeOutput *output)
{
  Q_UNUSED(output)

  return QVariant();
}

void Node::InvalidateCache(const rational &start_range, const rational &end_range, NodeInput *from)
{
  Q_UNUSED(from)

  foreach (NodeParam* param, params_) {
    if (param->type() == NodeParam::kOutput) {
      NodeOutput* output = static_cast<NodeOutput*>(param);
      output->drop_cached_values_overlapping(TimeRange(start_range, end_range));
    }
  }

  SendInvalidateCache(start_range, end_range);
}

TimeRange Node::InputTimeAdjustment(NodeInput *input, const TimeRange &input_time)
{
  Q_UNUSED(input)

  // Default behavior is no time adjustment at all

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
        Node* connected_node = connected_input->parent();

        // Send clear cache signal to the Node
        connected_node->InvalidateCache(start_range, end_range, connected_input);
      }
    }
  }
}

void Node::LockUserInput()
{
  user_input_lock_.lock();
}

void Node::UnlockUserInput()
{
  user_input_lock_.unlock();
}

void Node::LockProcessing()
{
  processing_lock_.lock();
}

void Node::UnlockProcessing()
{
  processing_lock_.unlock();
}

bool Node::IsProcessingLocked()
{
  if (processing_lock_.tryLock()) {
    processing_lock_.unlock();
    return false;
  } else {
    return true;
  }
}

void Node::CopyInputs(Node *source, Node *destination)
{
  Q_ASSERT(source->id() == destination->id());

  const QList<NodeParam*>& src_param = source->params_;
  const QList<NodeParam*>& dst_param = destination->params_;

  for (int i=0;i<src_param.size();i++) {
    if (src_param.at(i)->type() == NodeParam::kInput) {
      NodeInput* src = static_cast<NodeInput*>(src_param.at(i));

      if (src->dependent()) {
        NodeInput* dst = static_cast<NodeInput*>(dst_param.at(i));

        NodeInput::CopyValues(src, dst);
      }
    }
  }
}

bool Node::CanBeDeleted()
{
  return can_be_deleted_;
}

void Node::SetCanBeDeleted(bool s)
{
  can_be_deleted_ = s;
}

bool Node::IsBlock()
{
  return false;
}

rational Node::LastProcessedTime()
{
  rational t;

  LockUserInput();

  t = last_processed_time_;

  UnlockUserInput();

  return t;
}

NodeOutput *Node::LastProcessedOutput()
{
  NodeOutput* o;

  LockUserInput();

  o = last_processed_parameter_;

  UnlockUserInput();

  return o;
}

const QList<NodeParam *>& Node::parameters()
{
  return params_;
}

int Node::IndexOfParameter(NodeParam *param)
{
  return children().indexOf(param);
}

/**
 * @brief Recursively collects dependencies of Node `n` and appends them to QList `list`
 *
 * @param traverse
 *
 * TRUE to recursively traverse each node for a complete dependency graph. FALSE to return only the immediate
 * dependencies.
 */
void GetDependenciesInternal(Node* n, QList<Node*>& list, bool traverse) {
  foreach (NodeParam* p, n->parameters()) {
    if (p->type() == NodeParam::kInput) {
      Node* connected = static_cast<NodeInput*>(p)->get_connected_node();

      if (connected != nullptr && !list.contains(connected)) {
        list.append(connected);

        if (traverse) {
          GetDependenciesInternal(connected, list, traverse);
        }
      }
    }
  }
}

QList<Node *> Node::GetDependencies()
{
  QList<Node *> node_list;

  GetDependenciesInternal(this, node_list, true);

  return node_list;
}

QList<Node *> Node::GetExclusiveDependencies()
{
  QList<Node*> deps = GetDependencies();

  // Filter out any dependencies that are used elsewhere
  for (int i=0;i<deps.size();i++) {
    QList<NodeParam*> params = deps.at(i)->params_;

    // See if any of this Node's outputs are used outside of this dep list
    for (int j=0;j<params.size();j++) {
      NodeParam* p = params.at(j);

      if (p->type() == NodeParam::kOutput) {
        QVector<NodeEdgePtr> edges = p->edges();

        for (int k=0;k<edges.size();k++) {
          NodeEdgePtr edge = edges.at(k);

          // If any edge goes to from an output here to an input of a Node that isn't in this dep list, it's NOT an
          // exclusive dependency
          if (deps.contains(edge->input()->parent())) {
            deps.removeAt(i);

            i--;                // -1 since we just removed a Node in this list
            j = params.size();  // No need to keep looking at this Node's params
            break;              // Or this param's edges
          }
        }
      }
    }
  }

  return deps;
}

QList<Node *> Node::GetImmediateDependencies()
{
  QList<Node *> node_list;

  GetDependenciesInternal(this, node_list, false);

  return node_list;
}

QString Node::Code(NodeOutput *output)
{
  Q_UNUSED(output)

  return QString();
}

NodeParam *Node::GetParameterWithID(const QString &id)
{
  foreach (NodeParam* param, params_) {
    if (param->id() == id) {
      return param;
    }
  }

  return nullptr;
}

bool Node::OutputsTo(Node *n)
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

bool Node::HasInputs()
{
  return HasParamOfType(NodeParam::kInput, false);
}

bool Node::HasOutputs()
{
  return HasParamOfType(NodeParam::kOutput, false);
}

bool Node::HasConnectedInputs()
{
  return HasParamOfType(NodeParam::kInput, true);
}

bool Node::HasConnectedOutputs()
{
  return HasParamOfType(NodeParam::kOutput, true);
}

void Node::DisconnectAll()
{
  foreach (NodeParam* param, params_) {
    param->DisconnectAll();
  }
}

/*void Node::Hash(QCryptographicHash *hash, NodeOutput* from, const rational &time)
{
  // Add this Node's ID
  hash->addData(id().toUtf8());

  // Add each value
  foreach (NodeParam* param, params_) {
    if (param->type() == NodeParam::kInput
        && !param->IsConnected()
        && static_cast<NodeInput*>(param)->dependent()) {
      // Get the value at this time
      NodeInput* input = static_cast<NodeInput*>(param);

      QVariant v = input->value(time);

      hash->addData(NodeParam::ValueToBytes(input->data_type(), v));
    }
  }

  // Add each dependency node
  QList<NodeDependency> deps = RunDependencies(from, time);
  foreach (const NodeDependency& dep, deps) {
    // Hash the connected node
    dep.node()->parent()->Hash(hash, dep.node(), dep.in());
  }
}*/

QVariant Node::PtrToValue(void *ptr)
{
  return reinterpret_cast<quintptr>(ptr);
}

bool Node::HasParamWithID(const QString &id)
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

bool Node::HasParamOfType(NodeParam::Type type, bool must_be_connected)
{
  foreach (NodeParam* p, params_) {
    if (p->type() == type
        && (p->IsConnected() || !must_be_connected)) {
      return true;
    }
  }

  return false;
}

void Node::InputChanged(rational start, rational end)
{
  InvalidateCache(start, end, static_cast<NodeInput*>(sender()));
}

void Node::InputConnectionChanged(NodeEdgePtr edge)
{
  Q_UNUSED(edge)

  InvalidateCache(RATIONAL_MIN, RATIONAL_MAX, static_cast<NodeInput*>(sender()));
}
