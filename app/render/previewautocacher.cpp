#include "previewautocacher.h"

#include <QApplication>
#include <QtConcurrent/QtConcurrent>

#include "render/rendermanager.h"

OLIVE_NAMESPACE_ENTER

PreviewAutoCacher::PreviewAutoCacher() :
  viewer_node_(nullptr),
  paused_(false),
  has_changed_(false),
  use_custom_range_(false),
  last_update_time_(0),
  ignore_next_mouse_button_(false),
  video_params_changed_(false),
  audio_params_changed_(false)
{
  // Set default autocache range
  SetPlayhead(rational());
}

RenderTicketPtr PreviewAutoCacher::GetSingleFrame(const rational &t)
{
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("time", QVariant::fromValue(t));

  TryRender();

  return ticket;
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

void PreviewAutoCacher::NodeGraphChanged(NodeInput *source)
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

    // Check if this input supersedes an already queued input
    if ((source->IsArray() && static_cast<NodeInputArray*>(source)->sub_params().contains(queued_input))
        || queued_input->parentNode()->OutputsTo(source, true, true)) {
      // In which case, we don't need to queue it and can queue our own
      graph_update_queue_.removeAt(i);
      disconnect(queued_input, &NodeInput::destroyed, this, &PreviewAutoCacher::QueuedInputRemoved);
      i--;
    }

    // Check if the source is a member of this array, in which case it'll be copied eventually anyway
    if (queued_input->IsArray()
        && static_cast<NodeInputArray*>(queued_input)->sub_params().contains(source)) {
      return;
    }

    // Check if this dependency graph is already queued
    if (source->parentNode()->OutputsTo(queued_input, true, true)) {
      // In which case, no further copy is necessary
      return;
    }
  }

  graph_update_queue_.append(source);
  connect(source, &NodeInput::destroyed, this, &PreviewAutoCacher::QueuedInputRemoved);
}

void PreviewAutoCacher::GenerateHashes(ViewerOutput *viewer, const QVector<rational> &times, qint64 job_time)
{
  std::vector<QByteArray> existing_hashes;

  foreach (const rational& time, times) {
    // See if hash already exists in disk cache
    QByteArray hash = RenderManager::Hash(viewer, viewer->video_params(), time);

    // Check memory list since disk checking is slow
    bool hash_exists = (std::find(existing_hashes.begin(), existing_hashes.end(), hash) != existing_hashes.end());

    if (!hash_exists) {
      hash_exists = QFileInfo::exists(viewer->video_frame_cache()->CachePathName(hash));

      if (hash_exists) {
        existing_hashes.push_back(hash);
      }
    }

    // Set hash in FrameHashCache's thread rather than in ours to prevent race conditions
    QMetaObject::invokeMethod(viewer->video_frame_cache(), "SetHash", Qt::QueuedConnection,
                              OLIVE_NS_ARG(rational, time),
                              Q_ARG(QByteArray, hash),
                              Q_ARG(qint64, job_time),
                              Q_ARG(bool, hash_exists));
  }
}

void PreviewAutoCacher::VideoInvalidated(const TimeRange &range)
{
  qDebug() << "Video invalidated";

  ClearQueue(false);

  // Hash these frames since that should be relatively quick.
  if (ignore_next_mouse_button_ || !(qApp->mouseButtons() & Qt::LeftButton)) {
    ignore_next_mouse_button_ = false;

    invalidated_video_.insert(range);

    TryRender();
  }
}

void PreviewAutoCacher::AudioInvalidated(const TimeRange &range)
{
  ClearQueue(false);

  // Start jobs to re-render the audio at this range, split into 2 second chunks
  invalidated_audio_.insert(range);

  TryRender();
}

