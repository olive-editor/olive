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
  video_instance_queuer_(0),
  audio_instance_queuer_(0),
  divider_(1),
  render_mode_(RenderMode::kOnline),
  pix_fmt_(PixelFormat::PIX_FMT_RGBA32F),
  sample_fmt_(SampleFormat::SAMPLE_FMT_FLT),
  ic_from_conform_(false)
{
  // FIXME: Don't create in CLI mode
  cancel_dialog_ = new RenderCancelDialog(Core::instance()->main_window());
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
    foreach (RenderWorker* instance, video_instance_pool_) {
      instance->Close();
    }

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
    foreach (RenderWorker* instance, video_instance_pool_) {
      instance->Init(viewer_node_);
    }

    connect(viewer_node_,
            &ViewerOutput::GraphChangedFrom,
            this,
            &RenderBackend::NodeGraphChanged);

    connect(viewer_node_->audio_playback_cache(),
            &AudioPlaybackCache::Invalidated,
            this,
            &RenderBackend::AudioInvalidated);
  }
}

void RenderBackend::CancelQueue()
{
  // FIXME: Implement something better than this...
  video_thread_pool_.waitForDone();
}

QFuture<QByteArray> RenderBackend::Hash(const rational &time, bool block_for_update)
{
  return QtConcurrent::run(&video_thread_pool_,
                           GetInstanceFromPool(video_instance_pool_, video_thread_pool_, video_instance_queuer_),
                           &RenderWorker::Hash,
                           time,
                           block_for_update);
}

QFuture<FramePtr> RenderBackend::RenderFrame(const rational &time, bool clear_queue, bool block_for_update)
{
  if (clear_queue) {
    video_thread_pool_.clear();
  }

  return QtConcurrent::run(&video_thread_pool_,
                           GetInstanceFromPool(video_instance_pool_, video_thread_pool_, video_instance_queuer_),
                           &RenderWorker::RenderFrame,
                           time,
                           block_for_update);
}

void RenderBackend::SetDivider(const int &divider)
{
  divider_ = divider;
}

void RenderBackend::SetMode(const RenderMode::Mode &mode)
{
  render_mode_ = mode;
}

void RenderBackend::SetPixelFormat(const PixelFormat::Format &pix_fmt)
{
  pix_fmt_ = pix_fmt;
}

void RenderBackend::SetSampleFormat(const SampleFormat::Format &sample_fmt)
{
  sample_fmt_ = sample_fmt;
}

void RenderBackend::SetVideoDownloadMatrix(const QMatrix4x4 &mat)
{
  video_download_matrix_ = mat;
}

void RenderBackend::NodeGraphChanged(NodeInput *source)
{
  foreach (RenderWorker* worker, video_instance_pool_) {
    worker->Queue(source);
  }
}

void RenderBackend::UpdateInstance(RenderWorker *instance)
{
  instance->SetAvailable(false);
  instance->ProcessQueue();
  instance->SetVideoParams(video_params());
  instance->SetAudioParams(audio_params());
  instance->SetVideoDownloadMatrix(video_download_matrix_);
}

void RenderBackend::Close()
{
  CancelQueue();

  qDeleteAll(video_instance_pool_);
  video_instance_pool_.clear();
}

VideoRenderingParams RenderBackend::video_params() const
{
  return VideoRenderingParams(viewer_node_->video_params(), pix_fmt_, render_mode_, divider_);
}

AudioRenderingParams RenderBackend::audio_params() const
{
  return AudioRenderingParams(viewer_node_->audio_params(), sample_fmt_);
}

RenderWorker *RenderBackend::GetInstanceFromPool(QVector<RenderWorker *> &worker_pool, QThreadPool &thread_pool, int &instance_queuer)
{
  RenderWorker* instance = nullptr;

  foreach (RenderWorker* worker, worker_pool) {
    if (worker->IsAvailable()) {
      instance = worker;
      break;
    }
  }

  if (!instance) {
    if (worker_pool.size() < thread_pool.maxThreadCount()) {
      // Can create another instance
      instance = CreateNewWorker();
      worker_pool.append(instance);

      if (viewer_node_) {
        instance->Init(viewer_node_);
      }

      connect(instance, &RenderWorker::FinishedJob,
              this, &RenderBackend::WorkerFinished, Qt::QueuedConnection);

      connect(instance, &RenderWorker::AudioConformUnavailable,
              this, &RenderBackend::AudioConformUnavailable, Qt::QueuedConnection);
    } else {
      instance = worker_pool.at(instance_queuer % worker_pool.size());
      instance_queuer++;
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

  QFutureWatcher<SampleBufferPtr>* watcher = new QFutureWatcher<SampleBufferPtr>();
  connect(watcher, &QFutureWatcher<SampleBufferPtr>::finished, this, &RenderBackend::AudioRendered);

  audio_jobs_.insert(watcher, r);

  watcher->setFuture(QtConcurrent::run(&video_thread_pool_,
                                       GetInstanceFromPool(audio_instance_pool_, audio_thread_pool_, audio_instance_queuer_),
                                       &RenderWorker::RenderAudio,
                                       r));
}

void RenderBackend::AudioRendered()
{
  QFutureWatcher<SampleBufferPtr>* watcher = static_cast<QFutureWatcher<SampleBufferPtr>*>(sender());

  if (audio_jobs_.contains(watcher)) {
    TimeRange r = audio_jobs_.take(watcher);

    if (watcher->result()) {
      qDebug() << "Received" << watcher->result()->sample_count_per_channel() << "samples";
      viewer_node_->audio_playback_cache()->WritePCM(r, watcher->result());
    } else {
      qDebug() << "Received null";
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

OLIVE_NAMESPACE_EXIT
