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

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  viewer_node_(nullptr),
  auto_audio_(true),
  ic_from_conform_(false)
{
  // FIXME: Don't create in CLI mode
  //cancel_dialog_ = new RenderCancelDialog(Core::instance()->main_window());
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

  if (viewer_node_) {
    // Clear queue and wait for any currently running actions to complete
    CancelQueue();

    // Delete all of our copied nodes
    hash_pool_.Close();
    video_pool_.Close();
    audio_pool_.Close();
    queued_audio_.clear();

    disconnect(viewer_node_,
               &ViewerOutput::GraphChangedFrom,
               this,
               &RenderBackend::NodeGraphChanged);

    disconnect(viewer_node_->audio_playback_cache(),
               &AudioPlaybackCache::Invalidated,
               this,
               &RenderBackend::AudioInvalidated);
  }

  // Set viewer node
  viewer_node_ = viewer_node;

  if (viewer_node_) {
    // Initiate instances with new node
    hash_pool_.Init(viewer_node_);
    video_pool_.Init(viewer_node_);
    audio_pool_.Init(viewer_node_);

    connect(viewer_node_,
            &ViewerOutput::GraphChangedFrom,
            this,
            &RenderBackend::NodeGraphChanged);

    if (auto_audio_) {
      // Listen for audio invalidation signals
      connect(viewer_node_->audio_playback_cache(),
              &AudioPlaybackCache::Invalidated,
              this,
              &RenderBackend::AudioInvalidated);

      // Start caching audio
      foreach (const TimeRange& r, viewer_node_->audio_playback_cache()->GetInvalidatedRanges()) {
        AudioInvalidated(r);
      }
    }
  }
}

void RenderBackend::CancelQueue()
{
  // FIXME: Implement something better than this...
  video_pool_.threads.waitForDone();
  audio_pool_.threads.waitForDone();
  hash_pool_.threads.waitForDone();
}

QFuture<QByteArray> RenderBackend::Hash(const rational &time, bool block_for_update)
{
  return QtConcurrent::run(&hash_pool_.threads,
                           GetInstanceFromPool(hash_pool_),
                           &RenderWorker::Hash,
                           time,
                           block_for_update);
}

QFuture<FramePtr> RenderBackend::RenderFrame(const rational &time, bool clear_queue, bool block_for_update)
{
  if (clear_queue) {
    video_pool_.threads.clear();
  }

  return QtConcurrent::run(&video_pool_.threads,
                           GetInstanceFromPool(video_pool_),
                           &RenderWorker::RenderFrame,
                           time,
                           block_for_update);
}

QFuture<SampleBufferPtr> RenderBackend::RenderAudio(const TimeRange &r, bool block_for_update)
{
  return QtConcurrent::run(&audio_pool_.threads,
                           GetInstanceFromPool(audio_pool_),
                           &RenderWorker::RenderAudio,
                           r,
                           block_for_update);
}

void RenderBackend::SetVideoParams(const VideoRenderingParams &params)
{
  video_params_ = params;
}

void RenderBackend::SetAudioParams(const AudioRenderingParams &params)
{
  audio_params_ = params;
}

void RenderBackend::SetVideoDownloadMatrix(const QMatrix4x4 &mat)
{
  video_download_matrix_ = mat;
}

void RenderBackend::SetAutomaticAudio(bool e)
{
  auto_audio_ = e;
}

void RenderBackend::WorkerStartedRenderingAudio(const TimeRange &r)
{
  queued_audio_lock_.lock();
  queued_audio_.RemoveTimeRange(r);
  queued_audio_lock_.unlock();
}

QList<TimeRange> RenderBackend::SplitRangeIntoChunks(const TimeRange &r)
{
  const int chunk_size = 2;

  QList<TimeRange> split_ranges;

  int start_time = qFloor(r.in().toDouble() / static_cast<double>(chunk_size)) * chunk_size;
  int end_time = qCeil(r.out().toDouble() / static_cast<double>(chunk_size)) * chunk_size;

  for (int i=start_time; i<end_time; i+=chunk_size) {
    split_ranges.append(TimeRange(qMax(r.in(), rational(i)),
                         qMin(r.out(), rational(i + chunk_size))));
  }

  return split_ranges;
}

