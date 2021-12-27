/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "render/rendermanager.h"
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
  delayed_requeue_timer_.setInterval(Config::Current()[QStringLiteral("AutoCacheDelay")].toInt());
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

RenderTicketPtr PreviewAutoCacher::GetSingleFrame(const rational &t, bool prioritize)
{
  // If we have a single frame render queued (but not yet sent to the RenderManager), cancel it now
  CancelQueuedSingleFrameRender();

  // See if we have a hash, we may or may not retrieve one depending on the state of the video
  // frame cache
  QByteArray hash;
  if (viewer_node_->GetVideoAutoCacheEnabled()) {
    hash = viewer_node_->video_frame_cache()->GetHash(t);
  }

  // Create a new single frame render ticket
  auto sfr = std::make_shared<RenderTicket>();
  sfr->Start();
  sfr->setProperty("time", QVariant::fromValue(t));
  sfr->setProperty("prioritize", prioritize);
  sfr->setProperty("hash", hash);

  // Queue it and try to render
  single_frame_render_ = sfr;
  TryRender();

  return sfr;
}

RenderTicketPtr PreviewAutoCacher::GetRangeOfAudio(TimeRange range, bool prioritize)
{
  return RenderAudio(range, false, prioritize);
}

QVector<PreviewAutoCacher::HashData> PreviewAutoCacher::GenerateHashes(ViewerOutput *viewer, FrameHashCache* cache, const QVector<rational> &times)
{
  QVector<HashData> hash_data(times.size());

  QVector<QByteArray> existing_hashes;

  for (int i=0; i<times.size(); i++) {
    const rational &time = times.at(i);

    // See if hash already exists in disk cache
    QByteArray hash = RenderManager::Hash(viewer->GetConnectedTextureOutput(),
                                          viewer->GetConnectedTextureValueHint(),
                                          viewer->GetVideoParams(),
                                          time);

    // Check memory list since disk checking is slow
    bool hash_exists = existing_hashes.contains(hash);

    if (!hash_exists) {
      // FIXME: Using CachePathName here is NOT thread safe and should be replaced
      hash_exists = QFileInfo::exists(cache->CachePathName(hash));

      if (hash_exists) {
        existing_hashes.push_back(hash);
      }
    }

    // Set hash in FrameHashCache's thread rather than in ours to prevent race conditions
    hash_data[i] = {time, hash, hash_exists};
  }

  return hash_data;
}

void PreviewAutoCacher::VideoInvalidated(const TimeRange &range)
{
  // Stop any current render tasks because a) they might be out of date now anyway, and b) we
  // want to dedicate all our rendering power to realtime feedback for the user
  CancelVideoTasks();

  // If auto-cache is enabled and a slider is not being dragged, queue up to hash these frames
  if (viewer_node_->GetVideoAutoCacheEnabled() && !NodeInputDragger::IsInputBeingDragged()) {
    StartCachingVideoRange(range);
  }
}

void PreviewAutoCacher::AudioInvalidated(const TimeRange &range)
{
  // We don't stop rendering audio because currently there's no system of requeuing audio if it's
  // cancelled, so some areas may end up unrendered forever
  //  ClearAudioQueue();

  // If we're auto-caching audio or require realtime waveforms, we'll have to render this
  if (viewer_node_->GetAudioAutoCacheEnabled() || kRealTimeWaveformsEnabled) {
    StartCachingAudioRange(range);
  }
}

void PreviewAutoCacher::HashesProcessed()
{
  // Receive watcher
  QFutureWatcher< QVector<HashData> >* watcher = static_cast<QFutureWatcher<QVector<HashData> >*>(sender());

  // If the task list doesn't contain this watcher, presumably it was cleared as a result of a
  // viewer switch, so we'll completely ignore this watcher
  if (hash_tasks_.contains(watcher)) {
    // Remove task from hash task list
    hash_tasks_.removeOne(watcher);

    // Set all hashes we received that are still current
    JobTime job_time = watcher->property("job").value<JobTime>();
    auto hashes = watcher->result();
    foreach (auto hash, hashes) {
      if (video_job_tracker_.isCurrent(hash.time, job_time)) {
        viewer_node_->video_frame_cache()->SetHash(hash.time, hash.hash, hash.exists);
      }
    }

    // RequeueFrames won't run if hash tasks isn't empty, so if it is, trigger it now
    if (hash_tasks_.isEmpty()) {
      delayed_requeue_timer_.stop();
      delayed_requeue_timer_.start();
    }

    // Continue rendering
    TryRender();
  }

  delete watcher;
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

      if (viewer_node_->GetAudioAutoCacheEnabled()) {
        // WritePCM is tolerant to its buffer being null, it will just write silence instead
        viewer_node_->audio_playback_cache()->WritePCM(range,
                                                       valid_ranges,
                                                       watcher->Get().value<SampleBufferPtr>());
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
      }

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

    // Continue rendering
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::VideoRendered()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  // If the task list doesn't contain this watcher, presumably it was cleared as a result of a
  // viewer switch, so we'll completely ignore this watcher
  if (video_tasks_.contains(watcher)) {
    // Assume that a "result" is a fully completed image and a non-result is a cancelled ticket
    QByteArray hash = video_tasks_.take(watcher);

    if (watcher->HasResult()) {
      // Download frame in another thread
      if (!hash.isEmpty()) {
        FramePtr frame = watcher->Get().value<FramePtr>();
        RenderTicketWatcher* w = new RenderTicketWatcher();
        w->setProperty("job", QVariant::fromValue(last_update_time_));
        w->setProperty("frame", QVariant::fromValue(frame));
        video_download_tasks_.insert(w, hash);
        connect(w, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::VideoDownloaded);
        w->SetTicket(RenderManager::instance()->SaveFrameToCache(viewer_node_->video_frame_cache(),
                                                                 frame,
                                                                 hash,
                                                                 true));
      }
    }

    // Continue rendering
    TryRender();
  }

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

  delete watcher;
}

