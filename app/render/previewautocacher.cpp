/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "previewautocacher.h"

#include <QApplication>
#include <QtConcurrent/QtConcurrent>

#include "codec/conformmanager.h"
#include "node/inputdragger.h"
#include "node/project/project.h"
#include "render/renderprocessor.h"
#include "task/customcache/customcachetask.h"
#include "task/taskmanager.h"
#include "widget/slider/base/numericsliderbase.h"

namespace olive {

// We may want to make this configurable at some point, so for now this constant is used as a
// placeholder for where that configarable variable would be used.
const bool PreviewAutoCacher::kRealTimeWaveformsEnabled = true;

PreviewAutoCacher::PreviewAutoCacher() :
  viewer_node_(nullptr),
  use_custom_range_(false),
  pause_audio_(false),
  single_frame_render_(nullptr)
{
  // Set defaults
  SetPlayhead(0);

  // Wait a certain amount of time before requeuing when we receive an invalidate signal
  delayed_requeue_timer_.setInterval(OLIVE_CONFIG("AutoCacheDelay").toInt());
  delayed_requeue_timer_.setSingleShot(true);
  connect(&delayed_requeue_timer_, &QTimer::timeout, this, &PreviewAutoCacher::RequeueFrames);

  // Catch when a conform is ready
  connect(ConformManager::instance(), &ConformManager::ConformReady, this, &PreviewAutoCacher::ConformFinished);
}

PreviewAutoCacher::~PreviewAutoCacher()
{
  // Ensure everything is cleaned up appropriately
  SetViewerNode(nullptr);
}

RenderTicketPtr PreviewAutoCacher::GetSingleFrame(const rational &t, bool dry)
{
  // If we have a single frame render queued (but not yet sent to the RenderManager), cancel it now
  CancelQueuedSingleFrameRender();

  // Create a new single frame render ticket
  auto sfr = std::make_shared<RenderTicket>();
  sfr->Start();
  sfr->setProperty("time", QVariant::fromValue(t));
  sfr->setProperty("dry", dry);

  // Queue it and try to render
  single_frame_render_ = sfr;
  TryRender();

  return sfr;
}

RenderTicketPtr PreviewAutoCacher::GetRangeOfAudio(TimeRange range)
{
  return RenderAudio(range, false);
}

void PreviewAutoCacher::VideoInvalidated(const TimeRange &range)
{
  // Stop any current render tasks because a) they might be out of date now anyway, and b) we
  // want to dedicate all our rendering power to realtime feedback for the user
  CancelVideoTasks();

  // If auto-cache is enabled and a slider is not being dragged, queue up to hash these frames
  if (viewer_node_->video_frame_cache()->IsEnabled() && !NodeInputDragger::IsInputBeingDragged()) {
    StartCachingVideoRange(range);
  }
}

void PreviewAutoCacher::AudioInvalidated(const TimeRange &range)
{
  // We don't stop rendering audio because currently there's no system of requeuing audio if it's
  // cancelled, so some areas may end up unrendered forever
  //  ClearAudioQueue();

  // If we're auto-caching audio or require realtime waveforms, we'll have to render this
  if (viewer_node_->audio_playback_cache()->IsEnabled() || kRealTimeWaveformsEnabled) {
    StartCachingAudioRange(range);
  }
}

void PreviewAutoCacher::AudioRendered()
{
  // Receive watcher
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  // If the task list doesn't contain this watcher, presumably it was cleared as a result of a
  // viewer switch, so we'll completely ignore this watcher
  if (audio_tasks_.contains(watcher)) {
    // Assume that a "result" is a fully completed image and a non-result is a cancelled ticket
    TimeRange range = audio_tasks_.take(watcher);

    if (watcher->HasResult()) {
      // Remove this task from the list
      JobTime watcher_job_time = watcher->property("job").value<JobTime>();

      TimeRangeList valid_ranges = audio_job_tracker_.getCurrentSubRanges(range, watcher_job_time);

      AudioVisualWaveform waveform = watcher->GetTicket()->property("waveform").value<AudioVisualWaveform>();

      if (viewer_node_->audio_playback_cache()->IsEnabled()) {
        // WritePCM is tolerant to its buffer being null, it will just write silence instead
        viewer_node_->audio_playback_cache()->WritePCM(range,
                                                       valid_ranges,
                                                       watcher->Get().value<SampleBuffer>());
      }

      viewer_node_->audio_playback_cache()->WriteWaveform(range, valid_ranges, &waveform);

      // Detect if this audio was incomplete because it was waiting on a conform to finish
      if (watcher->GetTicket()->property("incomplete").toBool()) {
        if (last_conform_task_ > watcher_job_time) {
          // Requeue now
          viewer_node_->audio_playback_cache()->Invalidate(range);
        } else {
          // Wait for conform
          audio_needing_conform_.insert(range);
        }
      } else{
        // Retrieve visual waveforms
        QVector<RenderProcessor::RenderedWaveform> waveform_list = watcher->GetTicket()->property("waveforms").value< QVector<RenderProcessor::RenderedWaveform> >();
        foreach (const RenderProcessor::RenderedWaveform& waveform_info, waveform_list) {
          // Find original track
          ClipBlock* block = nullptr;

          for (auto it=copy_map_.cbegin(); it!=copy_map_.cend(); it++) {
            if (it.value() == waveform_info.block) {
              block = static_cast<ClipBlock*>(it.key());
              break;
            }
          }

          if (block && !valid_ranges.isEmpty()) {
            // Generate visual waveform in this background thread
            block->waveform().set_channel_count(viewer_node_->GetAudioParams().channel_count());

            // Determine which of the waveform ranges we got intersects with the valid ranges
            TimeRangeList intersections = valid_ranges.Intersects(waveform_info.range + block->in());
            foreach (TimeRange r, intersections) {
              // For each range, adjust it relative to the block and write it
              r -= block->in();

              if (waveform_info.silence) {
                block->waveform().OverwriteSilence(r.in(), r.length());
              } else {
                block->waveform().OverwriteSums(waveform_info.waveform, r.in(), r.in() - waveform_info.range.in(), r.length());
              }
            }

            emit block->PreviewChanged();
          }
        }
      }
    }

    // Continue rendering
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::VideoRendered()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  // Process passthroughs no matter what, if the viewer was switched, the passthrough map would be
  // cleared anyway
  QVector<RenderTicketPtr> tickets = video_immediate_passthroughs_.take(watcher);
  foreach (RenderTicketPtr t, tickets) {
    if (watcher->HasResult()) {
      t->Finish(watcher->Get());
    } else {
      t->Finish();
    }
  }

  // If the task list doesn't contain this watcher, presumably it was cleared as a result of a
  // viewer switch, so we'll completely ignore this watcher
  auto it = video_tasks_.find(watcher);

  if (it != video_tasks_.end()) {
    // Assume that a "result" is a fully completed image and a non-result is a cancelled ticket
    if (watcher->HasResult()) {
      // Download frame in another thread
      if (watcher->GetTicket()->property("cached").toBool()) {
        if (FrameHashCache *cache = Node::ValueToPtr<FrameHashCache>(watcher->property("cache"))) {
          cache->ValidateTime(it.value());
        }
      }
    }

    video_tasks_.erase(it);

    // Continue rendering
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::ProcessUpdateQueue()
{
  // Iterate everything that happened to the graph and do the same thing on our end
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
    case QueuedJob::kValueHintChanged:
      CopyValueHint(job.input);
      break;
    }
  }
  graph_update_queue_.clear();

  // Indicate that we have synchronized to this point, which is compared with the graph change
  // time to see if our copied graph is up to date
  UpdateLastSyncedValue();
}

void PreviewAutoCacher::AddNode(Node *node)
{
  if (dynamic_cast<NodeGroup*>(node)) {
    // Group nodes are just dummy nodes, no need to copy them
    return;
  }

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

  // Disconnect from node's caches
  DisconnectFromNodeCache(node);

  // Remove from created list
  created_nodes_.removeOne(copy);

  // Delete it
  delete copy;
}

void PreviewAutoCacher::AddEdge(Node *output, const NodeInput &input)
{
  // Create same connection with our copied graph
  Node* our_output = copy_map_.value(output);
  Node* our_input = copy_map_.value(input.node());

  Node::ConnectEdge(our_output, NodeInput(our_input, input.input(), input.element()));
}

void PreviewAutoCacher::RemoveEdge(Node *output, const NodeInput &input)
{
  // Remove same connection with our copied graph
  Node* our_output = copy_map_.value(output);
  Node* our_input = copy_map_.value(input.node());

  Node::DisconnectEdge(our_output, NodeInput(our_input, input.input(), input.element()));
}

void PreviewAutoCacher::CopyValue(const NodeInput &input)
{
  if (dynamic_cast<NodeGroup*>(input.node())) {
    // Group nodes are just dummy nodes, no need to copy them
    return;
  }

  // Copy all values to our graph
  Node* our_input = copy_map_.value(input.node());
  Node::CopyValuesOfElement(input.node(), our_input, input.input(), input.element());
}

void PreviewAutoCacher::CopyValueHint(const NodeInput &input)
{
  if (dynamic_cast<NodeGroup*>(input.node())) {
    // Group nodes are just dummy nodes, no need to copy them
    return;
  }

  // Copy value hint to our graph
  Node* our_input = copy_map_.value(input.node());
  Node::ValueHint hint = input.node()->GetValueHintForInput(input.input(), input.element());
  our_input->SetValueHintForInput(input.input(), hint, input.element());
}

void PreviewAutoCacher::InsertIntoCopyMap(Node *node, Node *copy)
{
  // Insert into map
  copy_map_.insert(node, copy);

  // Copy parameters
  Node::CopyInputs(node, copy, false);

  // Connect to node's cache
  ConnectToNodeCache(node);
}

void PreviewAutoCacher::ConnectToNodeCache(Node *node)
{
  // TEMP: Retain existing behavior until more work is done
  if (node == viewer_node_) {
    connect(node->video_frame_cache(),
            &PlaybackCache::EnabledChanged,
            this,
            &PreviewAutoCacher::VideoAutoCacheEnableChanged);

    connect(node->audio_playback_cache(),
            &PlaybackCache::EnabledChanged,
            this,
            &PreviewAutoCacher::AudioAutoCacheEnableChanged);

    connect(node->video_frame_cache(),
            &PlaybackCache::Invalidated,
            this,
            &PreviewAutoCacher::VideoInvalidated);

    connect(node->audio_playback_cache(),
            &PlaybackCache::Invalidated,
            this,
            &PreviewAutoCacher::AudioInvalidated);
  }
}

void PreviewAutoCacher::DisconnectFromNodeCache(Node *node)
{
  // TEMP: Retain existing behavior until more work is done
  if (node == viewer_node_) {
    disconnect(node->video_frame_cache(),
               &PlaybackCache::EnabledChanged,
               this,
               &PreviewAutoCacher::VideoAutoCacheEnableChanged);

    disconnect(node->audio_playback_cache(),
               &PlaybackCache::EnabledChanged,
               this,
               &PreviewAutoCacher::AudioAutoCacheEnableChanged);

    disconnect(node->video_frame_cache(),
               &PlaybackCache::Invalidated,
               this,
               &PreviewAutoCacher::VideoInvalidated);

    disconnect(node->audio_playback_cache(),
               &PlaybackCache::Invalidated,
               this,
               &PreviewAutoCacher::AudioInvalidated);
  }
}

void PreviewAutoCacher::UpdateGraphChangeValue()
{
  graph_changed_time_.Acquire();
}

void PreviewAutoCacher::UpdateLastSyncedValue()
{
  last_update_time_.Acquire();
}

void PreviewAutoCacher::CancelQueuedSingleFrameRender()
{
  if (single_frame_render_) {
    // Signal that this ticket was cancelled with no value
    single_frame_render_->Finish();
    single_frame_render_ = nullptr;
  }
}

void PreviewAutoCacher::VideoInvalidatedList(const TimeRangeList &list)
{
  foreach (const TimeRange &range, list) {
    VideoInvalidated(range);
  }
}

void PreviewAutoCacher::AudioInvalidatedList(const TimeRangeList &list)
{
  foreach (const TimeRange &range, list) {
    AudioInvalidated(range);
  }
}

void PreviewAutoCacher::StartCachingRange(const TimeRange &range, TimeRangeList *range_list, RenderJobTracker *tracker)
{
  range_list->insert(range);
  tracker->insert(range, graph_changed_time_);
}

void PreviewAutoCacher::StartCachingVideoRange(const TimeRange &range)
{
  StartCachingRange(range, &invalidated_video_, &video_job_tracker_);
  RequeueFrames();
}

void PreviewAutoCacher::StartCachingAudioRange(const TimeRange &range)
{
  StartCachingRange(range, &invalidated_audio_, &audio_job_tracker_);
  TryRender();
}

void PreviewAutoCacher::SetPlayhead(const rational &playhead)
{
  cache_range_ = TimeRange(playhead - OLIVE_CONFIG("DiskCacheBehind").value<rational>(),
                           playhead + OLIVE_CONFIG("DiskCacheAhead").value<rational>());

  RequeueFrames();
}

template<typename T>
void CancelTasks(const T &task_list, bool and_wait)
{
  for (auto it=task_list.cbegin(); it!=task_list.cend(); it++) {
    // Signal that the ticket should not be finished
    it.key()->Cancel();
  }

  if (and_wait) {
    // Wait for each ticket to finish
    for (auto it=task_list.cbegin(); it!=task_list.cend(); it++) {
      it.key()->WaitForFinished();
    }
  }
}

void PreviewAutoCacher::CancelVideoTasks(bool and_wait_for_them_to_finish)
{
  CancelTasks(video_tasks_, and_wait_for_them_to_finish);
}

void PreviewAutoCacher::CancelAudioTasks(bool and_wait_for_them_to_finish)
{
  CancelTasks(audio_tasks_, and_wait_for_them_to_finish);
}

void PreviewAutoCacher::SetAudioPaused(bool e)
{
  pause_audio_ = e;
  if (!e) {
    TryRender();
  }
}

void PreviewAutoCacher::NodeAdded(Node *node)
{
  graph_update_queue_.append({QueuedJob::kNodeAdded, node, NodeInput(), nullptr});
  UpdateGraphChangeValue();
}

void PreviewAutoCacher::NodeRemoved(Node *node)
{
  graph_update_queue_.append({QueuedJob::kNodeRemoved, node, NodeInput(), nullptr});
  UpdateGraphChangeValue();
}

void PreviewAutoCacher::EdgeAdded(Node *output, const NodeInput &input)
{
  graph_update_queue_.append({QueuedJob::kEdgeAdded, nullptr, input, output});
  UpdateGraphChangeValue();
}

void PreviewAutoCacher::EdgeRemoved(Node *output, const NodeInput &input)
{
  graph_update_queue_.append({QueuedJob::kEdgeRemoved, nullptr, input, output});
  UpdateGraphChangeValue();
}

void PreviewAutoCacher::ValueChanged(const NodeInput &input)
{
  graph_update_queue_.append({QueuedJob::kValueChanged, nullptr, input, nullptr});
  UpdateGraphChangeValue();
}

void PreviewAutoCacher::ValueHintChanged(const NodeInput &input)
{
  graph_update_queue_.append({QueuedJob::kValueHintChanged, nullptr, input, nullptr});
  UpdateGraphChangeValue();
}

void PreviewAutoCacher::TryRender()
{
  if (!graph_update_queue_.isEmpty()) {
    // Check if we have jobs running in other threads that shouldn't be interrupted right now
    // NOTE: We don't check for downloads because, while they run in another thread, they don't
    //       require any access to the graph and therefore don't risk race conditions.
    if (!audio_tasks_.isEmpty()
        || !video_tasks_.isEmpty()) {
      return;
    }

    // No jobs are active, we can process the update queue
    ProcessUpdateQueue();
  }

  // Check for newly invalidated video and hash it
  if (!invalidated_video_.isEmpty()) {
    if (!copied_viewer_node_->GetConnectedTextureOutput()) {
      queued_frame_iterator_.reset();
    } else if (queued_frame_iterator_.HasNext()) {
      queued_frame_iterator_.insert(invalidated_video_);
    } else {
      queued_frame_iterator_ = TimeRangeListFrameIterator(invalidated_video_, viewer_node_->GetVideoParams().frame_rate_as_time_base());
    }
    invalidated_video_.clear();
  }

  if (!invalidated_audio_.isEmpty()) {
    // Add newly invalidated audio to iterator
    audio_iterator_.insert(invalidated_audio_);
    invalidated_audio_.clear();
  }

  if (single_frame_render_) {
    // Make an explicit copy of the render ticket here - it seems that on some systems it can be set
    // to NULL before we're done with it...
    RenderTicketPtr t = single_frame_render_;
    single_frame_render_ = nullptr;

    RenderTicketWatcher *watcher = RenderFrame(t->property("time").value<rational>(),
                                               nullptr,
                                               t->property("dry").toBool());
    video_immediate_passthroughs_[watcher].append(t);
  }

  // Completely arbitrary number. I don't know what's optimal for this yet.
  const int max_tasks = 4;

  // Handle video tasks
  rational t;
  while (video_tasks_.size() < max_tasks && queued_frame_iterator_.GetNext(&t)) {
    RenderTicketWatcher* render_task = video_tasks_.key(t);

    // We want this hash, if we're not already rendering, start render now
    if (!render_task) {
      // Don't render any hash more than once
      RenderFrame(t, viewer_node_->video_frame_cache(), false);
    }

    emit SignalCacheProxyTaskProgress(double(queued_frame_iterator_.frame_index()) / double(queued_frame_iterator_.size()));

    if (!queued_frame_iterator_.HasNext()) {
      emit StopCacheProxyTasks();
    }
  }

  // Handle audio tasks
  while (!audio_iterator_.isEmpty() && audio_tasks_.size() < max_tasks && !pause_audio_) {
    // Copy first range in list
    TimeRange r = audio_iterator_.first();

    // Limit to the minimum sample rate supported by AudioVisualWaveform - we use this value so that
    // whatever chunk we render can be summed down to the smallest mipmap whole
    r.set_out(qMin(r.out(), r.in() + AudioVisualWaveform::kMinimumSampleRate.flipped()));

    // Start job
    RenderAudio(r, true);

    audio_iterator_.remove(r);
  }
}

RenderTicketWatcher* PreviewAutoCacher::RenderFrame(Node *node, const rational& time, FrameHashCache *cache, bool dry)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("job", QVariant::fromValue(last_update_time_));
  watcher->setProperty("cache", Node::PtrToValue(cache));
  connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::VideoRendered);
  video_tasks_.insert(watcher, time);
  watcher->SetTicket(RenderManager::instance()->RenderFrame(node,
                                                            copied_viewer_node_->GetVideoParams(),
                                                            copied_viewer_node_->GetAudioParams(),
                                                            copied_color_manager_,
                                                            time,
                                                            RenderMode::kOffline,
                                                            cache,
                                                            dry ? RenderManager::kNull : RenderManager::kTexture));

