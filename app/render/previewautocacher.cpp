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
#include "node/input/multicam/multicamnode.h"
#include "node/inputdragger.h"
#include "node/project/project.h"
#include "render/diskmanager.h"

namespace olive {

PreviewAutoCacher::PreviewAutoCacher(QObject *parent) :
  QObject(parent),
  viewer_node_(nullptr),
  use_custom_range_(false),
  pause_renders_(false),
  pause_thumbnails_(false),
  single_frame_render_(nullptr),
  display_color_processor_(nullptr),
  multicam_(nullptr),
  ignore_cache_requests_(false)
{
  copier_ = new ProjectCopier(this);
  connect(copier_, &ProjectCopier::AddedNode, this, &PreviewAutoCacher::ConnectToNodeCache);
  connect(copier_, &ProjectCopier::RemovedNode, this, &PreviewAutoCacher::DisconnectFromNodeCache);

  // Set defaults
  SetPlayhead(0);

  // Wait a certain amount of time before requeuing when we receive an invalidate signal
  delayed_requeue_timer_.setInterval(OLIVE_CONFIG("AutoCacheDelay").toInt());
  delayed_requeue_timer_.setSingleShot(true);
  connect(&delayed_requeue_timer_, &QTimer::timeout, this, &PreviewAutoCacher::TryRender);

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
  return GetSingleFrame(viewer_node_->GetConnectedTextureOutput(), t, dry);
}

RenderTicketPtr PreviewAutoCacher::GetSingleFrame(Node *n, const rational &t, bool dry)
{
  // If we have a single frame render queued (but not yet sent to the RenderManager), cancel it now
  CancelQueuedSingleFrameRender();

  // Create a new single frame render ticket
  auto sfr = std::make_shared<RenderTicket>();
  sfr->Start();
  sfr->setProperty("time", QVariant::fromValue(t));
  sfr->setProperty("dry", dry);
  sfr->setProperty("node", Node::PtrToValue(n));

  // Queue it and try to render
  single_frame_render_ = sfr;
  TryRender();

  return sfr;
}

RenderTicketPtr PreviewAutoCacher::GetRangeOfAudio(TimeRange range)
{
  return RenderAudio(copied_viewer_node_->GetConnectedSampleOutput(), range, nullptr);
}

void PreviewAutoCacher::ClearSingleFrameRenders()
{
  QMap<RenderTicketWatcher*, QVector<RenderTicketPtr> > copy = video_immediate_passthroughs_;
  for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
    it.key()->Cancel();
    if (!it.key()->IsRunning()) {
      RenderManager::instance()->RemoveTicket(it.key()->GetTicket());
      emit it.key()->GetTicket()->Finished();
    }
  }
}

void PreviewAutoCacher::VideoInvalidatedFromCache(const TimeRange &range)
{
  PlaybackCache *cache = static_cast<PlaybackCache*>(sender());

  cache->ClearRequestRange(range);

  VideoInvalidatedFromNode(cache, range);
}

void PreviewAutoCacher::AudioInvalidatedFromCache(const TimeRange &range)
{
  PlaybackCache *cache = static_cast<PlaybackCache*>(sender());

  cache->ClearRequestRange(range);

  AudioInvalidatedFromNode(cache, range);
}

void PreviewAutoCacher::CancelForCache()
{
  PlaybackCache *cache = static_cast<PlaybackCache*>(sender());

  if (dynamic_cast<FrameHashCache*>(cache) || dynamic_cast<ThumbnailCache*>(cache)) {
    for (auto it=pending_video_jobs_.begin(); it!=pending_video_jobs_.end(); ) {
      if ((*it).cache == cache) {
        it = pending_video_jobs_.erase(it);
      } else {
        it++;
      }
    }
  } else if (dynamic_cast<AudioPlaybackCache*>(cache) || dynamic_cast<AudioWaveformCache*>(cache)) {
    for (auto it=pending_audio_jobs_.begin(); it!=pending_audio_jobs_.end(); ) {
      if ((*it).cache == cache) {
        it = pending_audio_jobs_.erase(it);
      } else {
        it++;
      }
    }
  }
}