void PreviewAutoCacher::VideoDownloaded()
{
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  // If the task list doesn't contain this watcher, presumably it was cleared as a result of a
  // viewer switch, so we'll completely ignore this watcher
  if (video_download_tasks_.contains(watcher)) {
    // Remove from task list
    QByteArray hash = video_download_tasks_.take(watcher);

    // Assume that `true` is a completely successful frame save
    if (watcher->Get().toBool()) {
      viewer_node_->video_frame_cache()->ValidateFramesWithHash(hash);
    } else {
      qCritical() << "Failed to download video frame";
    }

    // No need to call TryRender here because it would not have been held up by a download task
    // nor does the completion of this ticket automatically trigger another ticket
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

  // Copy UUID
  copy->SetUUID(node->GetUUID());

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
  // Copy all values to our graph
  Node* our_input = copy_map_.value(input.node());
  Node::CopyValuesOfElement(input.node(), our_input, input.input(), input.element());
}

void PreviewAutoCacher::CopyValueHint(const NodeInput &input)
{
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

  TryRender();
}

void PreviewAutoCacher::StartCachingVideoRange(const TimeRange &range)
{
  StartCachingRange(range, &invalidated_video_, &video_job_tracker_);
}

void PreviewAutoCacher::StartCachingAudioRange(const TimeRange &range)
{
  StartCachingRange(range, &invalidated_audio_, &audio_job_tracker_);
}

void PreviewAutoCacher::SetPlayhead(const rational &playhead)
{
  cache_range_ = TimeRange(playhead - Config::Current()[QStringLiteral("DiskCacheBehind")].value<rational>(),
      playhead + Config::Current()[QStringLiteral("DiskCacheAhead")].value<rational>());

  RequeueFrames();
}

void PreviewAutoCacher::WaitForHashesToFinish()
{
  for (auto it=hash_tasks_.cbegin(); it!=hash_tasks_.cend(); it++) {
    (*it)->waitForFinished();
  }
}

void PreviewAutoCacher::WaitForVideoDownloadsToFinish()
{
  for (auto it=video_download_tasks_.cbegin(); it!=video_download_tasks_.cend(); it++) {
    it.key()->WaitForFinished();
  }
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
    if (!hash_tasks_.isEmpty()
        || !audio_tasks_.isEmpty()
        || !video_tasks_.isEmpty()) {
      return;
    }

    // No jobs are active, we can process the update queue
    ProcessUpdateQueue();
  }

  // Check for newly invalidated video and hash it
  if (!invalidated_video_.isEmpty()) {
    if (!copied_viewer_node_->GetConnectedTextureOutput()) {
      hash_iterator_.reset();
    } else if (hash_iterator_.HasNext()) {
      hash_iterator_.insert(invalidated_video_);
    } else {
      hash_iterator_ = TimeRangeListFrameIterator(invalidated_video_, viewer_node_->GetVideoParams().frame_rate_as_time_base());
    }
    invalidated_video_.clear();
  }

  if (!invalidated_audio_.isEmpty()) {
    // Add newly invalidated audio to iterator
    audio_iterator_.insert(invalidated_audio_);
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
                            !viewer_node_->GetVideoAutoCacheEnabled());

      video_immediate_passthroughs_[watcher].append(single_frame_render_);
    }

    single_frame_render_ = nullptr;
  }

  // Ensure we are running tasks if we have any
  const int max_tasks = RenderManager::GetNumberOfIdealConcurrentJobs();

  // Handle hash tasks
  while (hash_tasks_.size() < max_tasks && hash_iterator_.HasNext()) {
    // Magic number: dunno what the best number for this is yet
    static const int kMaxFrames = 1000;

    QVector<rational> times(kMaxFrames);
    for (int i=0; i<kMaxFrames; i++) {
      rational r;
      if (hash_iterator_.GetNext(&r)) {
        times[i] = r;
      } else {
        times.resize(i);
        break;
      }
    }

    QFutureWatcher< QVector<HashData> >* watcher = new QFutureWatcher< QVector<HashData> >();
    watcher->setProperty("job", QVariant::fromValue(last_update_time_));
    hash_tasks_.append(watcher);
    connect(watcher, &QFutureWatcher< QVector<HashData> >::finished, this, &PreviewAutoCacher::HashesProcessed);
    watcher->setFuture(QtConcurrent::run(PreviewAutoCacher::GenerateHashes,
                                         copied_viewer_node_,
                                         viewer_node_->video_frame_cache(),
                                         times));
  }

  // Handle video tasks
  rational t;
  while (video_tasks_.size() < max_tasks && queued_frame_iterator_.GetNext(&t)) {
    const QByteArray& hash = viewer_node_->video_frame_cache()->GetHash(t);

    RenderTicketWatcher* render_task = video_tasks_.key(hash);

    // We want this hash, if we're not already rendering, start render now
    if (!render_task && !video_download_tasks_.key(hash)) {
      // Don't render any hash more than once
      RenderFrame(hash, t, false, false);
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
    RenderAudio(r, true, false);

    audio_iterator_.remove(r);
  }
}

