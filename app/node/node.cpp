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

#include "common/qobjectlistcast.h"

Node::Node() :
  last_processed_time_(-1)
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

void Node::AddParameter(NodeParam *param)
{
  // Ensure no other param with this ID has been added to this Node (since that defeats the purpose)
  Q_ASSERT(!HasParamWithID(param->id()));

  param->setParent(this);

  connect(param, SIGNAL(EdgeAdded(NodeEdgePtr)), this, SIGNAL(EdgeAdded(NodeEdgePtr)));
  connect(param, SIGNAL(EdgeRemoved(NodeEdgePtr)), this, SIGNAL(EdgeRemoved(NodeEdgePtr)));
}

void Node::RemoveParameter(NodeParam *param)
{
  delete param;
}

void Node::InvalidateCache(const rational &start_range, const rational &end_range)
{
  QList<NodeParam *> params = parameters();

  // Loop through all parameters (there should be no children that are not NodeParams)
  foreach (NodeParam* param, params) {

    // If the Node is an output, relay the signal to any Nodes that are connected to it
    if (param->type() == NodeParam::kOutput) {

      foreach (NodeEdgePtr edge, param->edges()) {

        NodeInput* connected_input = edge->input();
        Node* connected_node = connected_input->parent();

        // Only send this signal if the Node isn't ignoring invalidate cache signals from this input
        if (!connected_node->ignore_invalid_cache_inputs_.contains(connected_input)) {
          connected_node->InvalidateCache(start_range, end_range);
        }
      }
    }
  }
}

void Node::IgnoreCacheInvalidationFrom(NodeInput *input)
{
  ignore_invalid_cache_inputs_.append(input);
}

rational Node::LastProcessedTime()
{
  rational t;

  lock_.lock();

  t = last_processed_time_;

  lock_.unlock();

  return t;
}

NodeOutput *Node::LastProcessedOutput()
{
  NodeOutput* o;

  lock_.lock();

  o = last_processed_parameter_;

  lock_.unlock();

  return o;
}

QVariant Node::Run(NodeOutput* output, const rational& time)
{
  lock_.lock();

  QVariant v = Value(output, time);

  lock_.unlock();

  return v;
}

NodeParam *Node::ParamAt(int index)
{
  return static_cast<NodeParam*>(children().at(index));
}

QList<NodeParam *> Node::parameters()
{
  return static_qobjectlist_cast<NodeParam>(children());
}

int Node::ParameterCount()
{
  return children().size();
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
  QList<NodeParam*> params = n->parameters();

  foreach (NodeParam* p, params) {
    if (p->type() == NodeParam::kInput) {
      QVector<NodeEdgePtr> param_edges = p->edges();

      foreach (NodeEdgePtr edge, param_edges) {
        Node* connected_node = edge->output()->parent();

        list.append(connected_node);

        if (traverse) {
          GetDependenciesInternal(connected_node, list, traverse);
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
    QList<NodeParam*> params = deps.at(i)->parameters();

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

QList<NodeDependency> Node::RunDependencies(NodeOutput *output, const rational &time)
{
  Q_UNUSED(output)

  QList<Node*> immediate_deps = GetImmediateDependencies();
  QList<NodeDependency> run_deps;

  foreach (Node* dep, immediate_deps) {
    run_deps.append(NodeDependency(dep, time));
  }

  return run_deps;
}

bool Node::OutputsTo(Node *n)
{
  QList<NodeParam*> params = parameters();

  foreach (NodeParam* param, params) {
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

QVariant Node::PtrToValue(void *ptr)
{
  return reinterpret_cast<quintptr>(ptr);
}

bool Node::HasParamWithID(const QString &id)
{
  QList<NodeParam*> params = parameters();

  foreach (NodeParam* p, params)
  {
    if (p->id() == id)
    {
      return true;
    }
  }

  return false;
}

NodeDependency::NodeDependency(Node *node, const rational &time) :
  node_(node),
  time_(time)
{
}

Node *NodeDependency::node()
{
  return node_;
}

rational NodeDependency::time()
{
  return time_;
}
