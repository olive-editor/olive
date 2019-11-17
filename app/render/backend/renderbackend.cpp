#include "renderbackend.h"

#include <QDateTime>
#include <QThread>

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  compiled_(false),
  caching_(false),
  started_(false),
  viewer_node_(nullptr)
{
}

bool RenderBackend::Init()
{
  if (started_) {
    return true;
  }

  threads_.resize(QThread::idealThreadCount());

  for (int i=0;i<threads_.size();i++) {
    QThread* thread = new QThread(this);
    threads_.replace(i, thread);

    // We use low priority to keep the app responsive at all times (GUI thread should always prioritize over this one)
    thread->start(QThread::LowPriority);
  }

  started_ = InitInternal();

  // Connects workers and moves them to their respective threads
  InitWorkers();

  if (!started_) {
    Close();
  }

  return started_;
}

void RenderBackend::Close()
{
  if (!started_) {
    return;
  }

  started_ = false;

  CloseInternal();

  decoder_cache_.Clear();

  foreach (QThread* thread, threads_) {
    thread->quit();
    thread->wait(); // FIXME: Maximum time in case a thread is stuck?
  }
  threads_.clear();

  foreach (RenderWorker* processor, processors_) {
    delete processor;
  }
  processors_.clear();
}

const QString &RenderBackend::GetError() const
{
  return error_;
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ != nullptr) {
    DisconnectViewer(viewer_node_);

    Decompile();
  }

  viewer_node_ = viewer_node;

  if (viewer_node_ != nullptr) {
    ConnectViewer(viewer_node_);
  }
}

void RenderBackend::SetCacheName(const QString &s)
{
  cache_name_ = s;
  cache_time_ = QDateTime::currentMSecsSinceEpoch();

  RegenerateCacheID();
}

bool RenderBackend::IsInitiated()
{
  return started_;
}

bool RenderBackend::Compile()
{
  if (compiled_) {
    return true;
  }

  compiled_ = CompileInternal();

  if (!compiled_) {
    Decompile();
  }

  return compiled_;
}

void RenderBackend::Decompile()
{
  if (!compiled_) {
    return;
  }

  DecompileInternal();
}

void RenderBackend::RegenerateCacheID()
{
  QCryptographicHash hash(QCryptographicHash::Sha1);

  if (cache_name_.isEmpty()
      || !cache_time_
      || !GenerateCacheIDInternal(hash)) {
    cache_id_.clear();
    CacheIDChangedEvent(QString());
    return;
  }

  hash.addData(cache_name_.toUtf8());
  hash.addData(QString::number(cache_time_).toUtf8());

  QByteArray bytes = hash.result();
  cache_id_ = bytes.toHex();
  CacheIDChangedEvent(cache_id_);
}

void RenderBackend::SetError(const QString &error)
{
  error_ = error;
}

void RenderBackend::ConnectViewer(ViewerOutput *node)
{
  Q_UNUSED(node)
}

void RenderBackend::DisconnectViewer(ViewerOutput *node)
{
  Q_UNUSED(node)
}

void RenderBackend::CacheNext()
{
  if (!Init() || cache_queue_.isEmpty() || viewer_node() == nullptr || caching_) {
    return;
  }

  TimeRange cache_frame = cache_queue_.takeFirst();

  //qDebug() << "Caching FRAME" << cache_frame.in() << "to" << cache_frame.out();

  caching_ = GenerateData(cache_frame);
}

bool RenderBackend::GenerateData(const TimeRange &range)
{
  if (!Compile()) {
    qDebug() << "Graph remains uncompiled, nothing to be done";
    return false;
  }

  NodeDependency dep = NodeDependency(GetDependentInput()->get_connected_output(), range.in(), range.out());

  foreach (RenderWorker* worker, processors_) {
    if (worker->IsAvailable() || worker == processors_.last()) {
      QMetaObject::invokeMethod(worker,
                                "Render",
                                Qt::QueuedConnection,
                                Q_ARG(NodeDependency, dep));
      return true;
    }
  }

  return false;
}

ViewerOutput *RenderBackend::viewer_node() const
{
  return viewer_node_;
}

DecoderCache *RenderBackend::decoder_cache()
{
  return &decoder_cache_;
}

const QString &RenderBackend::cache_id() const
{
  return cache_id_;
}

const QVector<QThread *> &RenderBackend::threads()
{
  return threads_;
}

void RenderBackend::CacheIDChangedEvent(const QString &id)
{
  Q_UNUSED(id)
}

void RenderBackend::InitWorkers()
{
  for (int i=0;i<processors_.size();i++) {
    RenderWorker* processor = processors_.at(i);
    QThread* thread = threads().at(i);

    // Connect to it
    connect(processor, SIGNAL(RequestSibling(NodeDependency)), this, SLOT(ThreadRequestedSibling(NodeDependency)));
    ConnectWorkerToThis(processor);

    // Finally, we can move it to its own thread
    processor->moveToThread(thread);

    // This function blocks the main thread intentionally. See the documentation for this function to see why.
    processor->Init();
  }
}

void RenderBackend::ThreadRequestedSibling(NodeDependency dep)
{
  // Try to queue another thread to run this dep in advance
  foreach (RenderWorker* worker, processors_) {
    if (worker->IsAvailable()) {
      QMetaObject::invokeMethod(worker,
                                "RenderAsSibling",
                                Qt::QueuedConnection,
                                Q_ARG(NodeDependency, dep));
      return;
    }
  }
}