  return watcher;
}

RenderTicketPtr PreviewAutoCacher::RenderAudio(Node *node, const TimeRange &r, bool generate_waveforms)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("job", QVariant::fromValue(last_update_time_));
  connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::AudioRendered);
  audio_tasks_.insert(watcher, r);

  RenderTicketPtr ticket = RenderManager::instance()->RenderAudio(node, r, copied_viewer_node_->GetAudioParams(), RenderMode::kOffline, generate_waveforms);
  watcher->SetTicket(ticket);
  return ticket;
}

void PreviewAutoCacher::RequeueFrames()
{
  delayed_requeue_timer_.stop();

  if (viewer_node_
      && (viewer_node_->video_frame_cache()->IsEnabled() || use_custom_range_)
      && viewer_node_->video_frame_cache()->HasInvalidatedRanges(viewer_node_->GetVideoLength())
      && !IsRenderingCustomRange()) {
    TimeRange using_range = use_custom_range_ ? custom_autocache_range_ : cache_range_;

    TimeRangeList invalidated = viewer_node_->video_frame_cache()->GetInvalidatedRanges(using_range);
    queued_frame_iterator_ = TimeRangeListFrameIterator(invalidated, viewer_node_->video_frame_cache()->GetTimebase());

    queued_frame_iterator_.SetCustomRange(use_custom_range_);

    emit StopCacheProxyTasks();

    if (use_custom_range_) {
      CustomCacheTask *cct = new CustomCacheTask(viewer_node_->GetLabelOrName());
      connect(this, &PreviewAutoCacher::StopCacheProxyTasks, cct, &CustomCacheTask::Finish);
      connect(this, &PreviewAutoCacher::SignalCacheProxyTaskProgress, cct, &CustomCacheTask::ProgressChanged);
      connect(cct, &CustomCacheTask::Cancelled, this, &PreviewAutoCacher::CacheProxyTaskCancelled);
      TaskManager::instance()->AddTask(cct);
    }

    use_custom_range_ = false;

    TryRender();
  }
}