void PreviewAutoCacher::HashesProcessed()
{
  QFutureWatcher<void>* watcher = static_cast<QFutureWatcher<void>*>(sender());

  if (hash_tasks_.contains(watcher)) {
    hash_tasks_.removeOne(watcher);

    RequeueFrames();
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
    }

    audio_tasks_.remove(watcher);
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
      QFutureWatcher<bool>* w = new QFutureWatcher<bool>();
      video_download_tasks_.insert(w, hash);
      connect(w, &QFutureWatcher<bool>::finished, this, &PreviewAutoCacher::VideoDownloaded);
      w->setFuture(QtConcurrent::run(viewer_node_->video_frame_cache(),
                                     &FrameHashCache::SaveCacheFrame,
                                     hash,
                                     watcher->Get().value<FramePtr>()));
    }

    video_tasks_.remove(watcher);
  }

  delete watcher;
}

void PreviewAutoCacher::VideoDownloaded()
{
  QFutureWatcher<bool>* watcher = static_cast<QFutureWatcher<bool>*>(sender());

  if (video_download_tasks_.contains(watcher)) {
    if (!watcher->isCanceled()) {
      if (watcher->result()) {
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

void PreviewAutoCacher::QueuedInputRemoved()
{
  NodeInput* i = static_cast<NodeInput*>(sender());
  disconnect(i, &NodeInput::destroyed, this, &PreviewAutoCacher::QueuedInputRemoved);
  graph_update_queue_.removeOne(i);
}

void PreviewAutoCacher::VideoParamsChanged()
{
  // In case the user is pressing the mouse at this exact moment
  IgnoreNextMouseButton();

  ClearVideoQueue();
  video_params_changed_ = true;
  TryRender();
}

void PreviewAutoCacher::AudioParamsChanged()
{
  ClearAudioQueue();
  audio_params_changed_ = true;
  TryRender();
}

//#define PRINT_UPDATE_QUEUE_INFO
void PreviewAutoCacher::ProcessUpdateQueue()
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
    disconnect(i, &NodeInput::destroyed, this, &PreviewAutoCacher::QueuedInputRemoved);

    CopyNodeInputValue(i);
  }

#ifdef PRINT_UPDATE_QUEUE_INFO
  qDebug() << "Update queue took:" << (QDateTime::currentMSecsSinceEpoch() - t);
#endif
}

bool PreviewAutoCacher::HasActiveJobs() const
{
  return !hash_tasks_.isEmpty()
      || !audio_tasks_.isEmpty()
      || !video_tasks_.isEmpty()
      || !video_download_tasks_.isEmpty();
}

void PreviewAutoCacher::SetPlayhead(const rational &playhead)
{
  cache_range_ = TimeRange(playhead - Config::Current()["DiskCacheBehind"].value<rational>(),
      playhead + Config::Current()["DiskCacheAhead"].value<rational>());

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
    for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
      it.key()->WaitForFinished();
    }
  }
}

void PreviewAutoCacher::CopyNodeInputValue(NodeInput *input)
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

Node* PreviewAutoCacher::CopyNodeConnections(Node* src_node)
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

void PreviewAutoCacher::CopyNodeMakeConnection(NodeInput* src_input, NodeInput* dst_input)
{
  if (src_input->is_connected()) {
    Node* dst_node = CopyNodeConnections(src_input->get_connected_node());

    NodeOutput* corresponding_output = dst_node->GetOutputWithID(src_input->get_connected_output()->id());

    NodeParam::ConnectEdge(corresponding_output,
                           dst_input);
  }
}

