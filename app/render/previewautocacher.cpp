#include "previewautocacher.h"

#include <QApplication>
#include <QtConcurrent/QtConcurrent>

#include "project/item/sequence/sequence.h"
#include "project/project.h"
#include "render/rendermanager.h"
#include "render/renderprocessor.h"

namespace olive {

PreviewAutoCacher::PreviewAutoCacher() :
  viewer_node_(nullptr),
  paused_(false),
  has_changed_(false),
  use_custom_range_(false),
  single_frame_render_(nullptr),
  last_update_time_(0),
  ignore_next_mouse_button_(false),
  color_manager_(nullptr)
{
  // Set default autocache range
  SetPlayhead(rational());

  delayed_requeue_timer_.setInterval(Config::Current()[QStringLiteral("AutoCacheDelay")].toInt());
  delayed_requeue_timer_.setSingleShot(true);
  connect(&delayed_requeue_timer_, &QTimer::timeout, this, &PreviewAutoCacher::RequeueFrames);
}

PreviewAutoCacher::~PreviewAutoCacher()
{
  // Ensure everything is cleaned up appropriately
  SetViewerNode(nullptr);
}

RenderTicketPtr PreviewAutoCacher::GetSingleFrame(const rational &t)
{
  if (single_frame_render_) {
    single_frame_render_->Cancel();
  }

  single_frame_render_ = std::make_shared<RenderTicket>();

  single_frame_render_->setProperty("time", QVariant::fromValue(t));

  // Copy because TryRender() might set this to null and we still want to return a handle to this
  RenderTicketPtr copy = single_frame_render_;

  TryRender();

  return copy;
}

void PreviewAutoCacher::SetPaused(bool paused)
{
  paused_ = paused;

  if (paused_) {
    // Pause the autocache
    ClearVideoQueue();
  } else {
    // Unpause the cache
    RequeueFrames();
  }
}

void PreviewAutoCacher::GenerateHashes(Sequence *viewer, FrameHashCache* cache, const QVector<rational> &times, qint64 job_time)
{
  std::vector<QByteArray> existing_hashes;

  foreach (const rational& time, times) {
    // See if hash already exists in disk cache
    QByteArray hash = RenderManager::Hash(viewer->GetConnectedNode(Sequence::kTextureInput), viewer->video_params(), time);

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
  ClearAudioQueue();

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
    if (!watcher->WasCancelled()) {
      viewer_node_->audio_playback_cache()->WritePCM(audio_tasks_.value(watcher),
                                                     watcher->Get().value<SampleBufferPtr>(),
                                                     watcher->GetTicket()->GetJobTime());

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
            track->waveform().set_channel_count(viewer_node_->audio_params().channel_count());

            foreach (const TimeRange& r, valid_ranges) {
              track->waveform().OverwriteSums(waveform_info.waveform, r.in(), r.in() - waveform_info.range.in(), r.length());
            }

            emit track->PreviewChanged();
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
    if (watcher->WasCancelled()) {
      // We didn't get this hash
      currently_caching_hashes_.removeOne(watcher->property("hash").toByteArray());
    } else {
      const QByteArray& hash = video_tasks_.value(watcher);

      // Download frame in another thread
      RenderTicketWatcher* w = new RenderTicketWatcher();
      video_download_tasks_.insert(w, hash);
      connect(w, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::VideoDownloaded);
      w->SetTicket(RenderManager::instance()->SaveFrameToCache(viewer_node_->video_frame_cache(),
                                                               watcher->Get().value<FramePtr>(),
                                                               hash,
                                                               true));
    }

    video_tasks_.remove(watcher);
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
    if (!watcher->WasCancelled()) {
      if (watcher->Get().toBool()) {
        const QByteArray& hash = video_download_tasks_.value(watcher);

        currently_caching_hashes_.removeOne(hash);

        viewer_node_->video_frame_cache()->ValidateFramesWithHash(hash);
      } else {
        qCritical() << "Failed to download video frame";
      }
    }

    video_download_tasks_.remove(watcher);
  }

  delete watcher;
}

void PreviewAutoCacher::SingleFrameFinished()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());
  RenderTicketPtr passthrough = watcher->property("passthrough").value<RenderTicketPtr>();
  passthrough->Finish(watcher->GetTicket()->Get(), watcher->GetTicket()->WasCancelled());
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

void PreviewAutoCacher::SetPlayhead(const rational &playhead)
{
  cache_range_ = TimeRange(playhead - Config::Current()[QStringLiteral("DiskCacheBehind")].value<rational>(),
      playhead + Config::Current()[QStringLiteral("DiskCacheAhead")].value<rational>());

  has_changed_ = true;
  use_custom_range_ = false;

  RequeueFrames();
}

void PreviewAutoCacher::ClearQueue(bool wait)
{
  ClearHashQueue(wait);
  ClearVideoQueue(wait);
  ClearAudioQueue(wait);
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
  }
}