void PreviewAutoCacher::ConformFinished()
{
  // Got an audio conform, requeue all the audio currently needing a conform
  last_conform_task_.Acquire();

  if (!audio_needing_conform_.isEmpty()) {
    // This list should be empty if there was a viewer switch
    foreach (const TimeRange &range, audio_needing_conform_) {
      viewer_node_->audio_playback_cache()->Invalidate(range);
    }
    audio_needing_conform_.clear();
  }
}

void PreviewAutoCacher::VideoAutoCacheEnableChanged(bool e)
{
  if (e) {
    VideoInvalidatedList(viewer_node_->video_frame_cache()->GetInvalidatedRanges(viewer_node_->GetVideoLength()));
  } else {
    CancelVideoTasks();
    queued_frame_iterator_.reset();
  }
}

void PreviewAutoCacher::AudioAutoCacheEnableChanged(bool e)
{
  if (e) {
    AudioInvalidatedList(viewer_node_->audio_playback_cache()->GetInvalidatedRanges(viewer_node_->GetAudioLength()));
  } else {
    CancelAudioTasks();
    audio_iterator_.clear();
  }
}

void PreviewAutoCacher::CacheProxyTaskCancelled()
{
  queued_frame_iterator_.reset();
  RequeueFrames();
}

void PreviewAutoCacher::ForceCacheRange(const TimeRange &range)
{
  use_custom_range_ = true;
  custom_autocache_range_ = range;

  // Re-hash these frames and start rendering
  StartCachingVideoRange(range);
}

