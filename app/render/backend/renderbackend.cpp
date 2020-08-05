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

#include "config/config.h"
#include "core.h"
#include "task/conform/conform.h"
#include "task/taskmanager.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

QVector<RenderBackend*> RenderBackend::instances_;
QMutex RenderBackend::instance_lock_;
RenderBackend* RenderBackend::active_instance_ = nullptr;
QThreadPool RenderBackend::thread_pool_;

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  viewer_node_(nullptr),
  autocache_enabled_(false),
  autocache_paused_(false),
  generate_audio_previews_(false),
  render_mode_(RenderMode::kOnline),
  autocache_has_changed_(false),
  use_custom_autocache_range_(false)
{
  instance_lock_.lock();
  instances_.append(this);
  instance_lock_.unlock();

  // Set default autocache range
  SetAutoCachePlayhead(rational());
}

RenderBackend::~RenderBackend()
{
  Close();
}

void RenderBackend::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ == viewer_node) {
    return;
  }

  ViewerOutput* old_viewer = viewer_node_;
  if (!viewer_node) {
    // If setting to null, set it here before we wait for jobs to finish to prevent WorkerFinished()
    // from calling RunNextJob() again and preventing us from finishing
    viewer_node_ = nullptr;
  }

  if (old_viewer) {
    // Cancel any remaining tickets
    ClearQueue();

    // Wait for any currently running jobs to finish
    foreach (RenderTicketPtr ticket, running_tickets_) {
      ticket->WaitForFinished();
    }

    // Delete all of our copied nodes
    foreach (Node* c, copy_map_) {
      c->deleteLater();
    }
    copy_map_.clear();
    copied_viewer_node_ = nullptr;
    graph_update_queue_.clear();

    // Disconnect signal (will be a no-op if the signal was never connected)
    disconnect(old_viewer,
               &ViewerOutput::GraphChangedFrom,
               this,
               &RenderBackend::NodeGraphChanged);

    disconnect(old_viewer->video_frame_cache(),
               &PlaybackCache::Invalidated,
               this,
               &RenderBackend::AutoCacheVideoInvalidated);

    disconnect(old_viewer->audio_playback_cache(),
               &PlaybackCache::Invalidated,
               this,
               &RenderBackend::AutoCacheAudioInvalidated);
  }

  if (viewer_node) {
    // If setting to non-null, set it now
    viewer_node_ = viewer_node;

    // Copy graph
    copied_viewer_node_ = static_cast<ViewerOutput*>(viewer_node_->copy());
    copy_map_.insert(viewer_node_, copied_viewer_node_);

    NodeGraphChanged(viewer_node_->texture_input());
    NodeGraphChanged(viewer_node_->samples_input());
    ProcessUpdateQueue();

    if (autocache_enabled_) {
      connect(viewer_node_,
              &ViewerOutput::GraphChangedFrom,
              this,
              &RenderBackend::NodeGraphChanged);

      connect(viewer_node_->video_frame_cache(),
              &PlaybackCache::Invalidated,
              this,
              &RenderBackend::AutoCacheVideoInvalidated);

      connect(viewer_node_->audio_playback_cache(),
              &PlaybackCache::Invalidated,
              this,
              &RenderBackend::AutoCacheAudioInvalidated);
    }
  }
}

void RenderBackend::AutoCacheRange(const TimeRange &range)
{
  Q_ASSERT(autocache_enabled_);

  autocache_has_changed_ = true;
  use_custom_autocache_range_ = true;
  custom_autocache_range_ = range;

  AutoCacheRequeueFrames();
}

RenderTicketPtr RenderBackend::Hash(const QVector<rational> &times, bool prioritize)
{
  Q_ASSERT(viewer_node_);

  SetActiveInstance();

  RenderTicketPtr ticket = std::make_shared<RenderTicket>(RenderTicket::kTypeHash,
                                                          QVariant::fromValue(times));

  if (prioritize) {
    render_queue_.push_front(ticket);
  } else {
    render_queue_.push_back(ticket);
  }

  QMetaObject::invokeMethod(this, "RunNextJob", Qt::QueuedConnection);

  return ticket;
}

