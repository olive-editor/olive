#include "previewautocacher.h"

#include <QApplication>
#include <QtConcurrent/QtConcurrent>

#include "codec/conformmanager.h"
#include "node/project/project.h"
#include "render/rendermanager.h"
#include "render/renderprocessor.h"

namespace olive {

PreviewAutoCacher::PreviewAutoCacher() :
  viewer_node_(nullptr),
  has_changed_(false),
  use_custom_range_(false),
  single_frame_render_(nullptr),
  last_update_time_(0),
  ignore_next_mouse_button_(false),
  last_conform_task_(0)
{
  paused_ = !Config::Current()[QStringLiteral("AutoCacheEnabled")].toBool(),

  SetPlayhead(0);

  delayed_requeue_timer_.setInterval(Config::Current()[QStringLiteral("AutoCacheDelay")].toInt());
  delayed_requeue_timer_.setSingleShot(true);
  connect(&delayed_requeue_timer_, &QTimer::timeout, this, &PreviewAutoCacher::RequeueFrames);

  connect(ConformManager::instance(), &ConformManager::ConformReady, this, &PreviewAutoCacher::ConformFinished);
}

PreviewAutoCacher::~PreviewAutoCacher()
{
  // Ensure everything is cleaned up appropriately
  SetViewerNode(nullptr);
}

RenderTicketPtr PreviewAutoCacher::GetSingleFrame(const rational &t, bool prioritize)
{
  CancelQueuedSingleFrameRender();

  QByteArray hash;
  if (!paused_) {
    hash = viewer_node_->video_frame_cache()->GetHash(t);
  }

  auto sfr = std::make_shared<RenderTicket>();
  sfr->Start();
  sfr->setProperty("time", QVariant::fromValue(t));
  sfr->setProperty("prioritize", prioritize);
  sfr->setProperty("hash", hash);

  // Attempt to queue
  single_frame_render_ = sfr;
  TryRender();

  return sfr;
}

void PreviewAutoCacher::SetPaused(bool paused)
{
  paused_ = paused;
}

void GenerateHashesInternal(ViewerOutput *viewer, FrameHashCache* cache, const QVector<rational> &times, qint64 job_time)
{
  std::vector<QByteArray> existing_hashes;

  foreach (const rational& time, times) {
    // See if hash already exists in disk cache
    QByteArray hash = RenderManager::Hash(viewer->GetConnectedTextureOutput(), viewer->GetVideoParams(), time);

    // Check memory list since disk checking is slow
    bool hash_exists = (std::find(existing_hashes.begin(), existing_hashes.end(), hash) != existing_hashes.end());

    if (!hash_exists) {
      hash_exists = QFileInfo::exists(cache->CachePathName(hash));

      if (hash_exists) {
        existing_hashes.push_back(hash);
      }
    }

    // Set hash in FrameHashCache's thread rather than in ours to prevent race conditions
    QMetaObject::invokeMethod(cache, "SetHash", Qt::QueuedConnection,
                              OLIVE_NS_ARG(rational, time),
                              Q_ARG(QByteArray, hash),
                              Q_ARG(qint64, job_time),
                              Q_ARG(bool, hash_exists));
  }
}

void PreviewAutoCacher::GenerateHashes(ViewerOutput *viewer, FrameHashCache* cache, const QVector<rational> &times, qint64 job_time)
{
  // Ensure number of threads doesn't exceed idealThreadCount for maximum concurrency
  int hashes_per_thread = times.size() / qMax(1, QThread::idealThreadCount()-1);

  // Somewhat arbitrary (it felt right) number used to determine when the overhead of sending this
  // to threads will exceed the benefit of multithreading
  static const int kMinimumHashesPerThread = 500;
  if (hashes_per_thread < kMinimumHashesPerThread) {
    hashes_per_thread = kMinimumHashesPerThread;
  }

  // Queue threaded tasks for each
  if (hashes_per_thread >= times.size()) {
    // Don't bother queuing in other thread, just run
    GenerateHashesInternal(viewer, cache, times, job_time);
  } else {
    QVector<QFuture<void> > threads;
    for (int i=0; i<times.size(); i+=hashes_per_thread) {
      threads.append(QtConcurrent::run(GenerateHashesInternal, viewer, cache, times.mid(i, i == times.size() - 1 ? -1 : hashes_per_thread), job_time));
    }

    for (int i=0; i<threads.size(); i++) {
      threads[i].waitForFinished();
    }
  }
}

void PreviewAutoCacher::VideoInvalidated(const TimeRange &range)
{
  ClearVideoQueue();

  // Hash these frames since that should be relatively quick.
  if (ignore_next_mouse_button_ || !(qApp->mouseButtons() & Qt::LeftButton)) {
    ignore_next_mouse_button_ = false;

    invalidated_video_.insert(range);

    TryRender();
  }
}

void PreviewAutoCacher::AudioInvalidated(const TimeRange &range)
{
//  ClearAudioQueue();

  // Start jobs to re-render the audio at this range, split into 2 second chunks
  invalidated_audio_.insert(range);

  TryRender();
}

void PreviewAutoCacher::HashesProcessed()
{
  QFutureWatcher<void>* watcher = static_cast<QFutureWatcher<void>*>(sender());

  if (hash_tasks_.contains(watcher)) {
    hash_tasks_.removeOne(watcher);

    // Restart delayed requeue timer
    delayed_requeue_timer_.stop();
    delayed_requeue_timer_.start();
  }

  // The cacher might be waiting for this job to finish
  if (!graph_update_queue_.isEmpty()) {
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::AudioRendered()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (audio_tasks_.contains(watcher)) {
    if (watcher->HasResult()) {
      const TimeRange &range = audio_tasks_.value(watcher);

      AudioVisualWaveform waveform = watcher->GetTicket()->property("waveform").value<AudioVisualWaveform>();

      viewer_node_->audio_playback_cache()->WritePCM(range,
                                                     watcher->Get().value<SampleBufferPtr>(),
                                                     &waveform,
                                                     watcher->GetTicket()->GetJobTime());

      bool pcm_is_usable = true;

      if (watcher->GetTicket()->property("incomplete").toBool()) {
        if (last_conform_task_ > watcher->GetTicket()->GetJobTime()) {
          // Requeue now
          viewer_node_->audio_playback_cache()->Invalidate(range, QDateTime::currentMSecsSinceEpoch());
          pcm_is_usable = false;
        } else {
          // Wait for conform
          audio_needing_conform_.insert(range);
        }
      }

      if (pcm_is_usable) {
        // Retrieve visual waveforms
        QVector<RenderProcessor::RenderedWaveform> waveform_list = watcher->GetTicket()->property("waveforms").value< QVector<RenderProcessor::RenderedWaveform> >();
        foreach (const RenderProcessor::RenderedWaveform& waveform_info, waveform_list) {
          // Find original track
          Track* track = nullptr;

          for (auto it=copy_map_.cbegin(); it!=copy_map_.cend(); it++) {
            if (it.value() == waveform_info.track) {
              track = static_cast<Track*>(it.key());
              break;
            }
          }

          if (track) {
            QList<TimeRange> valid_ranges = viewer_node_->audio_playback_cache()->GetValidRanges(waveform_info.range,
                                                                                                 watcher->GetTicket()->GetJobTime());
            if (!valid_ranges.isEmpty()) {
              // Generate visual waveform in this background thread
              track->waveform().set_channel_count(viewer_node_->GetAudioParams().channel_count());

              foreach (const TimeRange& r, valid_ranges) {
                track->waveform().OverwriteSums(waveform_info.waveform, r.in(), r.in() - waveform_info.range.in(), r.length());
              }

              emit track->PreviewChanged();
            }
          }
        }
      }
    }

    audio_tasks_.remove(watcher);
  }

  // The cacher might be waiting for this job to finish
  if (!graph_update_queue_.isEmpty()) {
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::VideoRendered()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (video_tasks_.contains(watcher)) {
    if (watcher->HasResult()) {
      const QByteArray& hash = video_tasks_.value(watcher);

      // Download frame in another thread
      if (!hash.isEmpty() && VideoParams::FormatIsFloat(viewer_node_->GetVideoParams().format())) {
        FramePtr frame = watcher->Get().value<FramePtr>();
        RenderTicketWatcher* w = new RenderTicketWatcher();
        w->setProperty("frame", QVariant::fromValue(frame));
        video_download_tasks_.insert(w, hash);
        connect(w, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::VideoDownloaded);
        w->SetTicket(RenderManager::instance()->SaveFrameToCache(viewer_node_->video_frame_cache(),
                                                                 frame,
                                                                 hash,
                                                                 true));
      }
    }

    video_tasks_.remove(watcher);
  }

  // Process passthroughs
  QVector<RenderTicketPtr> tickets = video_immediate_passthroughs_.take(watcher);
  foreach (RenderTicketPtr t, tickets) {
    if (watcher->HasResult()) {
      t->Finish(watcher->Get());
    } else {
      t->Finish();
    }
  }

  // The cacher might be waiting for this job to finish
  if (!graph_update_queue_.isEmpty()) {
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::VideoDownloaded()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  if (video_download_tasks_.contains(watcher)) {
    if (watcher->HasResult()) {
      if (watcher->Get().toBool()) {
        const QByteArray& hash = video_download_tasks_.value(watcher);

        viewer_node_->video_frame_cache()->ValidateFramesWithHash(hash);
      } else {
        qCritical() << "Failed to download video frame";
      }
    }

    video_download_tasks_.remove(watcher);
  }

  // The cacher might be waiting for this job to finish
  if (!graph_update_queue_.isEmpty()) {
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::ProcessUpdateQueue()
{
  foreach (const QueuedJob& job, graph_update_queue_) {
    switch (job.type) {
    case QueuedJob::kNodeAdded:
      AddNode(job.node);
      break;
    case QueuedJob::kNodeRemoved:
      RemoveNode(job.node);
      break;
    case QueuedJob::kEdgeAdded:
      AddEdge(job.output, job.input);
      break;
    case QueuedJob::kEdgeRemoved:
      RemoveEdge(job.output, job.input);
      break;
    case QueuedJob::kValueChanged:
      CopyValue(job.input);
      break;
    }
  }
  graph_update_queue_.clear();

  UpdateLastSyncedValue();
}

bool PreviewAutoCacher::HasActiveJobs() const
{
  return !hash_tasks_.isEmpty()
      || !audio_tasks_.isEmpty()
      || !video_tasks_.isEmpty();
}

void PreviewAutoCacher::AddNode(Node *node)
{
  // Copy node
  Node* copy = node->copy();

  // Add to project
  copy->setParent(&copied_project_);

  // Insert into map
  InsertIntoCopyMap(node, copy);

  // Keep track of our nodes
  created_nodes_.append(copy);
}

void PreviewAutoCacher::RemoveNode(Node *node)
{
  // Find our copy and remove it
  Node* copy = copy_map_.take(node);

  // Remove from created list
  created_nodes_.removeOne(copy);

  // Delete it
  delete copy;
}

void PreviewAutoCacher::AddEdge(const NodeOutput &output, const NodeInput &input)
{
  Node* our_output = copy_map_.value(output.node());
  Node* our_input = copy_map_.value(input.node());

  Node::ConnectEdge(NodeOutput(our_output, output.output()), NodeInput(our_input, input.input(), input.element()));
}

void PreviewAutoCacher::RemoveEdge(const NodeOutput &output, const NodeInput &input)
{
  Node* our_output = copy_map_.value(output.node());
  Node* our_input = copy_map_.value(input.node());

  Node::DisconnectEdge(NodeOutput(our_output, output.output()), NodeInput(our_input, input.input(), input.element()));
}

void PreviewAutoCacher::CopyValue(const NodeInput &input)
{
  Node* our_input = copy_map_.value(input.node());
  Node::CopyValuesOfElement(input.node(), our_input, input.input(), input.element());
}

void PreviewAutoCacher::InsertIntoCopyMap(Node *node, Node *copy)
{
  // Insert into map
  copy_map_.insert(node, copy);

  // Copy parameters
  Node::CopyInputs(node, copy, false);
}

void PreviewAutoCacher::UpdateLastSyncedValue()
{
  last_update_time_ = QDateTime::currentMSecsSinceEpoch();
}

void PreviewAutoCacher::CancelQueuedSingleFrameRender()
{
  if (single_frame_render_) {
    // Signal that this ticket was cancelled with no value
    single_frame_render_->Finish();
    single_frame_render_ = nullptr;
  }
}

void PreviewAutoCacher::SetPlayhead(const rational &playhead)
{
  cache_range_ = TimeRange(playhead - Config::Current()[QStringLiteral("DiskCacheBehind")].value<rational>(),
      playhead + Config::Current()[QStringLiteral("DiskCacheAhead")].value<rational>());

  has_changed_ = true;
  use_custom_range_ = false;

  RequeueFrames();
}

void PreviewAutoCacher::ClearHashQueue(bool wait)
{
  auto copy = hash_tasks_;

  for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
    (*it)->cancel();
  }
  if (wait) {
    copy = hash_tasks_;
    for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
      (*it)->waitForFinished();
    }

    hash_tasks_.clear();
  }
}

void PreviewAutoCacher::ClearVideoQueue(bool hard)
{
  ClearQueueInternal(video_tasks_, hard, &PreviewAutoCacher::VideoRendered);

  has_changed_ = true;
  use_custom_range_ = false;
}

void PreviewAutoCacher::ClearAudioQueue(bool hard)
{
  ClearQueueInternal(audio_tasks_, hard, &PreviewAutoCacher::AudioRendered);
}

void PreviewAutoCacher::ClearVideoDownloadQueue(bool hard)
{
  ClearQueueInternal(video_download_tasks_, hard, &PreviewAutoCacher::VideoDownloaded);
}

void PreviewAutoCacher::NodeAdded(Node *node)
{
  graph_update_queue_.append({QueuedJob::kNodeAdded, node, NodeInput(), NodeOutput()});
}

void PreviewAutoCacher::NodeRemoved(Node *node)
{
  graph_update_queue_.append({QueuedJob::kNodeRemoved, node, NodeInput(), NodeOutput()});
}

void PreviewAutoCacher::EdgeAdded(const NodeOutput &output, const NodeInput &input)
{
  graph_update_queue_.append({QueuedJob::kEdgeAdded, nullptr, input, output});
}

void PreviewAutoCacher::EdgeRemoved(const NodeOutput &output, const NodeInput &input)
{
  graph_update_queue_.append({QueuedJob::kEdgeRemoved, nullptr, input, output});
}

void PreviewAutoCacher::ValueChanged(const NodeInput &input)
{
  graph_update_queue_.append({QueuedJob::kValueChanged, nullptr, input, NodeOutput()});
}

void PreviewAutoCacher::TryRender()
{
  if (!graph_update_queue_.isEmpty()) {
    if (HasActiveJobs()) {
      // Still waiting for jobs to finish
      return;
    }

    // No jobs are active, we can process the update queue
    ProcessUpdateQueue();
  }

  // If we're here, we must be able to render
  if (!invalidated_video_.isEmpty()) {
    QVector<rational> frames = viewer_node_->video_frame_cache()->GetFrameListFromTimeRange(invalidated_video_);

    QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
    hash_tasks_.append(watcher);
    connect(watcher, &QFutureWatcher<void>::finished, this, &PreviewAutoCacher::HashesProcessed);
    watcher->setFuture(QtConcurrent::run(&PreviewAutoCacher::GenerateHashes,
                                         copied_viewer_node_,
                                         viewer_node_->video_frame_cache(),
                                         frames,
                                         last_update_time_));

    invalidated_video_.clear();
  }

  if (!invalidated_audio_.isEmpty()) {
    foreach (const TimeRange& range, invalidated_audio_) {
      std::list<TimeRange> chunks = range.Split(30);

      foreach (const TimeRange& r, chunks) {
        RenderTicketWatcher* watcher = new RenderTicketWatcher();
        connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::AudioRendered);
        audio_tasks_.insert(watcher, r);
        watcher->SetTicket(RenderManager::instance()->RenderAudio(copied_viewer_node_, r, RenderMode::kOffline, true));
      }
    }

    invalidated_audio_.clear();
  }

  if (single_frame_render_) {
    // Check if already caching this
    QByteArray hash = single_frame_render_->property("hash").toByteArray();
    RenderTicketWatcher* watcher;
    if (!hash.isEmpty() && (watcher = video_tasks_.key(hash))) {
      video_immediate_passthroughs_[watcher].append(single_frame_render_);
    } else if (!hash.isEmpty() && (watcher = video_download_tasks_.key(hash))) {
      single_frame_render_->Finish(watcher->property("frame"));
    } else {
      watcher = RenderFrame(hash,
                            single_frame_render_->property("time").value<rational>(),
                            single_frame_render_->property("prioritize").toBool(),
                            paused_);

      video_immediate_passthroughs_[watcher].append(single_frame_render_);
    }

    single_frame_render_ = nullptr;
  }
}

RenderTicketWatcher* PreviewAutoCacher::RenderFrame(const QByteArray &hash, const rational& time, bool prioritize, bool texture_only)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("hash", hash);
  connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::VideoRendered);
  video_tasks_.insert(watcher, hash);
  watcher->SetTicket(RenderManager::instance()->RenderFrame(copied_viewer_node_,
                                                            copied_color_manager_,
                                                            time,
                                                            RenderMode::kOffline,
                                                            viewer_node_->video_frame_cache(),
                                                            prioritize,
                                                            texture_only));
  return watcher;
}

void PreviewAutoCacher::RequeueFrames()
{
  delayed_requeue_timer_.stop();

  if (viewer_node_
      && viewer_node_->video_frame_cache()->HasInvalidatedRanges()
      && hash_tasks_.isEmpty()
      && has_changed_
      && VideoParams::FormatIsFloat(viewer_node_->GetVideoParams().format())
      && (!paused_ || use_custom_range_)) {
    TimeRange using_range;

    if (use_custom_range_) {
      using_range = custom_autocache_range_;
      use_custom_range_ = false;
    } else {
      using_range = cache_range_;
    }

    QVector<rational> invalidated_ranges = viewer_node_->video_frame_cache()->GetInvalidatedFrames(using_range);

    foreach (const rational& t, invalidated_ranges) {
      const QByteArray& hash = viewer_node_->video_frame_cache()->GetHash(t);

      RenderTicketWatcher* render_task = video_tasks_.key(hash);

      if (t >= using_range.in()
          && t < using_range.out()) {
        // We want this hash, if we're not already rendering, start render now
        if (!render_task && !video_download_tasks_.key(hash)) {
          // Don't render any hash more than once
          RenderFrame(hash, t, false, false);
        }
      } else if (render_task) {
        // Cancel this frame unless it's already started
        QMutexLocker locker(render_task->GetTicket()->lock());

        if (!render_task->GetTicket()->IsRunning(false)) {
          video_tasks_.remove(render_task);
          delete render_task;
        }
      }
    }

    has_changed_ = false;
  }
}

void PreviewAutoCacher::ConformFinished()
{
  last_conform_task_ = QDateTime::currentMSecsSinceEpoch();

  if (viewer_node_) {
    foreach (const TimeRange &range, audio_needing_conform_) {
      viewer_node_->audio_playback_cache()->Invalidate(range, QDateTime::currentMSecsSinceEpoch());
    }
    audio_needing_conform_.clear();
  }
}

void PreviewAutoCacher::IgnoreNextMouseButton()
{
  ignore_next_mouse_button_ = true;
}

void PreviewAutoCacher::ForceCacheRange(const TimeRange &range)
{
  has_changed_ = true;
  use_custom_range_ = true;
  custom_autocache_range_ = range;

  RequeueFrames();
}

void PreviewAutoCacher::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ == viewer_node) {
    return;
  }

  if (viewer_node_) {
    // Cancel any remaining tickets and wait for them to finish

    // We need to wait for these since they send signals directly to the FrameHashCache
    // which might get deleted after this function.
    ClearHashQueue(true);

    // This can be cleared normally (frames will be discarded and need to be rendered again)
    ClearVideoQueue(true);

    // This can be cleared normally (PCM data will be discarded and need to be rendered again)
    ClearAudioQueue(true);

    // We'll need to wait for these since they work directly on the FrameHashCache. Frames will
    // be in the cache for later use.
    ClearVideoDownloadQueue(true);

    // Clear any single frame render that might be queued
    CancelQueuedSingleFrameRender();

    // No more immediate passthroughts
    video_immediate_passthroughs_.clear();

    // No more audio conforms
    audio_needing_conform_.clear();

    // Delete all of our copied nodes
    qDeleteAll(created_nodes_);
    created_nodes_.clear();
    copy_map_.clear();
    copied_viewer_node_ = nullptr;
    graph_update_queue_.clear();

    // Disconnect signals for future node additions/deletions
    NodeGraph* graph = viewer_node_->parent();

    disconnect(graph, &NodeGraph::NodeAdded, this, &PreviewAutoCacher::NodeAdded);
    disconnect(graph, &NodeGraph::NodeRemoved, this, &PreviewAutoCacher::NodeRemoved);
    disconnect(graph, &NodeGraph::InputConnected, this, &PreviewAutoCacher::EdgeAdded);
    disconnect(graph, &NodeGraph::InputDisconnected, this, &PreviewAutoCacher::EdgeRemoved);
    disconnect(graph, &NodeGraph::ValueChanged, this, &PreviewAutoCacher::ValueChanged);

    // Disconnect signal (will be a no-op if the signal was never connected)
    disconnect(viewer_node_->video_frame_cache(),
               &PlaybackCache::Invalidated,
               this,
               &PreviewAutoCacher::VideoInvalidated);

    disconnect(viewer_node_->audio_playback_cache(),
               &PlaybackCache::Invalidated,
               this,
               &PreviewAutoCacher::AudioInvalidated);
  }

  viewer_node_ = viewer_node;

  if (viewer_node_) {
    // Copy graph
    NodeGraph* graph = viewer_node_->parent();

    // Add all nodes
    for (int i=0; i<copied_project_.nodes().size(); i++) {
      InsertIntoCopyMap(graph->nodes().at(i), copied_project_.nodes().at(i));
    }
    for (int i=copied_project_.nodes().size(); i<graph->nodes().size(); i++) {
      AddNode(graph->nodes().at(i));
    }

    // Find copied viewer node
    copied_viewer_node_ = static_cast<ViewerOutput*>(copy_map_.value(viewer_node_));
    copied_color_manager_ = static_cast<ColorManager*>(copy_map_.value(viewer_node_->project()->color_manager()));

    // Add all connections
    foreach (Node* node, graph->nodes()) {
      for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
        AddEdge(it->second, it->first);
      }
    }

    UpdateLastSyncedValue();

    // Connect signals for future node additions/deletions
    connect(graph, &NodeGraph::NodeAdded, this, &PreviewAutoCacher::NodeAdded);
    connect(graph, &NodeGraph::NodeRemoved, this, &PreviewAutoCacher::NodeRemoved);
    connect(graph, &NodeGraph::InputConnected, this, &PreviewAutoCacher::EdgeAdded);
    connect(graph, &NodeGraph::InputDisconnected, this, &PreviewAutoCacher::EdgeRemoved);
    connect(graph, &NodeGraph::ValueChanged, this, &PreviewAutoCacher::ValueChanged);

    // Copy invalidated ranges - used to determine which frames need hashing
    invalidated_video_ = viewer_node_->video_frame_cache()->GetInvalidatedRanges();
    invalidated_audio_ = viewer_node_->audio_playback_cache()->GetInvalidatedRanges();

    connect(viewer_node_->video_frame_cache(),
            &PlaybackCache::Invalidated,
            this,
            &PreviewAutoCacher::VideoInvalidated);

    connect(viewer_node_->audio_playback_cache(),
            &PlaybackCache::Invalidated,
            this,
            &PreviewAutoCacher::AudioInvalidated);

    TryRender();
  }
}