void PreviewAutoCacher::AudioRendered()
{
  // Receive watcher
  RenderTicketWatcher* watcher = static_cast<RenderTicketWatcher*>(sender());

  // If the task list doesn't contain this watcher, presumably it was cleared as a result of a
  // viewer switch, so we'll completely ignore this watcher
  if (running_audio_tasks_.removeOne(watcher)) {
    // Assume that a "result" is a fully completed image and a non-result is a cancelled ticket
    TimeRange range = watcher->property("time").value<TimeRange>();
    Node *node = copier_->GetOriginal(Node::ValueToPtr<Node>(watcher->property("node")));

    if (watcher->HasResult() && node) {
      if (PlaybackCache *cache = Node::ValueToPtr<PlaybackCache>(watcher->property("cache"))) {
        AudioCacheData &d = audio_cache_data_[cache];

        JobTime watcher_job_time = watcher->property("job").value<JobTime>();

        TimeRangeList valid_ranges = d.job_tracker.getCurrentSubRanges(range, watcher_job_time);

        AudioVisualWaveform waveform = watcher->GetTicket()->property("waveform").value<AudioVisualWaveform>();

        SampleBuffer buf = watcher->Get().value<SampleBuffer>();

        bool incomplete = watcher->GetTicket()->property("incomplete").toBool();

        if (AudioPlaybackCache *pcm = dynamic_cast<AudioPlaybackCache*>(cache)) {
          // WritePCM is tolerant to its buffer being null, it will just write silence instead
          pcm->SetParameters(buf.audio_params());
          pcm->WritePCM(range,
                        valid_ranges,
                        watcher->Get().value<SampleBuffer>());
        } else if (AudioWaveformCache *wave = dynamic_cast<AudioWaveformCache*>(cache)) {
          wave->SetParameters(buf.audio_params());
          if (!incomplete) {
            wave->WriteWaveform(range, valid_ranges, &waveform);
          }
        }

        if (incomplete) {
          if (last_conform_task_ > watcher_job_time) {
            // Requeue now
            cache->Invalidate(range);
          } else {
            // Wait for conform
            d.needs_conform.insert(range);
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

  const QStringList bad_cache_names = watcher->GetTicket()->property("badcache").toStringList();
  if (!bad_cache_names.empty()) {
    for (const QString &fn : bad_cache_names) {
      DiskManager::instance()->DeleteSpecificFile(fn);
    }
  }

  // Process passthroughs no matter what, if the viewer was switched, the passthrough map would be
  // cleared anyway
  QVector<RenderTicketPtr> tickets = video_immediate_passthroughs_.take(watcher);
  foreach (RenderTicketPtr t, tickets) {
    if (watcher->HasResult()) {
      t->setProperty("multicam_output", watcher->GetTicket()->property("multicam_output"));
      t->Finish(watcher->Get());
    } else {
      t->Finish();
    }
  }

  // If the task list doesn't contain this watcher, presumably it was cleared as a result of a
  // viewer switch, so we'll completely ignore this watcher
  if (running_video_tasks_.removeOne(watcher)) {
    // Assume that a "result" is a fully completed image and a non-result is a cancelled ticket
    if (watcher->HasResult()) {
      if (watcher->GetTicket()->property("cached").toBool()) {
        if (FrameHashCache *cache = Node::ValueToPtr<FrameHashCache>(watcher->property("cache"))) {
          rational time = watcher->property("time").value<rational>();
          JobTime job = watcher->property("job").value<JobTime>();

          if (video_cache_data_.value(cache).job_tracker.isCurrent(time, job)) {
            cache->ValidateTime(time);
          }
        }
      }
    }

    // Continue rendering
    TryRender();
  }

  delete watcher;
}

void PreviewAutoCacher::ConnectToNodeCache(Node *node)
{
  if (ignore_cache_requests_) {
    return;
  }

  connect(node->video_frame_cache(),
          &PlaybackCache::Requested,
          this,
          &PreviewAutoCacher::VideoInvalidatedFromCache);

  connect(node->thumbnail_cache(),
          &PlaybackCache::Requested,
          this,
          &PreviewAutoCacher::VideoInvalidatedFromCache);

  connect(node->audio_playback_cache(),
          &PlaybackCache::Requested,
          this,
          &PreviewAutoCacher::AudioInvalidatedFromCache);

  connect(node->waveform_cache(),
          &PlaybackCache::Requested,
          this,
          &PreviewAutoCacher::AudioInvalidatedFromCache);

  connect(node->video_frame_cache(),
          &PlaybackCache::CancelAll,
          this,
          &PreviewAutoCacher::CancelForCache);

  connect(node->audio_playback_cache(),
          &PlaybackCache::CancelAll,
          this,
          &PreviewAutoCacher::CancelForCache);

  node->video_frame_cache()->ResignalRequests();
  node->thumbnail_cache()->ResignalRequests();
  node->audio_playback_cache()->ResignalRequests();
  node->waveform_cache()->ResignalRequests();
}

void PreviewAutoCacher::DisconnectFromNodeCache(Node *node)
{
  disconnect(node->video_frame_cache(),
             &PlaybackCache::Requested,
             this,
             &PreviewAutoCacher::VideoInvalidatedFromCache);

  disconnect(node->thumbnail_cache(),
             &PlaybackCache::Requested,
             this,
             &PreviewAutoCacher::VideoInvalidatedFromCache);

  disconnect(node->audio_playback_cache(),
             &PlaybackCache::Requested,
             this,
             &PreviewAutoCacher::AudioInvalidatedFromCache);

  disconnect(node->waveform_cache(),
             &PlaybackCache::Requested,
             this,
             &PreviewAutoCacher::AudioInvalidatedFromCache);

  disconnect(node->video_frame_cache(),
             &PlaybackCache::CancelAll,
             this,
             &PreviewAutoCacher::CancelForCache);

  disconnect(node->audio_playback_cache(),
             &PlaybackCache::CancelAll,
             this,
             &PreviewAutoCacher::CancelForCache);
}

void PreviewAutoCacher::CancelQueuedSingleFrameRender()
{
  if (single_frame_render_) {
    // Signal that this ticket was cancelled with no value
    single_frame_render_->Finish();
    single_frame_render_ = nullptr;
  }
}

void PreviewAutoCacher::StartCachingRange(const TimeRange &range, TimeRangeList *range_list, RenderJobTracker *tracker)
{
  range_list->insert(range);
  tracker->insert(range, copier_->GetGraphChangeTime());
}

void PreviewAutoCacher::StartCachingVideoRange(PlaybackCache *cache, const TimeRange &range)
{
  Node *node = cache->parent();
  rational using_tb;
  if (ThumbnailCache *thumbs = dynamic_cast<ThumbnailCache*>(cache)) {
    using_tb = thumbs->GetTimebase();
  } else {
    using_tb = viewer_node_->GetVideoParams().frame_rate_as_time_base();
  }

  cache->ClearRequestRange(range);

  TimeRangeListFrameIterator iterator({range}, using_tb);
  pending_video_jobs_.push_back({node, cache, range, iterator});
  video_cache_data_[cache].job_tracker.insert(TimeRange(iterator.Snap(range.in()), range.out()), copier_->GetGraphChangeTime());
  TryRender();
}

void PreviewAutoCacher::StartCachingAudioRange(PlaybackCache *cache, const TimeRange &range)
{
  Node *node = cache->parent();

  cache->ClearRequestRange(range);

  pending_audio_jobs_.push_back({node, cache, range});
  audio_cache_data_[cache].job_tracker.insert(range, copier_->GetGraphChangeTime());
  TryRender();
}

void PreviewAutoCacher::VideoInvalidatedFromNode(PlaybackCache *cache, const TimeRange &range)
{
  // Ignore render requests if no video is present
  if (!viewer_node_ || !viewer_node_->GetVideoParams().is_valid()) {
    return;
  }

  // Stop any current render tasks because a) they might be out of date now anyway, and b) we
  // want to dedicate all our rendering power to realtime feedback for the user
  //CancelVideoTasks(node);

  cache->ClearRequestRange(range);

  // If auto-cache is enabled and a slider is not being dragged, queue up to hash these frames
  if (!NodeInputDragger::IsInputBeingDragged()) {
    StartCachingVideoRange(cache, range);
  }
}

void PreviewAutoCacher::AudioInvalidatedFromNode(PlaybackCache *cache, const TimeRange &range)
{
  // Ignore render requests if no video is present
  if (!viewer_node_ || !viewer_node_->GetAudioParams().is_valid()) {
    return;
  }

  // We don't stop rendering audio because currently there's no system of requeuing audio if it's
  // cancelled, so some areas may end up unrendered forever
  //  ClearAudioQueue();

  cache->ClearRequestRange(range);

  // If we're auto-caching audio or require realtime waveforms, we'll have to render this
  StartCachingAudioRange(cache, range);
}

void PreviewAutoCacher::SetPlayhead(const rational &playhead)
{
  cache_range_ = TimeRange(playhead - OLIVE_CONFIG("DiskCacheBehind").value<rational>(),
                           playhead + OLIVE_CONFIG("DiskCacheAhead").value<rational>());

  TryRender();
}

template<typename T>
void CancelTasks(const T &task_list, bool and_wait)
{
  for (auto it=task_list.cbegin(); it!=task_list.cend(); it++) {
    // Signal that the ticket should not be finished
    (*it)->Cancel();
  }

  if (and_wait) {
    // Wait for each ticket to finish
    for (auto it=task_list.cbegin(); it!=task_list.cend(); it++) {
      (*it)->WaitForFinished();
    }
  }
}

void PreviewAutoCacher::CancelVideoTasks(bool and_wait_for_them_to_finish)
{
  CancelTasks(running_video_tasks_, and_wait_for_them_to_finish);
}

void PreviewAutoCacher::CancelAudioTasks(bool and_wait_for_them_to_finish)
{
  CancelTasks(running_audio_tasks_, and_wait_for_them_to_finish);
}

bool PreviewAutoCacher::IsRenderingCustomRange() const
{
  /*const VideoCacheData &d = video_cache_data_.value(viewer_node_);
  return d.iterator.IsCustomRange() && d.iterator.HasNext();*/
  return false;
}

void PreviewAutoCacher::SetRendersPaused(bool e)
{
  pause_renders_ = e;
  if (!e) {
    TryRender();
  }
}

void PreviewAutoCacher::SetThumbnailsPaused(bool e)
{
  pause_thumbnails_ = e;
  if (!e) {
    TryRender();
  }
}

void PreviewAutoCacher::TryRender()
{
  delayed_requeue_timer_.stop();

  if (copier_->HasUpdatesInQueue()) {
    // Check if we have jobs running in other threads that shouldn't be interrupted right now
    // NOTE: We don't check for downloads because, while they run in another thread, they don't
    //       require any access to the graph and therefore don't risk race conditions.
    if (!running_audio_tasks_.isEmpty()
        || !running_video_tasks_.isEmpty()) {
      return;
    }

    // No jobs are active, we can process the update queue
    copier_->ProcessUpdateQueue();
  }

  if (single_frame_render_) {
    // Make an explicit copy of the render ticket here - it seems that on some systems it can be set
    // to NULL before we're done with it...
    RenderTicketPtr t = single_frame_render_;
    single_frame_render_ = nullptr;

    // Check if already caching this
    Node *n = Node::ValueToPtr<Node>(t->property("node"));
    Node *copy = copier_->GetCopy(n);

    if (copy) {
      RenderTicketWatcher *watcher = RenderFrame(copy,
                                                 t->property("time").value<rational>(),
                                                 nullptr,
                                                 t->property("dry").toBool());
      video_immediate_passthroughs_[watcher].append(t);
    } else {
      qWarning() << "Failed to find copied node for SFR ticket";
      t->Finish();
    }
  }

  if (!pause_renders_) {
    // Completely arbitrary number. I don't know what's optimal for this yet.
    const int max_tasks = 4;

    // Handle video tasks
    if (!pause_thumbnails_) {
      while (!pending_video_jobs_.empty()) {
        VideoJob &d = pending_video_jobs_.front();

        if (Node *copy = copier_->GetCopy(d.node)) {
          // Queue next frames
          rational t;
          while (running_video_tasks_.size() < max_tasks && d.iterator.GetNext(&t)) {
            RenderFrame(copy, t, d.cache, false);

            emit SignalCacheProxyTaskProgress(double(d.iterator.frame_index()) / double(d.iterator.size()));

            if (!d.iterator.HasNext()) {
              emit StopCacheProxyTasks();
            }
          }
        } else {
          qCritical() << "Failed to find node copy for video job";
        }

        if (d.iterator.HasNext()) {
          break;
        } else {
          pending_video_jobs_.pop_front();
        }
      }
    }

    // Handle audio tasks
    while (!pending_audio_jobs_.empty() && running_audio_tasks_.size() < max_tasks) {
      AudioJob &d = pending_audio_jobs_.front();

      bool pop = true;

      // Start job
      if (Node *copy = copier_->GetCopy(d.node)) {
        TimeRange &queued_range = d.range;
        TimeRange use_range = queued_range;

        if (dynamic_cast<AudioWaveformCache*>(d.cache)) {
          rational new_out = std::min(use_range.in() + AudioVisualWaveform::kMinimumSampleRate.flipped(), use_range.out());

          if (new_out != use_range.out()) {
            use_range.set_out(new_out);
            queued_range.set_in(new_out);
            pop = false;
          }
        }

        RenderAudio(copy, use_range, d.cache);
      } else {
        qCritical() << "Failed to find node copy for audio job";
      }

      if (pop) {
        pending_audio_jobs_.pop_front();
      }
    }
  }
}

RenderTicketWatcher* PreviewAutoCacher::RenderFrame(Node *node, const rational& time, PlaybackCache *cache, bool dry)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("job", QVariant::fromValue(copier_->GetLastUpdateTime()));
  watcher->setProperty("cache", Node::PtrToValue(cache));
  watcher->setProperty("time", QVariant::fromValue(time));
  connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::VideoRendered);

  running_video_tasks_.append(watcher);

  RenderManager::RenderVideoParams rvp(node,
                                       copied_viewer_node_->GetVideoParams(),
                                       copied_viewer_node_->GetAudioParams(),
                                       time,
                                       copied_color_manager_,
                                       RenderMode::kOffline);

  if (FrameHashCache *frame_cache = dynamic_cast<FrameHashCache *>(cache)) {
    if (ThumbnailCache *wave_cache = dynamic_cast<ThumbnailCache *>(cache)) {
      rvp.video_params.set_divider(VideoParams::GetDividerForTargetResolution(rvp.video_params.width(), rvp.video_params.height(), 160, 120));
      rvp.force_color_output = display_color_processor_;
      rvp.force_format = PixelFormat::U8;
    } else {
      frame_cache->SetTimebase(viewer_node_->GetVideoParams().frame_rate_as_time_base());
    }

    rvp.AddCache(frame_cache);
  }

  rvp.return_type = dry ? RenderManager::kNull : RenderManager::kTexture;

  // Allow using cached images for this render job
  rvp.use_cache = true;

  // Multicam
  rvp.multicam = copier_->GetCopy(multicam_);

  watcher->SetTicket(RenderManager::instance()->RenderFrame(rvp));

  return watcher;
}

RenderTicketPtr PreviewAutoCacher::RenderAudio(Node *node, const TimeRange &r, PlaybackCache *cache)
{
  RenderTicketWatcher* watcher = new RenderTicketWatcher();
  watcher->setProperty("job", QVariant::fromValue(copier_->GetLastUpdateTime()));
  watcher->setProperty("node", Node::PtrToValue(node));
  watcher->setProperty("cache", Node::PtrToValue(cache));
  watcher->setProperty("time", QVariant::fromValue(r));
  connect(watcher, &RenderTicketWatcher::Finished, this, &PreviewAutoCacher::AudioRendered);
  running_audio_tasks_.append(watcher);

  RenderManager::RenderAudioParams rap(node,
                                       r,
                                       copied_viewer_node_->GetAudioParams(),
                                       RenderMode::kOffline);

  rap.generate_waveforms = dynamic_cast<AudioWaveformCache*>(cache);
  rap.clamp = false;

  RenderTicketPtr ticket = RenderManager::instance()->RenderAudio(rap);
  watcher->SetTicket(ticket);
  return ticket;
}

void PreviewAutoCacher::ConformFinished()
{
  // Got an audio conform, requeue all the audio currently needing a conform
  last_conform_task_.Acquire();

  for (auto it=audio_cache_data_.begin(); it!=audio_cache_data_.end(); it++) {
    foreach (const TimeRange &range, it.value().needs_conform) {
      it.key()->Request(range);
    }
    it.value().needs_conform.clear();
  }
}

void PreviewAutoCacher::CacheProxyTaskCancelled()
{
  pending_video_jobs_.clear();

  TryRender();
}

void PreviewAutoCacher::ForceCacheRange(const TimeRange &range)
{
  use_custom_range_ = true;
  custom_autocache_range_ = range;

  // Re-hash these frames and start rendering
  StartCachingVideoRange(viewer_node_->video_frame_cache(), range);
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
    if (!running_video_tasks_.isEmpty()) {
      // Cancel any video tasks and wait for them to finish
      CancelVideoTasks(true);
      running_video_tasks_.clear();
    }

    // Handle audio rendering tasks
    if (!running_audio_tasks_.isEmpty()) {
      // Cancel any audio tasks and wait for them to finish
      CancelAudioTasks(true);
      running_audio_tasks_.clear();
    }

    // Clear any single frame render that might be queued
    CancelQueuedSingleFrameRender();

    // Not interested in video passthroughs anymore
    video_immediate_passthroughs_.clear();

    // Disconnect from all node cache's
    for (auto it=copier_->GetNodeMap().cbegin(); it!=copier_->GetNodeMap().cend(); it++) {
      DisconnectFromNodeCache(it.key());
    }

    // Delete all of our copied nodes
    copier_->SetProject(nullptr);
    copied_viewer_node_ = nullptr;

    // Ensure all cache data is cleared
    video_cache_data_.clear();
    audio_cache_data_.clear();

    // Clear multicam reference
    multicam_ = nullptr;
  }

  viewer_node_ = viewer_node;

  if (viewer_node_) {
    // Copy graph (this should always be a Project)
    Project* graph = static_cast<Project*>(viewer_node_->parent());

    SetRendersPaused(true);

    copier_->SetProject(graph);

    for (int i=0; i<graph->nodes().size(); i++) {
      graph->nodes().at(i)->ConnectedToPreviewEvent();
    }

    // Find copied viewer node
    copied_viewer_node_ = copier_->GetCopy(viewer_node_);
    copied_color_manager_ = copier_->GetCopy(graph->color_manager());

    SetRendersPaused(false);
  }
}

}
