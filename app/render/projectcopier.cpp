/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "projectcopier.h"

namespace olive {

ProjectCopier::ProjectCopier(QObject *parent) :
  QObject(parent)
{
  original_ = nullptr;
  copy_ = new Project();
  copy_->setParent(this);
}

void ProjectCopier::SetProject(Project *project)
{
  if (original_) {
    // Clear current project
    qDeleteAll(created_nodes_);
    created_nodes_.clear();
    copy_map_.clear();
    graph_update_queue_.clear();

    disconnect(original_, &NodeGraph::NodeAdded, this, &ProjectCopier::QueueNodeAdd);
    disconnect(original_, &NodeGraph::NodeRemoved, this, &ProjectCopier::QueueNodeRemove);
    disconnect(original_, &NodeGraph::InputConnected, this, &ProjectCopier::QueueEdgeAdd);
    disconnect(original_, &NodeGraph::InputDisconnected, this, &ProjectCopier::QueueEdgeRemove);
    disconnect(original_, &NodeGraph::ValueChanged, this, &ProjectCopier::QueueValueChange);
    disconnect(original_, &NodeGraph::InputValueHintChanged, this, &ProjectCopier::QueueValueHintChange);
  }

  original_ = project;

  if (original_) {
    // Add all nodes
    for (int i=0; i<copy_->nodes().size(); i++) {
      InsertIntoCopyMap(original_->nodes().at(i), copy_->nodes().at(i));
    }

    for (int i=copy_->nodes().size(); i<original_->nodes().size(); i++) {
      DoNodeAdd(original_->nodes().at(i));
    }

    // Add all connections
    foreach (Node* node, original_->nodes()) {
      for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
        DoEdgeAdd(it->second, it->first);
      }
    }

    // Ensure graph change value is just before the sync value
    UpdateGraphChangeValue();
    UpdateLastSyncedValue();

    // Connect signals for future node additions/deletions
    connect(original_, &NodeGraph::NodeAdded, this, &ProjectCopier::QueueNodeAdd, Qt::DirectConnection);
    connect(original_, &NodeGraph::NodeRemoved, this, &ProjectCopier::QueueNodeRemove, Qt::DirectConnection);
    connect(original_, &NodeGraph::InputConnected, this, &ProjectCopier::QueueEdgeAdd, Qt::DirectConnection);
    connect(original_, &NodeGraph::InputDisconnected, this, &ProjectCopier::QueueEdgeRemove, Qt::DirectConnection);
    connect(original_, &NodeGraph::ValueChanged, this, &ProjectCopier::QueueValueChange, Qt::DirectConnection);
    connect(original_, &NodeGraph::InputValueHintChanged, this, &ProjectCopier::QueueValueHintChange, Qt::DirectConnection);
  }
}

void ProjectCopier::ProcessUpdateQueue()
{
  // Iterate everything that happened to the graph and do the same thing on our end
  while (!graph_update_queue_.empty()) {
    QueuedJob job = graph_update_queue_.front();
    graph_update_queue_.pop_front();

    switch (job.type) {
    case QueuedJob::kNodeAdded:
      DoNodeAdd(job.node);
      break;
    case QueuedJob::kNodeRemoved:
      DoNodeRemove(job.node);
      break;
    case QueuedJob::kEdgeAdded:
      DoEdgeAdd(job.output, job.input);
      break;
    case QueuedJob::kEdgeRemoved:
      DoEdgeRemove(job.output, job.input);
      break;
    case QueuedJob::kValueChanged:
      DoValueChange(job.input);
      break;
    case QueuedJob::kValueHintChanged:
      DoValueHintChange(job.input);
      break;
    }
  }

  // Indicate that we have synchronized to this point, which is compared with the graph change
  // time to see if our copied graph is up to date
  UpdateLastSyncedValue();
}

