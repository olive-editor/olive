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

#include "rendermanager.h"

#include <QApplication>
#include <QMatrix4x4>
#include <QThread>

#include "config/config.h"
#include "core.h"
#include "render/opengl/openglrenderer.h"
#include "renderprocessor.h"
#include "task/conform/conform.h"
#include "task/taskmanager.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

RenderManager* RenderManager::instance_ = nullptr;
const rational RenderManager::kDryRunInterval = rational(10);

RenderManager::RenderManager(QObject *parent) :
  backend_(kOpenGL),
  aggressive_gc_(0)
{
  if (backend_ == kOpenGL) {
    context_ = new OpenGLRenderer();
    decoder_cache_ = new DecoderCache();
    shader_cache_ = new ShaderCache();
  } else {
    qCritical() << "Tried to initialize unknown graphics backend";
    context_ = nullptr;
    decoder_cache_ = nullptr;
  }

  if (context_) {
    video_thread_ = new RenderThread(context_, decoder_cache_, shader_cache_, this);
    dry_run_thread_ = new RenderThread(nullptr, decoder_cache_, shader_cache_, this);
    audio_thread_ = new RenderThread(nullptr, decoder_cache_, shader_cache_, this);

    video_thread_->start(QThread::IdlePriority);
    dry_run_thread_->start(QThread::IdlePriority);
    audio_thread_->start(QThread::IdlePriority);
  }

  decoder_clear_timer_ = new QTimer(this);
  decoder_clear_timer_->setInterval(kDecoderMaximumInactivity);
  connect(decoder_clear_timer_, &QTimer::timeout, this, &RenderManager::ClearOldDecoders);
  decoder_clear_timer_->start();
}

RenderManager::~RenderManager()
{
  if (context_) {
    delete shader_cache_;
    delete decoder_cache_;

    video_thread_->quit();
    video_thread_->wait();

    dry_run_thread_->quit();
    dry_run_thread_->wait();

    context_->PostDestroy();
    delete context_;

    audio_thread_->quit();
    audio_thread_->wait();
  }
}

RenderTicketPtr RenderManager::RenderFrame(Node *node, const VideoParams &vparam, const AudioParams &param,
                                           ColorManager* color_manager, const rational& time, RenderMode::Mode mode,
                                           FrameHashCache* cache, ReturnType return_type)
{
  return RenderFrame(node,
                     color_manager,
                     time,
                     mode,
                     vparam,
                     param,
                     QSize(0, 0),
                     QMatrix4x4(),
                     VideoParams::kFormatInvalid,
                     0,
                     nullptr,
                     cache,
                     return_type);
}

RenderTicketPtr RenderManager::RenderFrame(Node *node, ColorManager* color_manager,
                                           const rational& time, RenderMode::Mode mode,
                                           const VideoParams &video_params, const AudioParams &audio_params,
                                           const QSize& force_size,
                                           const QMatrix4x4& force_matrix, VideoParams::Format force_format,
                                           int force_channel_count,
                                           ColorProcessorPtr force_color_output,
                                           FrameHashCache* cache, ReturnType return_type)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("node", Node::PtrToValue(node));
  ticket->setProperty("time", QVariant::fromValue(time));
  ticket->setProperty("size", force_size);
  ticket->setProperty("matrix", force_matrix);
  ticket->setProperty("format", force_format);
  ticket->setProperty("channelcount", force_channel_count);
  ticket->setProperty("mode", mode);
  ticket->setProperty("type", kTypeVideo);
  ticket->setProperty("colormanager", Node::PtrToValue(color_manager));
  ticket->setProperty("coloroutput", QVariant::fromValue(force_color_output));
  ticket->setProperty("vparam", QVariant::fromValue(video_params));
  ticket->setProperty("aparam", QVariant::fromValue(audio_params));
  ticket->setProperty("return", return_type);

  if (cache) {
    ticket->setProperty("cache", cache->GetCacheDirectory());
    ticket->setProperty("cachetimebase", QVariant::fromValue(cache->GetTimebase()));
    ticket->setProperty("cacheuuid", QVariant::fromValue(cache->GetUuid()));
  }

  if (return_type == ReturnType::kNull) {
    dry_run_thread_->AddTicket(ticket);
  } else {
    video_thread_->AddTicket(ticket);
  }

  return ticket;
}

