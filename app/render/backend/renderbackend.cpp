#include "renderbackend.h"

#include <QDateTime>
#include <QThread>

#include "core.h"
#include "window/mainwindow/mainwindow.h"

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  compiled_(false),
  started_(false),
  viewer_node_(nullptr),
  copied_viewer_node_(nullptr),
  recompile_queued_(false),
  input_update_queued_(false)
{
  // FIXME: Don't create in CLI mode
  cancel_dialog_ = new RenderCancelDialog(Core::instance()->main_window());
}

bool RenderBackend::Init()
{
  if (started_) {
    return true;
  }

  threads_.resize(qMax(1, QThread::idealThreadCount() - 1));

  for (int i=0;i<threads_.size();i++) {
    QThread* thread = new QThread(this);
    threads_.replace(i, thread);

    // We use low priority to keep the app responsive at all times (GUI thread should always prioritize over this one)
    thread->start(QThread::LowPriority);
  }

  cancel_dialog_->SetWorkerCount(threads_.size());

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

  CancelQueue();

  Decompile();

  CloseInternal();

  for (int i=0;i<processors_.size();i++) {
    // Invoke close and quit signals on processor and thred
    QMetaObject::invokeMethod(processors_.at(i),
                              "Close",
                              Qt::QueuedConnection);

    threads_.at(i)->quit();
  }

  for (int i=0;i<processors_.size();i++) {
    threads_.at(i)->wait(); // FIXME: Maximum time in case a thread is stuck?
    delete threads_.at(i);
    delete processors_.at(i);
  }

  threads_.clear();
  processors_.clear();
}

const QString &RenderBackend::GetError() const
{
  return error_;
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ != nullptr) {
    CancelQueue();

    DisconnectViewer(viewer_node_);

    Decompile();
  }

  viewer_node_ = viewer_node;

  if (viewer_node_ != nullptr) {
    ConnectViewer(viewer_node_);

    RegenerateCacheID();
  }
}

bool RenderBackend::IsInitiated()
{
  return started_;
}

void RenderBackend::InvalidateCache(const rational &start_range, const rational &end_range)
{
  if (!CanRender()) {
    return;
  }

  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), start_range);
  rational end_range_adj = qMin(GetSequenceLength(), end_range);

  qDebug() << "Cache invalidated between"
           << start_range_adj.toDouble()
           << "and"
           << end_range_adj.toDouble();

  // Queue value update
  QueueValueUpdate();

  InvalidateCacheInternal(start_range_adj, end_range_adj);
}

bool RenderBackend::Compile()
{
  if (compiled_) {
    return true;
  }

  // Get dependencies of viewer node
  source_node_list_.append(viewer_node_);
  source_node_list_.append(viewer_node_->GetDependencies());

  // Copy all dependencies into graph
  foreach (Node* n, source_node_list_) {
    Node* copy = n->copy();

    Node::CopyInputs(n, copy, false);

    copied_graph_.AddNode(copy);
  }

  // We just copied the inputs, so if an input update is queued, it's unnecessary
  input_update_queued_ = false;

  // We know that the first node will be the viewer node since we appended that first in the copy
  copied_viewer_node_ = static_cast<ViewerOutput*>(copied_graph_.nodes().first());

  // Copy connections
  Node::DuplicateConnectionsBetweenLists(source_node_list_, copied_graph_.nodes());

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

  copied_graph_.Clear();
  copied_viewer_node_ = nullptr;
  source_node_list_.clear();

  compiled_ = false;
}

void RenderBackend::RegenerateCacheID()
{
  QCryptographicHash hash(QCryptographicHash::Sha1);

  if (!viewer_node_
      || !GenerateCacheIDInternal(hash)) {
    cache_id_.clear();
    CacheIDChangedEvent(QString());
    return;
  }

  hash.addData(viewer_node_->uuid().toByteArray());

  QByteArray bytes = hash.result();
  cache_id_ = bytes.toHex();
  CacheIDChangedEvent(cache_id_);
}

bool RenderBackend::CanRender()
{
  return true;
}

TimeRange RenderBackend::PopNextFrameFromQueue()
{
  return cache_queue_.takeFirst();
}