RenderTicketWatcher* PreviewAutoCacher::RenderFrame(const QByteArray &hash, const rational& time, bool prioritize, bool texture_only)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("hash", hash);
  watcher->setProperty("job", QVariant::fromValue(last_update_time_));
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

RenderTicketPtr PreviewAutoCacher::RenderAudio(const TimeRange &r, bool generate_waveforms, bool prioritize)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("job", QVariant::fromValue(last_update_time_));
  connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::AudioRendered);
  audio_tasks_.insert(watcher, r);

  RenderTicketPtr ticket = RenderManager::instance()->RenderAudio(copied_viewer_node_, r, RenderMode::kOffline, generate_waveforms, prioritize);
  watcher->SetTicket(ticket);
  return ticket;
}

void PreviewAutoCacher::RequeueFrames()
{
  delayed_requeue_timer_.stop();

  if (viewer_node_
      && (viewer_node_->GetVideoAutoCacheEnabled() || use_custom_range_)
      && viewer_node_->video_frame_cache()->HasInvalidatedRanges(viewer_node_->GetVideoLength())
      && hash_tasks_.isEmpty()
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

    // Handle hashes
    if (!hash_tasks_.isEmpty()) {
      // Wait for hashes to finish
      WaitForHashesToFinish();

      // Clear the hash list to indicate we're not interested in the results of any of these
      hash_tasks_.clear();
    }

    // Handle video rendering tasks
    if (!video_tasks_.isEmpty()) {
      // Cancel any video tasks and wait for them to finish
      CancelVideoTasks(true);
    }

    // Handle audio rendering tasks
    if (!audio_tasks_.isEmpty()) {
      // Cancel any audio tasks and wait for them to finish
      CancelAudioTasks(true);
    }

    // Handle video download tasks
    if (!video_download_tasks_.isEmpty()) {
      WaitForVideoDownloadsToFinish();
    }

    // Clear iterators
    queued_frame_iterator_.reset();
    audio_iterator_.clear();
    hash_iterator_.reset();

    // Clear any invalidated ranges
    invalidated_video_.clear();
    invalidated_audio_.clear();

    // Clear any single frame render that might be queued
    CancelQueuedSingleFrameRender();

    // Not interested in video passthroughs anymore
    video_immediate_passthroughs_.clear();

    // Not interested in audio conforming anymore
    audio_needing_conform_.clear();

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

    // Disconnect signal (will be a no-op if the signal was never connected)
    disconnect(viewer_node_,
               &ViewerOutput::VideoAutoCacheChanged,
               this,
               &PreviewAutoCacher::VideoAutoCacheEnableChanged);

    disconnect(viewer_node_,
               &ViewerOutput::AudioAutoCacheChanged,
               this,
               &PreviewAutoCacher::AudioAutoCacheEnableChanged);

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
    copied_viewer_node_->SetViewerVideoCacheEnabled(false);
    copied_viewer_node_->SetViewerAudioCacheEnabled(false);
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

    connect(viewer_node_,
            &ViewerOutput::VideoAutoCacheChanged,
            this,
            &PreviewAutoCacher::VideoAutoCacheEnableChanged);

    connect(viewer_node_,
            &ViewerOutput::AudioAutoCacheChanged,
            this,
            &PreviewAutoCacher::AudioAutoCacheEnableChanged);

    connect(viewer_node_->video_frame_cache(),
            &PlaybackCache::Invalidated,
            this,
            &PreviewAutoCacher::VideoInvalidated);

    connect(viewer_node_->audio_playback_cache(),
            &PlaybackCache::Invalidated,
            this,
            &PreviewAutoCacher::AudioInvalidated);

    // Copy invalidated ranges and start rendering if necessary
    VideoInvalidatedList(viewer_node_->video_frame_cache()->GetInvalidatedRanges(viewer_node_->GetVideoLength()));
    AudioInvalidatedList(viewer_node_->audio_playback_cache()->GetInvalidatedRanges(viewer_node_->GetAudioLength()));
  }
}

}