void ProjectCopier::DoNodeAdd(Node *node)
{
  if (dynamic_cast<NodeGroup*>(node)) {
    // Group nodes are just dummy nodes, no need to copy them
    return;
  }

  // Copy node
  Node* copy = node->copy();

  // Add to project
  copy->setParent(copy_);

  // Disable caches for copy
  copy->SetCachesEnabled(false);

  // Copy cache UUIDs
  copy->CopyCacheUuidsFrom(node);

  // Insert into map
  InsertIntoCopyMap(node, copy);

  // Keep track of our nodes
  created_nodes_.append(copy);
}

void ProjectCopier::DoNodeRemove(Node *node)
{
  // Find our copy and remove it
  Node* copy = copy_map_.take(node);

  // Disconnect from node's caches
  emit RemovedNode(node);

  // Remove from created list
  created_nodes_.removeOne(copy);

  // Delete it
  delete copy;
}

void ProjectCopier::DoEdgeAdd(Node *output, const NodeInput &input)
{
  // Create same connection with our copied graph
  Node* our_output = copy_map_.value(output);
  Node* our_input = copy_map_.value(input.node());

  Node::ConnectEdge(our_output, NodeInput(our_input, input.input(), input.element()));
}

void ProjectCopier::DoEdgeRemove(Node *output, const NodeInput &input)
{
  // Remove same connection with our copied graph
  Node* our_output = copy_map_.value(output);
  Node* our_input = copy_map_.value(input.node());

  Node::DisconnectEdge(our_output, NodeInput(our_input, input.input(), input.element()));
}

void ProjectCopier::DoValueChange(const NodeInput &input)
{
  if (dynamic_cast<NodeGroup*>(input.node())) {
    // Group nodes are just dummy nodes, no need to copy them
    return;
  }

  // Copy all values to our graph
  Node* our_input = copy_map_.value(input.node());
  Node::CopyValuesOfElement(input.node(), our_input, input.input(), input.element());
}

void ProjectCopier::DoValueHintChange(const NodeInput &input)
{
  if (dynamic_cast<NodeGroup*>(input.node())) {
    // Group nodes are just dummy nodes, no need to copy them
    return;
  }

  // Copy value hint to our graph
  Node* our_input = copy_map_.value(input.node());
  Node::ValueHint hint = input.node()->GetValueHintForInput(input.input(), input.element());
  our_input->SetValueHintForInput(input.input(), hint, input.element());
}

void ProjectCopier::InsertIntoCopyMap(Node *node, Node *copy)
{
  // Insert into map
  copy_map_.insert(node, copy);

  // Copy parameters
  Node::CopyInputs(node, copy, false);

  // Connect to node's cache
  emit AddedNode(node);
}

void ProjectCopier::QueueNodeAdd(Node *node)
{
  graph_update_queue_.push_back({QueuedJob::kNodeAdded, node, NodeInput(), nullptr});
  UpdateGraphChangeValue();
}

void ProjectCopier::QueueNodeRemove(Node *node)
{
  graph_update_queue_.push_back({QueuedJob::kNodeRemoved, node, NodeInput(), nullptr});
  UpdateGraphChangeValue();
}

void ProjectCopier::QueueEdgeAdd(Node *output, const NodeInput &input)
{
  graph_update_queue_.push_back({QueuedJob::kEdgeAdded, nullptr, input, output});
  UpdateGraphChangeValue();
}

void ProjectCopier::QueueEdgeRemove(Node *output, const NodeInput &input)
{
  graph_update_queue_.push_back({QueuedJob::kEdgeRemoved, nullptr, input, output});
  UpdateGraphChangeValue();
}

void ProjectCopier::QueueValueChange(const NodeInput &input)
{
  graph_update_queue_.push_back({QueuedJob::kValueChanged, nullptr, input, nullptr});
  UpdateGraphChangeValue();
}

void ProjectCopier::QueueValueHintChange(const NodeInput &input)
{
  graph_update_queue_.push_back({QueuedJob::kValueHintChanged, nullptr, input, nullptr});
  UpdateGraphChangeValue();
}

void ProjectCopier::UpdateGraphChangeValue()
{
  graph_changed_time_.Acquire();
}

void ProjectCopier::UpdateLastSyncedValue()
{
  last_update_time_.Acquire();
}

}
