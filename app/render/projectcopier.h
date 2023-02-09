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

#ifndef PROJECTCOPIER_H
#define PROJECTCOPIER_H

#include "node/project/project.h"

namespace olive {

class ProjectCopier : public QObject
{
  Q_OBJECT
public:
  ProjectCopier(QObject *parent = nullptr);

  void SetProject(Project *project);

  template <typename T>
  T *GetCopy(T *original)
  {
    return static_cast<T*>(copy_map_.value(original));
  }

  template <typename T>
  T *GetOriginal(T *copy)
  {
    return static_cast<T*>(copy_map_.key(copy));
  }

  const QHash<Node*, Node*> &GetNodeMap() const { return copy_map_; }

  const JobTime &GetGraphChangeTime() const { return graph_changed_time_; }
  const JobTime &GetLastUpdateTime() const { return last_update_time_; }

  bool HasUpdatesInQueue() const { return !graph_update_queue_.empty(); }

  /**
   * @brief Process all changes to internal NodeGraph copy
   *
   * PreviewAutoCacher staggers updates to its internal NodeGraph copy, only applying them when the
   * RenderManager is not reading from it. This function is called when such an opportunity arises.
   */
  void ProcessUpdateQueue();

signals:
  void AddedNode(Node *n);
  void RemovedNode(Node *n);

private:
  void DoNodeAdd(Node* node);
  void DoNodeRemove(Node* node);
  void DoEdgeAdd(Node *output, const NodeInput& input);
  void DoEdgeRemove(Node *output, const NodeInput& input);
  void DoValueChange(const NodeInput& input);
  void DoValueHintChange(const NodeInput& input);

  void InsertIntoCopyMap(Node* node, Node* copy);

  void UpdateGraphChangeValue();
  void UpdateLastSyncedValue();

  Project *original_;
  Project *copy_;

  class QueuedJob {
  public:
    enum Type {
      kNodeAdded,
      kNodeRemoved,
      kEdgeAdded,
      kEdgeRemoved,
      kValueChanged,
      kValueHintChanged
    };

    Type type;
    Node* node;
    NodeInput input;
    Node *output;
  };

  std::list<QueuedJob> graph_update_queue_;
  QHash<Node*, Node*> copy_map_;
  QHash<NodeGraph*, NodeGraph*> graph_map_;
  QVector<Node*> created_nodes_;

  JobTime graph_changed_time_;
  JobTime last_update_time_;

private slots:
  void QueueNodeAdd(Node* node);

  void QueueNodeRemove(Node* node);

  void QueueEdgeAdd(Node *output, const NodeInput& input);

  void QueueEdgeRemove(Node *output, const NodeInput& input);

  void QueueValueChange(const NodeInput& input);

  void QueueValueHintChange(const NodeInput &input);

};

}

#endif // PROJECTCOPIER_H