void RenderBackend::NodeGraphChanged(NodeInput *source)
{
  video_pool_.Queue(source);
  audio_pool_.Queue(source);
  hash_pool_.Queue(source);
}

void RenderBackend::UpdateInstance(RenderWorker *instance)
{
  instance->SetAvailable(false);
  instance->ProcessQueue();
  instance->SetVideoParams(video_params_);
  instance->SetAudioParams(audio_params_);
  instance->SetVideoDownloadMatrix(video_download_matrix_);
  instance->SetAudioModeIsPreview(auto_audio_);
}

void RenderBackend::Close()
{
  CancelQueue();

  video_pool_.Destroy();
  audio_pool_.Destroy();
  hash_pool_.Destroy();
}

RenderWorker *RenderBackend::GetInstanceFromPool(RenderPool& pool)
{
  RenderWorker* instance = nullptr;

  foreach (RenderWorker* worker, pool.instances) {
    if (worker->IsAvailable()) {
      instance = worker;
      break;
    }
  }

  if (!instance) {
    if (pool.instances.size() < pool.threads.maxThreadCount()) {
      // Can create another instance
      instance = CreateNewWorker();
      pool.instances.append(instance);

      if (viewer_node_) {
        instance->Init(viewer_node_);
      }

      connect(instance, &RenderWorker::FinishedJob,
              this, &RenderBackend::WorkerFinished, Qt::QueuedConnection);

      connect(instance, &RenderWorker::AudioConformUnavailable,
              this, &RenderBackend::AudioConformUnavailable, Qt::QueuedConnection);
    } else {
      instance = pool.instances.at(pool.queuer % pool.instances.size());
      pool.queuer++;
    }
  }

  return instance;
}

void RenderBackend::ListenForConformSignal(AudioStreamPtr s)
{
  foreach (const ConformWaitInfo& info, conform_wait_info_) {
    if (info.stream == s) {
      // We've probably already connected to this one
      return;
    }
  }

  connect(s.get(), &AudioStream::ConformAppended, this, &RenderBackend::AudioConformUpdated);
}

void RenderBackend::StopListeningForConformSignal(AudioStream *s)
{
  foreach (const ConformWaitInfo& info, conform_wait_info_) {
    if (info.stream.get() == s) {
      // There are still conforms we're waiting for, don't disconnect
      return;
    }
  }

  disconnect(s, &AudioStream::ConformAppended, this, &RenderBackend::AudioConformUpdated);
}

void RenderBackend::AudioConformUnavailable(StreamPtr stream, TimeRange range, rational stream_time, AudioRenderingParams params)
{
  ConformWaitInfo info = {stream, params, range, stream_time};

  if (conform_wait_info_.contains(info)) {
    return;
  }

  AudioStreamPtr audio_stream = std::static_pointer_cast<AudioStream>(stream);

  if (audio_stream->try_start_conforming(params)) {

    // Start indexing process
    ListenForConformSignal(audio_stream);

    conform_wait_info_.append(info);

    ConformTask* conform_task = new ConformTask(audio_stream, params);

    TaskManager::instance()->AddTask(conform_task);

  } else if (audio_stream->has_conformed_version(params)) {

    // Conform JUST finished, requeue this time
    ic_from_conform_ = true;
    AudioInvalidated(range);
    ic_from_conform_ = false;

  } else {

    // A conform task is already running, so we'll just wait for it
    ListenForConformSignal(audio_stream);

    conform_wait_info_.append(info);

  }
}