void PreviewAutoCacher::TryRender()
{
  if (!graph_update_queue_.isEmpty()) {
    if (HasActiveJobs()) {
      // Still waiting for jobs to finish
      return;
    }

    // No jobs are active, we can process the update queue
    last_update_time_ = QDateTime::currentMSecsSinceEpoch();

    ProcessUpdateQueue();

    if (video_params_changed_) {
      copied_viewer_node_->set_video_params(viewer_node_->video_params());
      video_params_changed_ = false;
    }

    if (audio_params_changed_) {
      copied_viewer_node_->set_audio_params(viewer_node_->audio_params());
      audio_params_changed_ = false;
    }
  }

  // If we're here, we must be able to render
  if (!invalidated_video_.isEmpty()) {
    QVector<rational> frames = viewer_node_->video_frame_cache()->GetFrameListFromTimeRange(invalidated_video_);

    QFutureWatcher<void>* watcher = new QFutureWatcher<void>();
    hash_tasks_.append(watcher);
    connect(watcher, &QFutureWatcher<void>::finished, this, &PreviewAutoCacher::HashesProcessed);
    watcher->setFuture(QtConcurrent::run(&PreviewAutoCacher::GenerateHashes,
                                         copied_viewer_node_,
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

  if (!single_frame_renders_.isEmpty()) {
    foreach (RenderTicketPtr ticket, single_frame_renders_) {
      RenderTicketWatcher* watcher = new RenderTicketWatcher();

      watcher->setProperty("passthrough", QVariant::fromValue(ticket));

      connect(watcher, &RenderTicketWatcher::Finished, watcher, [watcher]{
        RenderTicketPtr passthrough = watcher->property("passthrough").value<RenderTicketPtr>();
        passthrough->Finish(watcher->GetTicket()->Get(), watcher->GetTicket()->WasCancelled());
        watcher->deleteLater();
      });

      watcher->SetTicket(RenderManager::instance()->RenderFrame(copied_viewer_node_,
                                                                ticket->property("time").value<rational>(),
                                                                RenderMode::kOffline, true));
    }
    single_frame_renders_.clear();
  }
}

void PreviewAutoCacher::RequeueFrames()
{
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
        watcher->SetTicket(RenderManager::instance()->RenderFrame(copied_viewer_node_, t, RenderMode::kOffline, false));
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

void PreviewAutoCacher::SetViewerNode(ViewerOutput *viewer_node)
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
      {
        QMap<QFutureWatcher<bool>*, QByteArray>::const_iterator i;
        for (i=video_download_tasks_.constBegin(); i!=video_download_tasks_.constEnd(); i++) {
          i.key()->waitForFinished();
        }
        video_download_tasks_.clear();
      }

      // No longer caching any hashes
      currently_caching_hashes_.clear();
    }

    // Delete all of our copied nodes
    foreach (Node* c, copy_map_) {
      delete c;
    }
    copy_map_.clear();
    copied_viewer_node_ = nullptr;
    graph_update_queue_.clear();

    video_params_changed_ = false;
    audio_params_changed_ = false;

    // Disconnect signal (will be a no-op if the signal was never connected)
    disconnect(viewer_node_,
               &ViewerOutput::GraphChangedFrom,
               this,
               &PreviewAutoCacher::NodeGraphChanged);

    disconnect(viewer_node_,
               &ViewerOutput::VideoParamsChanged,
               this,
               &PreviewAutoCacher::VideoParamsChanged);

    disconnect(viewer_node_,
               &ViewerOutput::AudioParamsChanged,
               this,
               &PreviewAutoCacher::AudioParamsChanged);

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
    copied_viewer_node_ = static_cast<ViewerOutput*>(viewer_node_->copy());
    copy_map_.insert(viewer_node_, copied_viewer_node_);

    // Copy parameters
    copied_viewer_node_->set_video_params(viewer_node_->video_params());
    copied_viewer_node_->set_audio_params(viewer_node_->audio_params());

    // We begin an operation and never end it which prevents the copy from unnecessarily
    // invalidating its own cache
    copied_viewer_node_->BeginOperation();

    NodeGraphChanged(viewer_node_->texture_input());
    NodeGraphChanged(viewer_node_->samples_input());
    ProcessUpdateQueue();

    invalidated_video_ = viewer_node_->video_frame_cache()->GetInvalidatedRanges();
    invalidated_audio_ = viewer_node_->audio_playback_cache()->GetInvalidatedRanges();

    connect(viewer_node_,
            &ViewerOutput::GraphChangedFrom,
            this,
            &PreviewAutoCacher::NodeGraphChanged);

    connect(viewer_node_,
            &ViewerOutput::VideoParamsChanged,
            this,
            &PreviewAutoCacher::VideoParamsChanged);

    connect(viewer_node_,
            &ViewerOutput::AudioParamsChanged,
            this,
            &PreviewAutoCacher::AudioParamsChanged);

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

OLIVE_NAMESPACE_EXIT