void PreviewAutoCacher::ClearVideoQueue(bool wait)
{
  // Copy because tasks that cancel immediately will be automatically removed from the list
  auto copy = video_tasks_;

  for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
    it.key()->Cancel();
  }
  if (wait) {
    copy = video_tasks_;
    for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
      it.key()->WaitForFinished();
    }
  }

  has_changed_ = true;
  use_custom_range_ = false;
}

void PreviewAutoCacher::ClearAudioQueue(bool wait)
{
  // Create a copy because otherwise
  auto copy = audio_tasks_;

  for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
    it.key()->Cancel();
  }
  if (wait) {
    copy = audio_tasks_;
    for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
      it.key()->WaitForFinished();
    }
  }
}

void PreviewAutoCacher::ClearVideoDownloadQueue(bool wait)
{
  // Create a copy because otherwise
  auto copy = video_download_tasks_;

  for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
    it.key()->Cancel();
  }
  if (wait) {
    copy = video_download_tasks_;
    for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
      it.key()->WaitForFinished();
    }
  }
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
      std::list<TimeRange> chunks = range.Split(2);

      foreach (const TimeRange& r, chunks) {
        RenderTicketWatcher* watcher = new RenderTicketWatcher();
        connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::AudioRendered);
        audio_tasks_.insert(watcher, r);
        watcher->SetTicket(RenderManager::instance()->RenderAudio(copied_viewer_node_, r, true));
      }
    }

    invalidated_audio_.clear();
  }

  if (single_frame_render_) {
    RenderTicketWatcher* watcher = new RenderTicketWatcher();

    watcher->setProperty("passthrough", QVariant::fromValue(single_frame_render_));

    connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::SingleFrameFinished);

    single_frame_render_->Start();

    watcher->SetTicket(RenderManager::instance()->RenderFrame(copied_viewer_node_,
                                                              color_manager_,
                                                              single_frame_render_->property("time").value<rational>(),
                                                              RenderMode::kOffline,
                                                              viewer_node_->video_frame_cache(),
                                                              true));

    single_frame_render_ = nullptr;
  }
}

void PreviewAutoCacher::RequeueFrames()
{
  delayed_requeue_timer_.stop();

  if (viewer_node_
      && viewer_node_->video_frame_cache()->HasInvalidatedRanges()
      && hash_tasks_.isEmpty()
      && has_changed_
      && (!paused_ || use_custom_range_)) {
    TimeRange using_range;

    if (use_custom_range_) {
      using_range = custom_autocache_range_;
      use_custom_range_ = false;
    } else {
      using_range = cache_range_;
    }

    QVector<rational> invalidated_ranges = viewer_node_->video_frame_cache()->GetInvalidatedFrames(using_range);

    ClearVideoQueue();

    foreach (const rational& t, invalidated_ranges) {
      const QByteArray& hash = viewer_node_->video_frame_cache()->GetHash(t);

      if (t >= using_range.in()
          && t < using_range.out()
          && !currently_caching_hashes_.contains(hash)) {
        // Don't render any hash more than once
        currently_caching_hashes_.append(hash);

        RenderTicketWatcher* watcher = new RenderTicketWatcher();
        watcher->setProperty("hash", hash);
        connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::VideoRendered);
        video_tasks_.insert(watcher, hash);
        watcher->SetTicket(RenderManager::instance()->RenderFrame(copied_viewer_node_,
                                                                  color_manager_,
                                                                  t,
                                                                  RenderMode::kOffline,
                                                                  viewer_node_->video_frame_cache(),
                                                                  false));
      }
    }

    has_changed_ = false;
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

void PreviewAutoCacher::SetViewerNode(Sequence *viewer_node)
{
  if (viewer_node_ == viewer_node) {
    return;
  }

  if (viewer_node_) {
    // Cancel any remaining tickets and wait for them to finish
    ClearQueue(true);

    // Clear autocache lists
    {
      // We need to wait for these since they work directly on the FrameHashCache. Most of the time
      // this is fine, but not if the FrameHashCache gets deleted after this function.
      ClearHashQueue(true);

      // This can be cleared normally (frames will be discarded and need to be rendered again)
      ClearVideoQueue(false);

      // This can be cleared normally (PCM data will be discarded and need to be rendered again)
      ClearAudioQueue(false);

      // We'll need to wait for these since they work directly on the FrameHashCache. Frames will
      // be in the cache for later use.
      ClearVideoDownloadQueue(true);

      // No longer caching any hashes
      currently_caching_hashes_.clear();
    }

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
    copied_viewer_node_ = static_cast<Sequence*>(copy_map_.value(viewer_node_));

    // Copy parameters
    copied_viewer_node_->set_video_params(viewer_node_->video_params());
    copied_viewer_node_->set_audio_params(viewer_node_->audio_params());

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

}