RenderTicketPtr RenderBackend::RenderFrame(const rational &time, bool prioritize, const QByteArray& hash)
{
  Q_ASSERT(viewer_node_);

  SetActiveInstance();

  RenderTicketPtr ticket = std::make_shared<RenderTicket>(RenderTicket::kTypeVideo,
                                                          QVariant::fromValue(time));

  ticket->setProperty("hash", hash);

  if (prioritize) {
    render_queue_.push_front(ticket);
  } else {
    render_queue_.push_back(ticket);
  }

  QMetaObject::invokeMethod(this, "RunNextJob", Qt::QueuedConnection);

  return ticket;
}

RenderTicketPtr RenderBackend::RenderAudio(const TimeRange &r, bool prioritize)
{
  Q_ASSERT(viewer_node_);

  SetActiveInstance();

  RenderTicketPtr ticket = std::make_shared<RenderTicket>(RenderTicket::kTypeAudio,
                                                          QVariant::fromValue(r));

  if (prioritize) {
    render_queue_.push_front(ticket);
  } else {
    render_queue_.push_back(ticket);
  }

  QMetaObject::invokeMethod(this, "RunNextJob", Qt::QueuedConnection);

  return ticket;
}

void RenderBackend::SetVideoParams(const VideoParams &params)
{
  video_params_ = params;
}

void RenderBackend::SetAudioParams(const AudioParams &params)
{
  audio_params_ = params;
}

void RenderBackend::SetVideoDownloadMatrix(const QMatrix4x4 &mat)
{
  video_download_matrix_ = mat;
}

std::list<TimeRange> RenderBackend::SplitRangeIntoChunks(const TimeRange &r)
{
  // FIXME: Magic number
  const int chunk_size = 2;

  std::list<TimeRange> split_ranges;

  int start_time = qFloor(r.in().toDouble() / static_cast<double>(chunk_size)) * chunk_size;
  int end_time = qCeil(r.out().toDouble() / static_cast<double>(chunk_size)) * chunk_size;

  for (int i=start_time; i<end_time; i+=chunk_size) {
    split_ranges.push_back(TimeRange(qMax(r.in(), rational(i)),
                                     qMin(r.out(), rational(i + chunk_size))));
  }

  return split_ranges;
}

void RenderBackend::ClearVideoQueue()
{
  ClearQueueOfType(RenderTicket::kTypeVideo);

  autocache_has_changed_ = true;
  use_custom_autocache_range_ = false;
}

void RenderBackend::ClearAudioQueue()
{
  ClearQueueOfType(RenderTicket::kTypeAudio);
}

void RenderBackend::ClearQueue()
{
  foreach (RenderTicketPtr t, render_queue_) {
    t->Cancel();
  }
  render_queue_.clear();
}

void RenderBackend::NodeGraphChanged(NodeInput *source)
{
  // We need to determine:
  // - If we don't have this input, assume that it's coming soon and ignore it
  // - If we do, is this input a child of another input we're already copying?
  // - Or are any of the queued inputs children of this one?

  // First we need to find our copy of the input being queued
  Node* our_copy_node = copy_map_.value(source->parentNode());

  // If we don't have this node yet, assume it's coming in a later copy in which case it'll be
  // copied then
  if (!our_copy_node) {
    // Assert that there are updates coming
    Q_ASSERT(!graph_update_queue_.isEmpty());
    return;
  }

  // If we're here, we must have this node. Determine if we're already copying a "parent" of this
  for (int i=0; i<graph_update_queue_.size(); i++) {
    NodeInput* queued_input = graph_update_queue_.at(i);

    // If this input is already queued, nothing to be done
    if (source == queued_input) {
      return;
    }

    // Check if this dependency graph is already queued
    if (source->parentNode()->OutputsTo(queued_input, true, true)) {
      // In which case, no further copy is necessary
      return;
    }

    // Check if the source is a member of this array, in which case it'll be copied eventually anyway
    if (queued_input->IsArray()
        && static_cast<NodeInputArray*>(queued_input)->sub_params().contains(source)) {
      return;
    }

    // Check if this input supersedes an already queued input
    if (queued_input->parentNode()->OutputsTo(source, true, true)
        || (source->IsArray() && static_cast<NodeInputArray*>(source)->sub_params().contains(queued_input))) {
      // In which case, we don't need to queue it and can queue our own
      graph_update_queue_.removeAt(i);
      i--;
    }
  }

  graph_update_queue_.append(source);
}

