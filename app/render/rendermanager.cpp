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
  } else {
    qCritical() << "Tried to initialize unknown graphics backend";
    context_ = nullptr;
    decoder_cache_ = nullptr;
  }
}

RenderManager::~RenderManager()
{
  if (context_) {
    delete shader_cache_;
    delete decoder_cache_;

    context_->Destroy();
    context_->PostDestroy();
    delete context_;
  }
}

RenderTicketPtr RenderManager::RenderFrame(const RenderVideoParams &params)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("node", Node::PtrToValue(params.node));
  ticket->setProperty("time", QVariant::fromValue(params.time));
  ticket->setProperty("size", params.force_size);
  ticket->setProperty("matrix", params.force_matrix);
  ticket->setProperty("format", params.force_format);
  ticket->setProperty("usecache", params.use_cache);
  ticket->setProperty("type", kTypeVideo);
  ticket->setProperty("colormanager", Node::PtrToValue(params.color_manager));
  ticket->setProperty("coloroutput", QVariant::fromValue(params.force_color_output));
  Q_ASSERT(params.video_params.is_valid());
  ticket->setProperty("vparam", QVariant::fromValue(params.video_params));
  ticket->setProperty("aparam", QVariant::fromValue(params.audio_params));
  ticket->setProperty("return", params.return_type);
  ticket->setProperty("cache", params.cache_dir);
  ticket->setProperty("cachetimebase", QVariant::fromValue(params.cache_timebase));
  ticket->setProperty("cacheid", QVariant::fromValue(params.cache_id));

  AddTicket(ticket, params.priority);

  return ticket;
}

RenderTicketPtr RenderManager::RenderAudio(const RenderAudioParams &params)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("node", Node::PtrToValue(params.node));
  ticket->setProperty("time", QVariant::fromValue(params.range));
  ticket->setProperty("type", kTypeAudio);
  ticket->setProperty("enablewaveforms", params.generate_waveforms);
  ticket->setProperty("clamp", params.clamp);
  ticket->setProperty("aparam", QVariant::fromValue(params.audio_params));

  AddTicket(ticket, params.priority);

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

  RenderProcessor::Process(ticket, context_, decoder_cache_, shader_cache_);
}

}
