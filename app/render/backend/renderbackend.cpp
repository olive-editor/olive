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
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

RenderBackend::RenderBackend(QObject *parent) :
  QObject(parent),
  viewer_node_(nullptr),
  divider_(1),
  render_mode_(RenderMode::kOnline),
  pix_fmt_(PixelFormat::PIX_FMT_RGBA32F),
  sample_fmt_(SampleFormat::SAMPLE_FMT_FLT)
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
    foreach (RenderWorker* instance, instance_pool_) {
      instance->Close();
    }

    disconnect(viewer_node_,
               &ViewerOutput::GraphChangedFrom,
               this,
               &RenderBackend::NodeGraphChanged);

    disconnect(viewer_node_->audio_playback_cache(),
               &AudioPlaybackCache::Invalidated,
               this,
               &RenderBackend::AudioCallback);
  }

  // Set viewer node
  viewer_node_ = viewer_node;

  if (viewer_node_) {
    // Initiate instances with new node
    foreach (RenderWorker* instance, instance_pool_) {
      instance->Init(viewer_node_);
    }

    connect(viewer_node_,
            &ViewerOutput::GraphChangedFrom,
            this,
            &RenderBackend::NodeGraphChanged);

    connect(viewer_node_->audio_playback_cache(),
            &AudioPlaybackCache::Invalidated,
            this,
            &RenderBackend::AudioCallback);
  }
}

void RenderBackend::CancelQueue()
{
  // FIXME: Implement something better than this...
  thread_pool_.waitForDone();
}

QFuture<QByteArray> RenderBackend::Hash(const rational &time)
{
  RenderWorker* instance = GetInstanceFromPool();

  return QtConcurrent::run(&thread_pool_,
                           instance,
                           &RenderWorker::Hash,
                           time);
}

QFuture<FramePtr> RenderBackend::RenderFrame(const rational &time, bool clear_queue)
{
  if (clear_queue) {
    thread_pool_.clear();
  }

  RenderWorker* instance = GetInstanceFromPool();

  return QtConcurrent::run(&thread_pool_,
                           instance,
                           &RenderWorker::RenderFrame,
                           time);
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
  QLinkedList<RenderWorker*>::iterator i;

  for (i=instance_pool_.begin(); i!=instance_pool_.end(); i++) {
    (*i)->Queue(source);
  }
}

void RenderBackend::Close()
{
  CancelQueue();

  qDeleteAll(instance_pool_);
  instance_pool_.clear();
}

VideoRenderingParams RenderBackend::video_params() const
{
  return VideoRenderingParams(viewer_node_->video_params(), pix_fmt_, render_mode_, divider_);
}

AudioRenderingParams RenderBackend::audio_params() const
{
  return AudioRenderingParams(viewer_node_->audio_params(), sample_fmt_);
}

RenderWorker *RenderBackend::GetInstanceFromPool()
{
  RenderWorker* instance = nullptr;

  QLinkedList<RenderWorker*>::iterator i;

  for (i=instance_pool_.begin(); i!=instance_pool_.end(); i++) {
    if ((*i)->IsAvailable()) {
      instance = *i;
      break;
    }
  }

  if (!instance) {
    instance = CreateNewWorker();
    instance_pool_.append(instance);

    if (viewer_node_) {
      instance->Init(viewer_node_);
    }
  }

  instance->SetAvailable(false);
  instance->ProcessQueue();
  instance->SetVideoParams(video_params());
  instance->SetAudioParams(audio_params());
  instance->SetVideoDownloadMatrix(video_download_matrix_);

  return instance;
}

void RenderBackend::AudioCallback()
{
  qDebug() << "STUB";
  /*
  AudioPlaybackCache* pb_cache = viewer_node_->audio_playback_cache();
  QThreadPool* thread_pool = audio_copy_map_.thread_pool();

  while (!pb_cache->IsFullyValidated()
         && thread_pool->activeThreadCount() < thread_pool->maxThreadCount()) {
    // FIXME: Trigger a background audio thread
    TimeRange r = viewer_node_->audio_playback_cache()->GetInvalidatedRanges().first();

    qDebug() << "FIXME: Start rendering audio at:" << r;

    break;
  }
  */
}

bool RenderBackend::ConformWaitInfo::operator==(const RenderBackend::ConformWaitInfo &rhs) const
{
  return rhs.stream == stream
      && rhs.stream_time == stream_time
      && rhs.affected_range == affected_range;
}

OLIVE_NAMESPACE_EXIT
