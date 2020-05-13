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

#include "renderbackend.h"

#include <QDateTime>
#include <QThread>

#include "core.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  started_(false),
  viewer_node_(nullptr),
  copied_viewer_node_(nullptr)
{
  // FIXME: Don't create in CLI mode
  cancel_dialog_ = new RenderCancelDialog(Core::instance()->main_window());
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
    thread->start(QThread::IdlePriority);
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

  SetViewerNode(nullptr);

  CloseInternal();

  for (int i=0;i<processors_.size();i++) {
    // Invoke close and quit signals on processor and thread
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
  if (viewer_node_) {
    CancelQueue();

    DisconnectViewer(viewer_node_);

    copied_graph_.Clear();
    copied_viewer_node_ = nullptr;
    node_copy_map_.clear();
  }

  viewer_node_ = viewer_node;

  if (viewer_node_) {
    ConnectViewer(viewer_node_);

    RegenerateCacheID();

    copied_viewer_node_ = static_cast<ViewerOutput*>(viewer_node_->copy());
    copied_graph_.AddNode(copied_viewer_node_);
    node_copy_map_.insert(viewer_node_, copied_viewer_node_);

    InvalidateCache(TimeRange(0, RATIONAL_MAX),
                    static_cast<NodeInput*>(viewer_node_->GetInputWithID(GetDependentInput()->id())));
  }
}

bool RenderBackend::IsInitiated()
{
  return started_;
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

bool RenderBackend::InitInternal()
{
  return true;
}

void RenderBackend::CloseInternal()
{
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

  if (!ViewerIsConnected()
      || !CanRender()
      || !Init()) {
    return;
  }

  while (!input_update_queued_.isEmpty()) {
    if (!AllProcessorsAreAvailable()) {
      // To update the inputs, we need all workers to stop
      return;
    }

    CopyNodeInputValue(input_update_queued_.takeFirst());
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

      WorkerAboutToStartEvent(worker);

      QMetaObject::invokeMethod(worker,
                                "Render",
                                Qt::QueuedConnection,
                                OLIVE_NS_ARG(NodeDependency, dep),
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

  if (busy) {
    qDebug() << this << "is waiting for" << busy << "busy workers";
  }

  cancel_dialog_->RunIfWorkersAreBusy();
}

void RenderBackend::InvalidateVisible(const TimeRange &range, NodeInput *from)
{
  InvalidateCacheVeryInternal(range, from, true);
}

void RenderBackend::InvalidateCache(const TimeRange &range, NodeInput *from)
{
  InvalidateCacheVeryInternal(range, from, false);
}

bool RenderBackend::ViewerIsConnected() const
{
  return viewer_node_;
}

const QString &RenderBackend::cache_id() const
{
  return cache_id_;
}

void RenderBackend::QueueValueUpdate(NodeInput* from)
{
  if (!input_update_queued_.isEmpty()) {
    // Remove any inputs that are dependents of this input since they may have been removed since
    // it was queued
    QList<Node*> deps = from->GetDependencies();

    for (int i=0;i<input_update_queued_.size();i++) {
      if (deps.contains(input_update_queued_.at(i)->parentNode())) {
        // We don't need to queue this value since this input supersedes it
        input_update_queued_.removeAt(i);
        i--;
      }
    }
  }

  input_update_queued_.append(from);
}

bool RenderBackend::WorkerIsBusy(RenderWorker *worker) const
{
  return processor_busy_state_.at(processors_.indexOf(worker));
}

void RenderBackend::SetWorkerBusyState(RenderWorker *worker, bool busy)
{
  processor_busy_state_.replace(processors_.indexOf(worker), busy);
}

void RenderBackend::InvalidateCacheVeryInternal(const TimeRange &range, NodeInput *from, bool only_visible)
{
  // Adjust range to min/max values
  rational start_range_adj = qMax(rational(0), range.in());
  rational end_range_adj = qMin(GetSequenceLength(), range.out());

  qDebug() << "Cache invalidated between"
           << start_range_adj.toDouble()
           << "and"
           << end_range_adj.toDouble();

  if (from) {
    // Queue value update
    qDebug() << "  from" << from->parentNode()->id() << "::" << from->id();
    QueueValueUpdate(from);
  }

  InvalidateCacheInternal(start_range_adj, end_range_adj, only_visible);
}

void RenderBackend::CopyNodeInputValue(NodeInput *input)
{
  // Find our copy of this parameter
  Node* our_copy_node = node_copy_map_.value(input->parentNode());
  NodeInput* our_copy = our_copy_node->GetInputWithID(input->id());

  // Copy the standard/keyframe values between these two inputs
  NodeInput::CopyValues(input,
                        our_copy,
                        false);

  // Handle connections
  if (input->IsConnected() || our_copy->IsConnected()) {
    // If one of the inputs is connected, it's likely this change came from connecting or
    // disconnecting whatever was connected to it

    {
      // We start by removing all old dependencies from the map
      QList<Node*> old_deps = our_copy->GetExclusiveDependencies();

      foreach (Node* i, old_deps) {
        Node* n = node_copy_map_.take(node_copy_map_.key(i));
        copied_graph_.TakeNode(n);
        delete n;
      }

      // And clear any other edges
      while (!our_copy->edges().isEmpty()) {
        NodeParam::DisconnectEdge(our_copy->edges().first());
      }
    }

    // Then we copy all node dependencies and connections (if there are any)
    CopyNodeMakeConnection(input, our_copy);
  }

  // Call on sub-elements too
  if (input->IsArray()) {
    foreach (NodeInput* i, static_cast<NodeInputArray*>(input)->sub_params()) {
      CopyNodeInputValue(i);
    }
  }
}

Node* RenderBackend::CopyNodeConnections(Node* src_node)
{
  // Check if this node is already in the map
  Node* dst_node = node_copy_map_.value(src_node);

  // If not, create it now
  if (!dst_node) {
    dst_node = src_node->copy();
    copied_graph_.AddNode(dst_node);
    node_copy_map_.insert(src_node, dst_node);
  }

  // Make sure its values are copied
  Node::CopyInputs(src_node, dst_node, false);

  // Copy all connections
  QList<NodeInput*> src_node_inputs = src_node->GetInputsIncludingArrays();
  QList<NodeInput*> dst_node_inputs = dst_node->GetInputsIncludingArrays();

  for (int i=0;i<src_node_inputs.size();i++) {
    NodeInput* src_input = src_node_inputs.at(i);

    CopyNodeMakeConnection(src_input, dst_node_inputs.at(i));
  }

  return dst_node;
}

void RenderBackend::CopyNodeMakeConnection(NodeInput* src_input, NodeInput* dst_input)
{
  //qDebug() << "Copying input" << src_input->id() << "from" << src_input->parentNode()->id();

  if (src_input->IsConnected()) {
    Node* dst_node = CopyNodeConnections(src_input->get_connected_node());

    NodeOutput* corresponding_output = dst_node->GetOutputWithID(src_input->get_connected_output()->id());

    NodeParam::ConnectEdge(corresponding_output,
                           dst_input);
  }
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

void RenderBackend::InvalidateCacheInternal(const rational &start_range, const rational &end_range, bool only_visible)
{
  Q_UNUSED(only_visible)

  // Add the range to the list
  cache_queue_.InsertTimeRange(TimeRange(start_range, end_range));

  CacheNext();
}

void RenderBackend::CacheIDChangedEvent(const QString &id)
{
  Q_UNUSED(id)
}

void RenderBackend::WorkerAboutToStartEvent(RenderWorker *worker)
{
  Q_UNUSED(worker)
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

bool RenderBackend::FootageWaitInfo::operator==(const RenderBackend::FootageWaitInfo &rhs) const
{
  return rhs.stream == stream
      && rhs.stream_time == stream_time
      && rhs.affected_range == affected_range;
}

OLIVE_NAMESPACE_EXIT
