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

#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include <QLinkedList>

#include "dialog/rendercancel/rendercancel.h"
#include "decodercache.h"
#include "node/graph.h"
#include "node/output/viewer/viewer.h"
#include "renderworker.h"

OLIVE_NAMESPACE_ENTER

class RenderBackend : public QObject
{
  Q_OBJECT
public:
  RenderBackend(QObject* parent = nullptr);

  bool Init();

  void Close();

  const QString& GetError() const;

  void SetViewerNode(ViewerOutput* viewer_node);

  bool IsInitiated();

  ViewerOutput* viewer_node() const;

  void CancelQueue();

public slots:
  void InvalidateCache(const TimeRange &range, NodeInput *from);

signals:
  void QueueComplete();

protected:
  void RegenerateCacheID();

  virtual bool InitInternal();

  virtual void CloseInternal();

  virtual bool CanRender();

  virtual TimeRange PopNextFrameFromQueue();

  rational GetSequenceLength();

  const QVector<QThread*>& threads();

  /**
   * @brief Internal function for generating the cache ID
   */
  virtual bool GenerateCacheIDInternal(QCryptographicHash& hash) = 0;

  virtual void InvalidateCacheInternal(const rational &start_range, const rational &end_range);

  virtual void CacheIDChangedEvent(const QString& id);

  void SetError(const QString& error);

  virtual void ConnectViewer(ViewerOutput* node);
  virtual void DisconnectViewer(ViewerOutput* node);

  /**
   * @brief Function called when there are frames in the queue to cache
   *
   * This function is NOT thread-safe and should only be called in the main thread.
   */
  void CacheNext();

  void InitWorkers();

  virtual NodeInput* GetDependentInput() = 0;

  virtual void ConnectWorkerToThis(RenderWorker* worker) = 0;

  bool ViewerIsConnected() const;

  const QString& cache_id() const;

  void QueueValueUpdate(NodeInput *from);

  bool AllProcessorsAreAvailable() const;
  bool WorkerIsBusy(RenderWorker* worker) const;
  void SetWorkerBusyState(RenderWorker* worker, bool busy);

  TimeRangeList cache_queue_;

  QVector<RenderWorker*> processors_;

  QHash<TimeRange, qint64> render_job_info_;

  QHash<Node*, Node*> node_copy_map_;

  NodeGraph copied_graph_;

private:
  void CopyNodeInputValue(NodeInput* input);
  Node *CopyNodeConnections(Node *src_node);
  void CopyNodeMakeConnection(NodeInput *src_input, NodeInput *dst_input);

  /**
   * @brief Internal list of RenderProcessThreads
   */
  QVector<QThread*> threads_;

  /**
   * @brief Internal variable that contains whether the Renderer has started or not
   */
  bool started_;

  /**
   * @brief Internal reference to attached viewer node
   */
  ViewerOutput* viewer_node_;

  /**
   * @brief Internal reference to the copied viewer node we made in the compilation process
   */
  ViewerOutput* copied_viewer_node_;

  /**
   * @brief Error string that can be set in SetError() to handle failures
   */
  QString error_;

  QString cache_id_;

  QList<NodeInput*> input_update_queued_;

  QVector<bool> processor_busy_state_;

  RenderCancelDialog* cancel_dialog_;

  struct FootageWaitInfo {
    StreamPtr stream;
    TimeRange affected_range;
    rational stream_time;

    bool operator==(const FootageWaitInfo& rhs) const;
  };

  QList<FootageWaitInfo> footage_wait_info_;

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERBACKEND_H