rational RenderBackend::GetSequenceLength()
{
  if (viewer_node_ == nullptr) {
    return 0;
  }

  return viewer_node_->Length();
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
  if (cache_queue_.isEmpty()) {
    if (AllProcessorsAreAvailable()) {
      emit QueueComplete();
    }

    return;
  }

  if (!Init()
      || !ViewerIsConnected()
      || !CanRender()) {
    return;
  }

  if ((input_update_queued_ || recompile_queued_) && !AllProcessorsAreAvailable()) {
    return;
  }

  if (recompile_queued_) {
    Decompile();
    recompile_queued_ = false;
  }

  if (!compiled_ && !Compile()) {
    return;
  }

  if (input_update_queued_) {
    for (int i=0;i<source_node_list_.size();i++) {
      Node* src = source_node_list_.at(i);
      Node* dst = copied_graph_.nodes().at(i);

      Node::CopyInputs(src, dst, false);
    }

    input_update_queued_ = false;
  }

  Node* node_connected_to_viewer = GetDependentInput()->get_connected_node();

  if (!node_connected_to_viewer) {
    return;
  }

  foreach (RenderWorker* worker, processors_) {
    if (cache_queue_.isEmpty()) {
      break;
    }

    if (!WorkerIsBusy(worker)) {
      TimeRange cache_frame = PopNextFrameFromQueue();

      NodeDependency dep = NodeDependency(node_connected_to_viewer,
                                          cache_frame);

      // Timestamp this render job
      qint64 job_time = QDateTime::currentMSecsSinceEpoch();

      // Ensure the job's time is unique (since that's the whole point)
      // NOTE: This value will be 0 if it doesn't exist, which will never be the result of currentMSecsSinceEpoch so we
      //       can safely assume 0 means it doesn't exist.
      qint64 existing_job_time = render_job_info_.value(cache_frame);

      if (existing_job_time == job_time) {
        job_time = existing_job_time + 1;
      }

      render_job_info_.insert(cache_frame, job_time);

      SetWorkerBusyState(worker, true);
      cancel_dialog_->WorkerStarted();

      QMetaObject::invokeMethod(worker,
                                "Render",
                                Qt::QueuedConnection,
                                Q_ARG(NodeDependency, dep),
                                Q_ARG(qint64, job_time));
    }
  }
}

ViewerOutput *RenderBackend::viewer_node() const
{
  return copied_viewer_node_;
}

void RenderBackend::CancelQueue()
{
  cache_queue_.clear();

  int busy = 0;
  for (int i=0;i<processor_busy_state_.size();i++) {
    if (processor_busy_state_.at(i))
      busy++;
  }
  qDebug() << this << "is waiting for" << busy << "busy workers";

  cancel_dialog_->RunIfWorkersAreBusy();
}

bool RenderBackend::ViewerIsConnected() const
{
  return viewer_node_ != nullptr;
}

const QString &RenderBackend::cache_id() const
{
  return cache_id_;
}

void RenderBackend::QueueValueUpdate()
{
  input_update_queued_ = true;
}

bool RenderBackend::WorkerIsBusy(RenderWorker *worker) const
{
  return processor_busy_state_.at(processors_.indexOf(worker));
}

void RenderBackend::SetWorkerBusyState(RenderWorker *worker, bool busy)
{
  processor_busy_state_.replace(processors_.indexOf(worker), busy);
}

bool RenderBackend::AllProcessorsAreAvailable() const
{
  foreach (bool busy, processor_busy_state_) {
    if (busy) {
      return false;
    }
  }

  return true;
}

const QVector<QThread *> &RenderBackend::threads()
{
  return threads_;
}

void RenderBackend::InvalidateCacheInternal(const rational &start_range, const rational &end_range)
{
  // Add the range to the list
  cache_queue_.InsertTimeRange(TimeRange(start_range, end_range));

  CacheNext();
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
    ConnectWorkerToThis(processor);

    // Connect cancel dialog to it
    connect(processor, &RenderWorker::CompletedCache, cancel_dialog_, &RenderCancelDialog::WorkerDone, Qt::QueuedConnection);

    // Finally, we can move it to its own thread
    processor->moveToThread(thread);

    // This function blocks the main thread intentionally. See the documentation for this function to see why.
    processor->Init();
  }

  processor_busy_state_.resize(processors_.size());
  processor_busy_state_.fill(false);
}

void RenderBackend::QueueRecompile()
{
  recompile_queued_ = true;
}