void RenderBackend::AudioConformUpdated(AudioRenderingParams params)
{
  AudioStream *stream = static_cast<AudioStream*>(sender());

  for (int i=0;i<conform_wait_info_.size();i++) {
    const ConformWaitInfo& info = conform_wait_info_.at(i);

    if (info.stream.get() == stream
        && info.params == params) {

      // Make a copy so the values we use aren't corrupt
      ConformWaitInfo copy = info;

      // Remove this entry from the list
      conform_wait_info_.removeAt(i);
      i--;

      // Send invalidate cache signal
      ic_from_conform_ = true;
      AudioInvalidated(copy.affected_range);
      ic_from_conform_ = false;

    }
  }

  StopListeningForConformSignal(stream);
}

void RenderBackend::AudioInvalidated(const TimeRange& r)
{
  if (!ic_from_conform_) {
    // Cancel any ranges waiting on a conform here since obviously the contents have changed
    for (int i=0;i<conform_wait_info_.size();i++) {
      ConformWaitInfo& info = conform_wait_info_[i];

      // FIXME: Code shamelessly copied from TimeRangeList::RemoveTimeRange()

      if (r.Contains(info.affected_range)) {
        conform_wait_info_.removeAt(i);
        i--;
      } else if (info.affected_range.Contains(r, false, false)) {
        ConformWaitInfo copy = info;

        info.affected_range.set_out(r.in());
        copy.affected_range.set_in(r.out());

        conform_wait_info_.append(copy);
      } else if (info.affected_range.in() < r.in() && info.affected_range.out() > r.in()) {
        info.affected_range.set_out(r.in());
      } else if (info.affected_range.in() < r.out() && info.affected_range.out() > r.out()) {
        info.affected_range.set_in(r.out());
      }
    }
  }

  // Split into 2 second chunks, one for each thread
  QList<TimeRange> split_ranges = SplitRangeIntoChunks(r);

  foreach (const TimeRange& this_range, split_ranges) {

    {
      // Check if this range is already in the queue but hasn't started yet, in which case it'll
      // automatically update to the parameters we have now anyway and we don't need to queue again
      QMutexLocker locker(&queued_audio_lock_);
      if (queued_audio_.ContainsTimeRange(this_range)) {
        continue;
      }
      queued_audio_.InsertTimeRange(this_range);
    }

    // Queue this range
    QFutureWatcher<SampleBufferPtr>* watcher = new QFutureWatcher<SampleBufferPtr>();
    connect(watcher, &QFutureWatcher<SampleBufferPtr>::finished,
            this, &RenderBackend::AudioRendered);

    audio_jobs_.insert(watcher, this_range);

    watcher->setFuture(RenderAudio(this_range, auto_audio_));
  }
}

void RenderBackend::AudioRendered()
{
  QFutureWatcher<SampleBufferPtr>* watcher = static_cast<QFutureWatcher<SampleBufferPtr>*>(sender());

  if (audio_jobs_.contains(watcher)) {
    TimeRange r = audio_jobs_.take(watcher);

    if (watcher->result()) {
      viewer_node_->audio_playback_cache()->WritePCM(r, watcher->result());
    } else {
      viewer_node_->audio_playback_cache()->WriteSilence(r);
    }
  }

  watcher->deleteLater();
}

void RenderBackend::WorkerFinished()
{
  static_cast<RenderWorker*>(sender())->SetAvailable(true);
}

bool RenderBackend::ConformWaitInfo::operator==(const RenderBackend::ConformWaitInfo &rhs) const
{
  return rhs.stream == stream
      && rhs.stream_time == stream_time
      && rhs.affected_range == affected_range;
}

void RenderBackend::RenderPool::Init(ViewerOutput* v)
{
  foreach (RenderWorker* instance, instances) {
    instance->Init(v);
  }
}

void RenderBackend::RenderPool::Queue(NodeInput *input)
{
  foreach (RenderWorker* worker, instances) {
    worker->Queue(input);
  }
}

void RenderBackend::RenderPool::Destroy()
{
  qDeleteAll(instances);
  instances.clear();
}

void RenderBackend::RenderPool::Close()
{
  foreach (RenderWorker* worker, instances) {
    worker->Close();
  }
}

OLIVE_NAMESPACE_EXIT