void PreviewAutoCacher::SetViewerNode(ViewerOutput *viewer_node)
{
  if (viewer_node_ == viewer_node) {
    return;
  }

  if (viewer_node_) {
    // We must wait for any jobs to finish because they'll be using our copied graph and we're
    // about to destroy it

    // Stop requeue timer if it's running
    delayed_requeue_timer_.stop();

    // Handle video rendering tasks
    if (!video_tasks_.isEmpty()) {
      // Cancel any video tasks and wait for them to finish
      CancelVideoTasks(true);
      video_tasks_.clear();
    }

    // Handle audio rendering tasks
    if (!audio_tasks_.isEmpty()) {
      // Cancel any audio tasks and wait for them to finish
      CancelAudioTasks(true);
      audio_tasks_.clear();
    }

    // Clear iterators
    queued_frame_iterator_.reset();
    audio_iterator_.clear();

    // Clear any invalidated ranges
    invalidated_video_.clear();
    invalidated_audio_.clear();

    // Clear any single frame render that might be queued
    CancelQueuedSingleFrameRender();

    // Not interested in video passthroughs anymore
    video_immediate_passthroughs_.clear();

    // Not interested in audio conforming anymore
    audio_needing_conform_.clear();

    // Disconnect from all node cache's
    for (auto it=copy_map_.cbegin(); it!=copy_map_.cend(); it++) {
      DisconnectFromNodeCache(it.key());
    }

    // Delete all of our copied nodes
    qDeleteAll(created_nodes_);
    created_nodes_.clear();
    copy_map_.clear();
    copied_viewer_node_ = nullptr;
    graph_update_queue_.clear();
    video_job_tracker_.clear();
    audio_job_tracker_.clear();

    // Disconnect signals for future node additions/deletions
    NodeGraph* graph = viewer_node_->parent();

    disconnect(graph, &NodeGraph::NodeAdded, this, &PreviewAutoCacher::NodeAdded);
    disconnect(graph, &NodeGraph::NodeRemoved, this, &PreviewAutoCacher::NodeRemoved);
    disconnect(graph, &NodeGraph::InputConnected, this, &PreviewAutoCacher::EdgeAdded);
    disconnect(graph, &NodeGraph::InputDisconnected, this, &PreviewAutoCacher::EdgeRemoved);
    disconnect(graph, &NodeGraph::ValueChanged, this, &PreviewAutoCacher::ValueChanged);
    disconnect(graph, &NodeGraph::InputValueHintChanged, this, &PreviewAutoCacher::ValueHintChanged);
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

    // Ensure graph change value is just before the sync value
    UpdateGraphChangeValue();
    UpdateLastSyncedValue();

    // Connect signals for future node additions/deletions
    connect(graph, &NodeGraph::NodeAdded, this, &PreviewAutoCacher::NodeAdded, Qt::DirectConnection);
    connect(graph, &NodeGraph::NodeRemoved, this, &PreviewAutoCacher::NodeRemoved, Qt::DirectConnection);
    connect(graph, &NodeGraph::InputConnected, this, &PreviewAutoCacher::EdgeAdded, Qt::DirectConnection);
    connect(graph, &NodeGraph::InputDisconnected, this, &PreviewAutoCacher::EdgeRemoved, Qt::DirectConnection);
    connect(graph, &NodeGraph::ValueChanged, this, &PreviewAutoCacher::ValueChanged, Qt::DirectConnection);
    connect(graph, &NodeGraph::InputValueHintChanged, this, &PreviewAutoCacher::ValueHintChanged, Qt::DirectConnection);

    // Copy invalidated ranges and start rendering if necessary
    VideoInvalidatedList(viewer_node_->video_frame_cache()->GetInvalidatedRanges(viewer_node_->GetVideoLength()));
    AudioInvalidatedList(viewer_node_->audio_playback_cache()->GetInvalidatedRanges(viewer_node_->GetAudioLength()));
  }
}

}