void RenderBackend::Close()
{
  SetViewerNode(nullptr);

  for (int i=0;i<workers_.size();i++) {
    workers_.at(i).worker->deleteLater();
  }
  workers_.clear();
}

void RenderBackend::RunNextJob()
{
  // If queue is empty, nothing to be done
  if (render_queue_.empty()) {

    // If we're the active instance, unset it
    instance_lock_.lock();
    if (active_instance_ == this) {
      active_instance_ = nullptr;
    }
    instance_lock_.unlock();

    return;
  }

  // Check if params are valid
  if (!video_params_.is_valid()
      || !audio_params_.is_valid()) {
    qDebug() << "Failed to run job, parameters are invalid";
    return;
  }

  // If we have a value update queued, check if all workers are available and proceed from there
  if (autocache_enabled_ && !graph_update_queue_.isEmpty()) {
    bool all_workers_available = true;

    foreach (const WorkerData& data, workers_) {
      if (data.busy) {
        all_workers_available = false;
        break;
      }
    }

    if (all_workers_available) {
      // Process queue
      ProcessUpdateQueue();
    } else {
      return;
    }
  }

  // If we have no workers allocated, allocate them now
  if (workers_.isEmpty()) {
    // Allocate workers here
    workers_.resize(thread_pool_.maxThreadCount());

    for (int i=0;i<workers_.size();i++) {
      RenderWorker* worker = CreateNewWorker();

      connect(worker, &RenderWorker::WaveformGenerated, this, &RenderBackend::WorkerGeneratedWaveform);
      connect(worker, &RenderWorker::FinishedJob, this, &RenderBackend::WorkerFinished);

      workers_.replace(i, {worker, false});
    }
  }

  // Start popping jobs off the queue
  for (int i=0;i<workers_.size();i++) {
    if (!workers_.at(i).busy) {
      // This worker is available, send it the job

      RenderWorker* worker = workers_[i].worker;

      workers_[i].busy = true;

      worker->SetVideoParams(video_params_);
      worker->SetAudioParams(audio_params_);
      worker->SetVideoDownloadMatrix(video_download_matrix_);
      worker->SetRenderMode(render_mode_);
      worker->SetPreviewGenerationEnabled(generate_audio_previews_);
      worker->SetCopyMap(&copy_map_);

      // Move ticket from queue to running list
      RenderTicketPtr ticket = render_queue_.front();
      render_queue_.pop_front();
      running_tickets_.push_back(ticket);

      // Create watcher to remove from running list
      RenderTicketWatcher* watcher = new RenderTicketWatcher();
      connect(watcher, &RenderTicketWatcher::Finished, this, &RenderBackend::TicketFinished);
      watcher->SetTicket(ticket);

      // Set job time to now
      ticket->SetJobTime();

      switch (ticket->GetType()) {
      case RenderTicket::kTypeHash:
        QtConcurrent::run(&thread_pool_,
                          worker,
                          &RenderWorker::Hash,
                          ticket,
                          copied_viewer_node_,
                          ticket->GetTime().value<QVector<rational> >());
        break;
      case RenderTicket::kTypeVideo:
      {
        rational frame = ticket->GetTime().value<rational>();

        QtConcurrent::run(&thread_pool_,
                          worker,
                          &RenderWorker::RenderFrame,
                          ticket,
                          copied_viewer_node_,
                          frame);

        QByteArray frame_hash = ticket->property("hash").toByteArray();
        if (!frame_hash.isEmpty()) {
          currently_caching_hashes_.append(frame_hash);
        }
        break;
      }
      case RenderTicket::kTypeAudio:
        QtConcurrent::run(&thread_pool_,
                          worker,
                          &RenderWorker::RenderAudio,
                          ticket,
                          copied_viewer_node_,
                          ticket->GetTime().value<TimeRange>());
        break;
      }

      if (render_queue_.empty()) {
        // No more jobs, can exit here
        break;
      }
    }
  }
}

