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
#include "node/hashtraverser.h"
#include "render/opengl/openglrenderer.h"
#include "render/rendererthreadwrapper.h"
#include "renderprocessor.h"
#include "task/conform/conform.h"
#include "task/taskmanager.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

RenderManager* RenderManager::instance_ = nullptr;

RenderManager::RenderManager(QObject *parent) :
  ThreadPool(0, parent),
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

    decoder_cache_ = new DecoderCache();
    shader_cache_ = new ShaderCache();
    default_shader_ = context_->CreateNativeShader(ShaderCode(QString(), QString()));
  } else {
    qCritical() << "Tried to initialize unknown graphics backend";
    context_ = nullptr;
    decoder_cache_ = nullptr;
  }
}

RenderManager::~RenderManager()
{
  if (context_) {
    context_->DestroyNativeShader(default_shader_);

    delete shader_cache_;
    delete decoder_cache_;

    context_->Destroy();
    context_->PostDestroy();
    delete context_;
  }
}

QByteArray RenderManager::Hash(const Node *n, const Node::ValueHint &output, const VideoParams &params, const rational &time)
{
  Q_ASSERT(n);

  if (n) {
    HashTraverser hasher;
    return hasher.GetHash(n, output, params, TimeRange(time, time + params.frame_rate_as_time_base()));
  } else {
    qCritical() << "Hash called with null node";
    return QByteArray();
  }
}

RenderTicketPtr RenderManager::RenderFrame(ViewerOutput *viewer, ColorManager* color_manager,
                                           const rational& time, RenderMode::Mode mode,
                                           FrameHashCache* cache, RenderTicketPriority priority, bool texture_only)
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
                     priority,
                     texture_only);
}

RenderTicketPtr RenderManager::RenderFrame(ViewerOutput *viewer, ColorManager* color_manager,
                                           const rational& time, RenderMode::Mode mode,
                                           const VideoParams &video_params, const AudioParams &audio_params,
                                           const QSize& force_size,
                                           const QMatrix4x4& force_matrix, VideoParams::Format force_format,
                                           ColorProcessorPtr force_color_output,
                                           FrameHashCache* cache, RenderTicketPriority priority, bool texture_only)
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

  AddTicket(ticket, priority);

  return ticket;
}

RenderTicketPtr RenderManager::RenderAudio(ViewerOutput* viewer, const TimeRange& r, RenderMode::Mode mode, bool generate_waveforms, RenderTicketPriority priority)
{
  return RenderAudio(viewer, r, viewer->GetAudioParams(), mode, generate_waveforms, priority);
}

RenderTicketPtr RenderManager::RenderAudio(ViewerOutput* viewer, const TimeRange &r, const AudioParams &params, RenderMode::Mode mode, bool generate_waveforms, RenderTicketPriority priority)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("viewer", Node::PtrToValue(viewer));
  ticket->setProperty("time", QVariant::fromValue(r));
  ticket->setProperty("type", kTypeAudio);
  ticket->setProperty("mode", mode);
  ticket->setProperty("enablewaveforms", generate_waveforms);
  ticket->setProperty("aparam", QVariant::fromValue(params));

  AddTicket(ticket, priority);

  return ticket;
}

RenderTicketPtr RenderManager::SaveFrameToCache(FrameHashCache *cache, FramePtr frame, const QByteArray &hash, RenderTicketPriority priority)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("cache", cache->GetCacheDirectory());
  ticket->setProperty("frame", QVariant::fromValue(frame));
  ticket->setProperty("hash", hash);
  ticket->setProperty("type", kTypeVideoDownload);

  AddTicket(ticket, priority);

  return ticket;
}

void RenderManager::RunTicket(RenderTicketPtr ticket) const
{
  // Setup the ticket for ::Process
  ticket->Start();

  if (ticket->IsCancelled()) {
    ticket->Finish();
    return;
  }

  RenderProcessor::Process(ticket, context_, decoder_cache_, shader_cache_, default_shader_);
}

}
