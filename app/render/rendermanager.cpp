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

#include "rendermanager.h"

#include <QApplication>
#include <QMatrix4x4>
#include <QThread>

#include "config/config.h"
#include "core.h"
#include "render/opengl/openglrenderer.h"
#include "render/rendererthreadwrapper.h"
#include "renderprocessor.h"
#include "task/conform/conform.h"
#include "task/taskmanager.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

RenderManager* RenderManager::instance_ = nullptr;
const int RenderManager::kDecoderMaximumInactivity = 10000;

RenderManager::RenderManager(QObject *parent) :
  ThreadPool(QThread::IdlePriority, 0, parent),
  backend_(kOpenGL)
{
  Renderer* graphics_renderer = nullptr;

  if (backend_ == kOpenGL) {
    graphics_renderer = new OpenGLRenderer();
  }

  if (graphics_renderer) {
    context_ = new RendererThreadWrapper(graphics_renderer, this);
    context_->Init();
    context_->PostInit();

    still_cache_ = new StillImageCache();
    decoder_cache_ = new DecoderCache();
    shader_cache_ = new ShaderCache();
    default_shader_ = context_->CreateNativeShader(ShaderCode(QString(), QString()));

    decoder_clear_timer_.setInterval(kDecoderMaximumInactivity);
    connect(&decoder_clear_timer_, &QTimer::timeout, this, &RenderManager::ClearOldDecoders);
    decoder_clear_timer_.start();
  } else {
    qCritical() << "Tried to initialize unknown graphics backend";
    context_ = nullptr;
    still_cache_ = nullptr;
    decoder_cache_ = nullptr;
  }
}

RenderManager::~RenderManager()
{
  if (context_) {
    context_->DestroyNativeShader(default_shader_);

    delete shader_cache_;
    delete decoder_cache_;
    delete still_cache_;

    context_->Destroy();
    context_->PostDestroy();
    delete context_;
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

QByteArray RenderManager::Hash(const Node *n, const QString& output, const VideoParams &params, const rational &time)
{
  QCryptographicHash hasher(QCryptographicHash::Sha1);

  // Embed video parameters into this hash
  int width = params.effective_width();
  int height = params.effective_height();
  VideoParams::Format format = params.format();
  VideoParams::Interlacing interlacing = params.interlacing();

  hasher.addData(reinterpret_cast<const char*>(&width), sizeof(width));
  hasher.addData(reinterpret_cast<const char*>(&height), sizeof(height));
  hasher.addData(reinterpret_cast<const char*>(&format), sizeof(format));
  hasher.addData(reinterpret_cast<const char*>(&interlacing), sizeof(interlacing));

  if (n) {
    n->Hash(output, hasher, time, params);
  }

  return hasher.result();
}

RenderTicketPtr RenderManager::RenderFrame(ViewerOutput *viewer, ColorManager* color_manager,
                                           const rational& time, RenderMode::Mode mode,
                                           FrameHashCache* cache, bool prioritize, bool texture_only)
{
  return RenderFrame(viewer,
                     color_manager,
                     time,
                     mode,
                     viewer->GetVideoParams(),
                     viewer->GetAudioParams(),
                     QSize(0, 0),
                     QMatrix4x4(),
                     VideoParams::kFormatInvalid,
                     nullptr,
                     cache,
                     prioritize,
                     texture_only);
}

RenderTicketPtr RenderManager::RenderFrame(ViewerOutput *viewer, ColorManager* color_manager,
                                           const rational& time, RenderMode::Mode mode,
                                           const VideoParams &video_params, const AudioParams &audio_params,
                                           const QSize& force_size,
                                           const QMatrix4x4& force_matrix, VideoParams::Format force_format,
                                           ColorProcessorPtr force_color_output,
                                           FrameHashCache* cache, bool prioritize, bool texture_only)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("viewer", Node::PtrToValue(viewer));
  ticket->setProperty("time", QVariant::fromValue(time));
  ticket->setProperty("size", force_size);
  ticket->setProperty("matrix", force_matrix);
  ticket->setProperty("format", force_format);
  ticket->setProperty("mode", mode);
  ticket->setProperty("type", kTypeVideo);
  ticket->setProperty("colormanager", Node::PtrToValue(color_manager));
  ticket->setProperty("coloroutput", QVariant::fromValue(force_color_output));
  ticket->setProperty("vparam", QVariant::fromValue(video_params));
  ticket->setProperty("aparam", QVariant::fromValue(audio_params));
  ticket->setProperty("textureonly", texture_only);

  if (cache) {
    ticket->setProperty("cache", cache->GetCacheDirectory());
  }

  if (ticket->thread() != this->thread()) {
    ticket->moveToThread(this->thread());
  }

  // Queue appending the ticket and running the next job on our thread to make this function thread-safe
  QMetaObject::invokeMethod(this, "AddTicket", Qt::AutoConnection,
                            OLIVE_NS_ARG(RenderTicketPtr, ticket),
                            Q_ARG(bool, prioritize));

  return ticket;
}

RenderTicketPtr RenderManager::RenderAudio(ViewerOutput* viewer, const TimeRange& r, RenderMode::Mode mode, bool generate_waveforms, bool prioritize)
{
  return RenderAudio(viewer, r, viewer->GetAudioParams(), mode, generate_waveforms, prioritize);
}

RenderTicketPtr RenderManager::RenderAudio(ViewerOutput* viewer, const TimeRange &r, const AudioParams &params, RenderMode::Mode mode, bool generate_waveforms, bool prioritize)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("viewer", Node::PtrToValue(viewer));
  ticket->setProperty("time", QVariant::fromValue(r));
  ticket->setProperty("type", kTypeAudio);
  ticket->setProperty("mode", mode);
  ticket->setProperty("enablewaveforms", generate_waveforms);
  ticket->setProperty("aparam", QVariant::fromValue(params));

  if (ticket->thread() != this->thread()) {
    ticket->moveToThread(this->thread());
  }

  // Queue appending the ticket and running the next job on our thread to make this function thread-safe
  QMetaObject::invokeMethod(this, "AddTicket", Qt::AutoConnection,
                            OLIVE_NS_ARG(RenderTicketPtr, ticket),
                            Q_ARG(bool, prioritize));

  return ticket;
}

RenderTicketPtr RenderManager::SaveFrameToCache(FrameHashCache *cache, FramePtr frame, const QByteArray &hash, bool prioritize)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("cache", cache->GetCacheDirectory());
  ticket->setProperty("frame", QVariant::fromValue(frame));
  ticket->setProperty("hash", hash);
  ticket->setProperty("type", kTypeVideoDownload);

  if (ticket->thread() != this->thread()) {
    ticket->moveToThread(this->thread());
  }

  // Queue appending the ticket and running the next job on our thread to make this function thread-safe
  QMetaObject::invokeMethod(this, "AddTicket", Qt::AutoConnection,
                            OLIVE_NS_ARG(RenderTicketPtr, ticket),
                            Q_ARG(bool, prioritize));

  return ticket;
}

void RenderManager::RunTicket(RenderTicketPtr ticket) const
{
  RenderProcessor::Process(ticket, context_, still_cache_, decoder_cache_, shader_cache_, default_shader_);
}

}