void RenderBackend::TicketFinished()
{
  RenderTicketPtr ticket = static_cast<RenderTicketWatcher*>(sender())->GetTicket();
  delete sender();

  running_tickets_.remove(ticket);
}

void RenderBackend::WorkerGeneratedWaveform(RenderTicketPtr ticket, TrackOutput *track, AudioVisualWaveform samples, TimeRange range)
{
  qDebug() << "Hello?";

  QList<TimeRange> valid_ranges = viewer_node_->audio_playback_cache()->GetValidRanges(range,
                                                                                       ticket->GetJobTime());
  if (!valid_ranges.isEmpty()) {
    // Generate visual waveform in this background thread
    track->waveform_lock()->lock();

    track->waveform().set_channel_count(audio_params_.channel_count());

    foreach (const TimeRange& r, valid_ranges) {
      track->waveform().OverwriteSums(samples, r.in(), r.in() - range.in(), r.length());
    }

    track->waveform_lock()->unlock();

    emit track->PreviewChanged();
  }
}

void RenderBackend::AutoCacheVideoInvalidated(const TimeRange &range)
{
  ClearVideoQueue();

  // Hash these frames since that should be relatively quick.
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  QVector<rational> frames = viewer_node_->video_frame_cache()->GetFrameListFromTimeRange({range});
  autocache_hash_tasks_.insert(watcher, frames);
  connect(watcher, &RenderTicketWatcher::Finished, this, &RenderBackend::AutoCacheHashesGenerated);
  watcher->SetTicket(Hash(frames));
}

void RenderBackend::AutoCacheAudioInvalidated(const TimeRange &range)
{
  // Start a task to re-render the audio at this range
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  autocache_audio_tasks_.insert(watcher, range);
  connect(watcher, &RenderTicketWatcher::Finished, this, &RenderBackend::AutoCacheAudioRendered);
  watcher->SetTicket(RenderAudio(range, true));
}

void RenderBackend::SetHashes(FrameHashCache* cache, const QVector<rational>& times, const QVector<QByteArray>& hashes, qint64 job_time)
{
  std::vector<QByteArray> existing_hashes;

  for (int i=0; i<times.size(); i++) {
    // See if hash already exists in disk cache
    const QByteArray& hash = hashes.at(i);
    const rational& time = times.at(i);

    // Check memory list since disk checking is slow
    bool hash_exists = (std::find(existing_hashes.begin(), existing_hashes.end(), hash) != existing_hashes.end());

    if (!hash_exists) {
      hash_exists = QFileInfo::exists(cache->CachePathName(hash));

      if (hash_exists) {
        existing_hashes.push_back(hash);
      }
    }

    cache->SetHash(time, hash, job_time, hash_exists);
  }
}

void RenderBackend::AutoCacheHashesGenerated()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (autocache_hash_tasks_.contains(watcher)) {
    if (!watcher->WasCancelled()) {
      QFutureWatcher<void>* hw = new QFutureWatcher<void>();
      connect(hw, &QFutureWatcher<void>::finished, this, &RenderBackend::AutoCacheHashesProcessed);
      autocache_hash_process_tasks_.append(hw);
      hw->setFuture(QtConcurrent::run(this,
                                      &RenderBackend::SetHashes,
                                      viewer_node_->video_frame_cache(),
                                      autocache_hash_tasks_.value(watcher),
                                      watcher->Get().value<QVector<QByteArray> >(),
                                      watcher->GetTicket()->GetJobTime()));
    }

    autocache_hash_tasks_.remove(watcher);
  }

  delete watcher;
}

