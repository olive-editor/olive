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

#include "common/qobjectlistcast.h"

Node::Node()
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

void Node::InvalidateCache(const rational &start_range, const rational &end_range)
{
  QList<NodeParam *> params = parameters();

  // Loop through all parameters (there should be no children that are not NodeParams)
  for (int i=0;i<params.size();i++) {
    NodeParam* param = params.at(i);

    // If the Node is an output, relay the signal to any Nodes that are connected to it
    if (param->type() == NodeParam::kOutput) {
      for (int i=0;i<param->edges().size();i++) {
        param->edges().at(i)->input()->parent()->InvalidateCache(start_range, end_range);
      }
    }
  }
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
 */
void GetDependenciesInternal(Node* n, QList<Node*>& list) {
  QList<NodeParam*> params = n->parameters();

  foreach (NodeParam* p, params) {
    if (p->type() == NodeParam::kInput) {
      QVector<NodeEdgePtr> param_edges = p->edges();

      foreach (NodeEdgePtr edge, param_edges) {
        Node* connected_node = edge->output()->parent();

        list.append(connected_node);
        GetDependenciesInternal(connected_node, list);
      }
    }
  }
}

QList<Node *> Node::GetDependencies()
{
  QList<Node *> node_list;

  GetDependenciesInternal(this, node_list);

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

            i--;                // -1 since we just removed a Node here
            j = params.size();  // No need to keep looking at this Node's params
            break;              // Or this param's edges
          }
        }
      }
    }
  }

  return deps;
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
