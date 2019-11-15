#ifndef RENDERBACKEND_H
#define RENDERBACKEND_H

#include <QLinkedList>

#include "decodercache.h"
#include "node/output/viewer/viewer.h"
#include "renderworker.h"

class RenderBackend : public QObject
{
  Q_OBJECT
public:
  RenderBackend(QObject* parent = nullptr);

  Q_DISABLE_COPY_MOVE(RenderBackend)

  bool Init();

  void Close();

  const QString& GetError() const;

  void SetViewerNode(ViewerOutput* viewer_node);

  void SetCacheName(const QString& s);

  bool IsInitiated();

public slots:
  virtual void InvalidateCache(const rational &start_range, const rational &end_range) = 0;

  bool Compile();

  void Decompile();

protected:
  void RegenerateCacheID();

  virtual bool InitInternal() = 0;

  virtual void CloseInternal() = 0;

  virtual bool CompileInternal() = 0;

  virtual void DecompileInternal() = 0;

  const QVector<QThread*>& threads();

  /**
   * @brief Internal function for generating the cache ID
   */
  virtual bool GenerateCacheIDInternal(QCryptographicHash& hash) = 0;

  virtual void CacheIDChangedEvent(const QString& id);

  void SetError(const QString& error);

  virtual void ViewerNodeChangedEvent(ViewerOutput* node);

  /**
   * @brief Function called when there are frames in the queue to cache
   *
   * This function is NOT thread-safe and should only be called in the main thread.
   */
  void CacheNext();

  bool GenerateData(const TimeRange& range);

  void InitWorkers();

  virtual NodeInput* GetDependentInput() = 0;

  virtual void ConnectWorkerToThis(RenderWorker* worker) = 0;

  ViewerOutput* viewer_node() const;

  DecoderCache* decoder_cache();

  const QString& cache_id() const;

  QList<TimeRange> cache_queue_;

  QVector<RenderWorker*> processors_;

  bool compiled_;

  bool caching_;

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
   * @brief Error string that can be set in SetError() to handle failures
   */
  QString error_;

  DecoderCache decoder_cache_;

  QString cache_name_;
  qint64 cache_time_;
  QString cache_id_;

private slots:
  void ThreadRequestedSibling(NodeDependency dep);

};

#endif // RENDERBACKEND_H