void RenderBackend::AutoCacheHashesProcessed()
{
  QFutureWatcher<void>* watcher = static_cast<QFutureWatcher<void>*>(sender());

  if (autocache_hash_process_tasks_.contains(watcher)) {
    autocache_hash_process_tasks_.removeOne(watcher);

    AutoCacheRequeueFrames();
  }

  delete watcher;
}

void RenderBackend::AutoCacheAudioRendered()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (autocache_audio_tasks_.contains(watcher)) {
    if (!watcher->WasCancelled()) {
      viewer_node_->audio_playback_cache()->WritePCM(autocache_audio_tasks_.value(watcher),
                                                     watcher->Get().value<SampleBufferPtr>(),
                                                     watcher->GetTicket()->GetJobTime());
    }

    autocache_audio_tasks_.remove(watcher);
  }

  delete watcher;
}

void RenderBackend::AutoCacheVideoRendered()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (autocache_video_tasks_.contains(watcher)) {
    if (!watcher->WasCancelled()) {
      const QByteArray& hash = autocache_video_tasks_.value(watcher);

      // Download frame in another thread
      QFutureWatcher<bool>* w = new QFutureWatcher<bool>();
      autocache_video_download_tasks_.insert(w, hash);
      connect(w, &QFutureWatcher<bool>::finished, this, &RenderBackend::AutoCacheVideoDownloaded);
      w->setFuture(QtConcurrent::run(FrameHashCache::SaveCacheFrame,
                                     hash,
                                     watcher->Get().value<FramePtr>()));
    }

    autocache_video_tasks_.remove(watcher);
  }

  delete watcher;
}

void RenderBackend::AutoCacheVideoDownloaded()
{
  QFutureWatcher<bool>* watcher = static_cast<QFutureWatcher<bool>*>(sender());

  if (autocache_video_download_tasks_.contains(watcher)) {
    if (!watcher->isCanceled()) {
      if (watcher->result()) {
        const QByteArray& hash = autocache_video_download_tasks_.value(watcher);

        currently_caching_hashes_.removeOne(hash);

        viewer_node_->video_frame_cache()->ValidateFramesWithHash(hash);
      } else {
        qCritical() << "Failed to download video frame";
      }
    }

    autocache_video_download_tasks_.remove(watcher);
  }

  delete watcher;
}

//#define PRINT_UPDATE_QUEUE_INFO
void RenderBackend::ProcessUpdateQueue()
{
#ifdef PRINT_UPDATE_QUEUE_INFO
  qint64 t = QDateTime::currentMSecsSinceEpoch();
  qDebug() << "Processing update queue of" << graph_update_queue_.size() << "elements:";
#endif

  while (!graph_update_queue_.isEmpty()) {
    NodeInput* i = graph_update_queue_.takeFirst();
#ifdef PRINT_UPDATE_QUEUE_INFO
    qDebug() << " " << i->parentNode()->id() << i->id();
#endif
    CopyNodeInputValue(i);
  }

#ifdef PRINT_UPDATE_QUEUE_INFO
  qDebug() << "Update queue took:" << (QDateTime::currentMSecsSinceEpoch() - t);
#endif
}

void RenderBackend::WorkerFinished()
{
  RenderWorker* worker = static_cast<RenderWorker*>(sender());

  // Set busy state to false
  for (int i=0;i<workers_.size();i++) {
    if (workers_.at(i).worker == worker) {
      workers_[i].busy = false;
      break;
    }
  }

  if (viewer_node_) {
    RunNextJob();
  }
}

