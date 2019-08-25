#ifndef RENDERPATH_H
#define RENDERPATH_H

#include <QVector>

#include "node/node.h"

using RenderThreadPath = QVector<Node*>;

class RenderPath
{
public:
  RenderPath(int max_threads);

  /**
   * @brief Add a Node to this thread at this index
   *
   * The Node is guaranteed to be added to at least this index. The index may be higher if this thread has other Nodes
   * taking up this index, but it will never be lower.
   */
  void AddEntry(Node* node, int thread, int index);

  /**
   * @brief Removes a Node, replacing its entry with nullptr
   *
   * This function asserts whether the Node was added initially to help aid bug detection.
   *
   * @param node
   */
  void RemoveEntry(Node* node);

  /**
   * @brief Determine whether the RenderPath already contains this Node
   *
   * @return
   *
   * The thread index if one contains this Node, or -1 if none of them do
   */
  int ContainsNode(Node* node);

  /**
   * @brief Determine what index a Node has in whatever thread it's in
   *
   * @return
   *
   * The Node's index in its thread, or -1 if no threads have this Node.
   */
  int NodeIndex(Node* node);

  /**
   * @brief Determine what index a Node has in a particular thread
   *
   * @return
   *
   * The Node's index in this thread, or -1 if this thread does not have this Node.
   */
  int NodeIndexInThread(Node* node, int thread);

  /**
   * @brief Finalize the render path ready for executing
   *
   * Probing uses indices to determine what Nodes occur where. Depending on the state of the graph, many of these
   * indices will be empty across all the threads which, while helpful for probing, is unnecessary for executing.
   * Running this function assumes no further probing will occur.
   *
   * Finalizing places this RenderPath into more-or-less a read-only state.
   */
  void Finalize();

  int FindAvailableThreadAtIndex(int index);

  RenderThreadPath& GetThreadPath(int thread);

private:
  QVector<RenderThreadPath> render_path_;

  QMap<Node*, int> thread_map_;

  bool finalized_;
};

#endif // RENDERPATH_H