RenderTicketWatcher* RetrieveFromQueueIterator(QMap<RenderTicketWatcher*, QByteArray>::iterator it)
{
  return it.key();
}

RenderTicketWatcher* RetrieveFromQueueIterator(QMap<RenderTicketWatcher*, TimeRange>::iterator it)
{
  return it.key();
}

RenderTicketWatcher* RetrieveFromQueueIterator(QVector<RenderTicketWatcher*>::iterator it)
{
  return *it;
}

void PreviewAutoCacher::ClearQueueRemoveEventInternal(QMap<RenderTicketWatcher*, QByteArray>::iterator it)
{
  Q_UNUSED(it)
}

void PreviewAutoCacher::ClearQueueRemoveEventInternal(QMap<RenderTicketWatcher*, TimeRange>::iterator it)
{
  Q_UNUSED(it)
}

void PreviewAutoCacher::ClearQueueRemoveEventInternal(QVector<RenderTicketWatcher*>::iterator it)
{
  Q_UNUSED(it)
}

template<typename T, typename Func>
void PreviewAutoCacher::ClearQueueInternal(T& list, bool hard, Func member)
{
  for (auto it=list.begin(); it!=list.end(); ) {
    RenderTicketWatcher* ticket = RetrieveFromQueueIterator(it);

    QMutexLocker locker(ticket->GetTicket()->lock());

    bool ticket_is_running = ticket->GetTicket()->IsRunning(false);

    if (hard || !ticket_is_running) {
      // Override default signalling
      disconnect(ticket, &RenderTicketWatcher::Finished, this, member);

      if (ticket_is_running) {
        // If ticket is running, assume we're hard clearing and wait for the ticket to finish
        ticket->GetTicket()->WaitForFinished(ticket->GetTicket()->lock());
      } else {
        // Just remove the ticket from the queue
        RenderManager::instance()->RemoveTicket(ticket->GetTicket());
      }

      // Special functionality for certain queues
      ClearQueueRemoveEventInternal(it);

      // Destroy ticket
      locker.unlock();
      delete ticket;

      it = list.erase(it);
    } else {
      // We can't clear this, probably because we're soft clearing and the ticket is currently running
      it++;
    }
  }
}

}
