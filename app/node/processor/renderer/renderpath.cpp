#include "renderpath.h"

RenderPath::RenderPath(int max_threads) :
  finalized_(false)
{
  Q_ASSERT(max_threads > 0);

  render_path_.resize(max_threads);
}

void RenderPath::AddEntry(Node *node, int thread, int index)
{
  if (finalized_) {
    return;
  }

  RenderThreadPath& thread_path = render_path_[thread];

  // Make sure there are enough indices for this entry
  while (thread_path.size() < index) {
    thread_path.append(nullptr);
  }

  thread_path.append(node);
  thread_map_.insert(node, thread);
}

void RenderPath::RemoveEntry(Node *node)
{
  if (finalized_) {
    return;
  }

  Q_ASSERT(thread_map_.contains(node));

  int node_thread = thread_map_[node];

  render_path_[node_thread].replace(NodeIndexInThread(node, node_thread), nullptr);
  thread_map_.remove(node);
}

int RenderPath::ContainsNode(Node *node)
{
  if (thread_map_.contains(node)) {
    return thread_map_[node];
  }
  return -1;
}

int RenderPath::NodeIndex(Node *node)
{
  if (thread_map_.contains(node)) {
    return NodeIndexInThread(node, thread_map_[node]);
  }
  return -1;
}

int RenderPath::NodeIndexInThread(Node *node, int thread)
{
  return render_path_[thread].indexOf(node);
}

void RenderPath::Finalize()
{
  for (int i=0;i<render_path_.size();i++) {
    render_path_[i].removeAll(nullptr);
  }

  finalized_ = true;
}

int RenderPath::FindAvailableThreadAtIndex(int index)
{
  for (int i=0;i<render_path_.size();i++) {
    QVector<Node*>& thread_path_ = render_path_[i];

    if (index >= thread_path_.size() || thread_path_.at(index) == nullptr) {
      return i;
    }
  }

  return -1;
}

RenderThreadPath &RenderPath::GetThreadPath(int thread)
{
  return render_path_[thread];
}
