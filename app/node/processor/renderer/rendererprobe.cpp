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

#include "rendererprobe.h"

#include <QDebug>

RendererProbe::RendererProbe()
{

}

RenderPath RendererProbe::ProbeNode(Node *node, int thread_count, const rational& time)
{
  Q_ASSERT(thread_count > 0);

  RenderPath render_path(thread_count);

  TraverseNode(render_path, node, time, 0, 0);

  render_path.Finalize();

  return render_path;
}

void RendererProbe::TraverseNode(RenderPath& path, Node *node, const rational& time, int thread, int index)
{
  bool can_take_node = true;

  if (path.ContainsNode(node) >= 0) {
    if (path.NodeIndex(node) < index) {
      // If the other thread's index is smaller, it means we can do it earlier here so we should "steal" it
      qDebug() << "Stealing" << node << "to" << thread;
      path.RemoveEntry(node);
    } else {
      // Otherwise, we'll be delaying it and there's no point to that
      can_take_node = false;
    }
  }

  // Add this node to the thread path
  if (can_take_node) {
    qDebug() << node << "will run on thread" << thread << "at index" << index;
    path.AddEntry(node, thread, index);
  }

  QList<Node*> deps = node->GetImmediateDependenciesAt(time);

  // Print debugging information
  foreach (Node* dep, deps) {
    qDebug() << "  Dependency found:" << dep;
  }

  int next_index = index + 1;

  foreach (Node* dep, deps) {
    int ideal_thread = path.FindAvailableThreadAtIndex(index);

    // FIXME: This will throw this thread's index off by one, test whether this is okay
    int run_thread = (ideal_thread == -1) ? thread : ideal_thread;

    TraverseNode(path, dep, time, run_thread, next_index);
  }
}
