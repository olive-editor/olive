#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include <QLinkedList>

#include "common/constructors.h"
#include "dialog/rendercancel/rendercancel.h"
#include "decodercache.h"
#include "node/graph.h"
#include "node/output/viewer/viewer.h"
#include "renderworker.h"

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
  void InvalidateCache(const TimeRange &range);

  bool Compile();

  void Decompile();

signals:
  void QueueComplete();

protected:
  void RegenerateCacheID();

  virtual bool InitInternal();

  virtual void CloseInternal();

  virtual bool CompileInternal() = 0;

  virtual void DecompileInternal() = 0;

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

  void QueueValueUpdate();

  bool AllProcessorsAreAvailable() const;
  bool WorkerIsBusy(RenderWorker* worker) const;
  void SetWorkerBusyState(RenderWorker* worker, bool busy);

  DecoderCache* decoder_cache();

  TimeRangeList cache_queue_;

  QVector<RenderWorker*> processors_;

  DecoderCache decoder_cache_;

  bool compiled_;

  QHash<TimeRange, qint64> render_job_info_;

protected slots:
  void QueueRecompile();

private:
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

  QList<Node*> source_node_list_;
  NodeGraph copied_graph_;

  bool recompile_queued_;
  bool input_update_queued_;

  QVector<bool> processor_busy_state_;

  RenderCancelDialog* cancel_dialog_;

  struct FootageWaitInfo {
    StreamPtr stream;
    TimeRange affected_range;
    rational stream_time;

    bool operator==(const FootageWaitInfo& rhs) const;
  };

  QList<FootageWaitInfo> footage_wait_info_;

private slots:
  void FootageUnavailable(StreamPtr stream, Decoder::RetrieveState state, const TimeRange& path, const rational& stream_time);

  void IndexUpdated(Stream *stream);

};

#endif // RENDERBACKEND_H