RenderTicketPtr RenderManager::RenderAudio(Node *node, const TimeRange &r, const AudioParams &params, RenderMode::Mode mode, bool generate_waveforms)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("node", Node::PtrToValue(node));
  ticket->setProperty("time", QVariant::fromValue(r));
  ticket->setProperty("type", kTypeAudio);
  ticket->setProperty("mode", mode);
  ticket->setProperty("enablewaveforms", generate_waveforms);
  ticket->setProperty("aparam", QVariant::fromValue(params));

  audio_thread_->AddTicket(ticket);

  return ticket;
}

bool RenderManager::RemoveTicket(RenderTicketPtr ticket)
{
  if (video_thread_->RemoveTicket(ticket)) {
    return true;
  } else if (audio_thread_->RemoveTicket(ticket)) {
    return true;
  } else if (dry_run_thread_->RemoveTicket(ticket)) {
    return true;
  } else {
    return false;
  }
}

void RenderManager::SetAggressiveGarbageCollection(bool enabled)
{
  aggressive_gc_ += enabled ? 1 : -1;

  if (aggressive_gc_ > 0) {
    decoder_clear_timer_->setInterval(kDecoderMaximumInactivityAggressive);
  } else {
    decoder_clear_timer_->setInterval(kDecoderMaximumInactivity);
  }
}

void RenderManager::ClearOldDecoders()
{
  QMutexLocker locker(decoder_cache_->mutex());

  qint64 min_age = QDateTime::currentMSecsSinceEpoch() - kDecoderMaximumInactivity;

  for (auto it=decoder_cache_->begin(); it!=decoder_cache_->end(); ) {
    DecoderPair decoder = it.value();

    if (decoder.decoder->GetLastAccessedTime() < min_age) {
      decoder.decoder->Close();
      it = decoder_cache_->erase(it);
    } else {
      it++;
    }
  }
}

RenderThread::RenderThread(Renderer *renderer, DecoderCache *decoder_cache, ShaderCache *shader_cache, QObject *parent) :
  QThread(parent),
  cancelled_(false),
  context_(renderer),
  decoder_cache_(decoder_cache),
  shader_cache_(shader_cache)
{
  if (context_) {
    context_->Init();
    context_->moveToThread(this);
  }
}

void RenderThread::AddTicket(RenderTicketPtr ticket)
{
  QMutexLocker locker(&mutex_);
  queue_.push_back(ticket);
  wait_.wakeOne();
}

bool RenderThread::RemoveTicket(RenderTicketPtr ticket)
{
  QMutexLocker locker(&mutex_);

  auto it = std::find(queue_.begin(), queue_.end(), ticket);
  if (it == queue_.end()) {
    return false;
  }

  queue_.erase(it);
  return true;
}

void RenderThread::quit()
{
  QMutexLocker locker(&mutex_);
  cancelled_ = true;
  wait_.wakeOne();
}

void RenderThread::run()
{
  if (context_) {
    context_->PostInit();
  }

  QMutexLocker locker(&mutex_);

  while (!cancelled_) {
    if (queue_.empty()) {
      wait_.wait(&mutex_);
    }

    if (cancelled_) {
      break;
    }

    if (!queue_.empty()) {
      RenderTicketPtr ticket = queue_.front();
      queue_.pop_front();

      locker.unlock();

      // Setup the ticket for ::Process
      ticket->Start();

      if (ticket->IsCancelled()) {
        ticket->Finish();
      } else {
        RenderProcessor::Process(ticket, context_, decoder_cache_, shader_cache_);
      }

      locker.relock();
    }
  }

  if (context_) {
    context_->Destroy();
    context_->moveToThread(this->thread());
  }
}

}