void RenderBackend::CopyNodeInputValue(NodeInput *input)
{
  // Find our copy of this parameter
  Node* our_copy_node = copy_map_.value(input->parentNode());
  Q_ASSERT(our_copy_node);
  NodeInput* our_copy = our_copy_node->GetInputWithID(input->id());

  // Copy the standard/keyframe values between these two inputs
  NodeInput::CopyValues(input,
                        our_copy,
                        false,
                        false);

  // Handle connections
  if (input->is_connected() || our_copy->is_connected()) {
    // If one of the inputs is connected, it's likely this change came from connecting or
    // disconnecting whatever was connected to it

    // We start by removing all old dependencies from the map
    QList<Node*> old_deps = our_copy->GetExclusiveDependencies();
    foreach (Node* i, old_deps) {
      copy_map_.take(copy_map_.key(i))->deleteLater();
    }

    // And clear any other edges
    while (!our_copy->edges().isEmpty()) {
      NodeParam::DisconnectEdge(our_copy->edges().first());
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
  Node* dst_node = copy_map_.value(src_node);

  // If not, create it now
  if (!dst_node) {
    dst_node = src_node->copy();

    if (dst_node->IsTrack()) {
      // Hack that ensures the track type is set since we don't bother copying the whole timeline
      static_cast<TrackOutput*>(dst_node)->set_track_type(static_cast<TrackOutput*>(src_node)->track_type());
    }

    copy_map_.insert(src_node, dst_node);
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
  if (src_input->is_connected()) {
    Node* dst_node = CopyNodeConnections(src_input->get_connected_node());

    NodeOutput* corresponding_output = dst_node->GetOutputWithID(src_input->get_connected_output()->id());

    NodeParam::ConnectEdge(corresponding_output,
                           dst_input);
  }
}

void RenderBackend::ClearQueueOfType(RenderTicket::Type type)
{
  std::list<RenderTicketPtr>::iterator i = render_queue_.begin();

  while (i != render_queue_.end()) {
    if ((*i)->GetType() == type) {
      (*i)->Cancel();
      i = render_queue_.erase(i);
    } else {
      i++;
    }
  }
}

void RenderBackend::SetActiveInstance()
{
  QMutexLocker locker(&instance_lock_);

  if (active_instance_ != this) {
    // Signal active instance to stop
    QMetaObject::invokeMethod(active_instance_, "ClearVideoQueue", Qt::QueuedConnection);

    active_instance_ = this;
  }
}

void RenderBackend::AutoCacheRequeueFrames()
{
  if (viewer_node_
      && viewer_node_->video_frame_cache()->HasInvalidatedRanges()
      && autocache_hash_tasks_.isEmpty()
      && autocache_hash_process_tasks_.isEmpty()
      && autocache_has_changed_
      && (!autocache_paused_ || use_custom_autocache_range_)) {
    TimeRange using_range;

    if (use_custom_autocache_range_) {
      using_range = custom_autocache_range_;
      use_custom_autocache_range_ = false;
    } else {
      using_range = autocache_range_;
    }

    QVector<rational> invalidated_ranges = viewer_node_->video_frame_cache()->GetInvalidatedFrames(using_range);

    ClearVideoQueue();

    // QMaps are automatically sorted by time which is always best for rendering
    QList<QByteArray> queued_hashes;

    foreach (const rational& t, invalidated_ranges) {
      const QByteArray& hash = viewer_node_->video_frame_cache()->GetHash(t);

      if (t >= using_range.in()
          && t < using_range.out()
          && !queued_hashes.contains(hash)
          && !currently_caching_hashes_.contains(hash)) {
        // Don't render any hash more than once
        queued_hashes.append(hash);

        RenderTicketWatcher* watcher = new RenderTicketWatcher();
        connect(watcher, &RenderTicketWatcher::Finished, this, &RenderBackend::AutoCacheVideoRendered);
        autocache_video_tasks_.insert(watcher, hash);

        watcher->SetTicket(RenderFrame(t, false, hash));
      }
    }

    autocache_has_changed_ = false;
  }
}

OLIVE_NAMESPACE_EXIT
