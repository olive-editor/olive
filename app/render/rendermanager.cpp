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

#include "rendermanager.h"

#include <QApplication>
#include <QDateTime>
#include <QMatrix4x4>
#include <QThread>

#include "config/config.h"
#include "core.h"
#include "render/backend/opengl/openglcontext.h"
#include "render/backend/rendercontextthreadwrapper.h"
#include "renderprocessor.h"
#include "task/conform/conform.h"
#include "task/taskmanager.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

RenderManager* RenderManager::instance_ = nullptr;

RenderManager::RenderManager(QObject *parent) :
  ThreadPool(QThread::IdlePriority, 0, parent)
{
  context_ = new RenderContextThreadWrapper(new OpenGLContext(), this);
  context_->Init();
}

QByteArray RenderManager::Hash(const Node *n, const VideoParams &params, const rational &time)
{
  QCryptographicHash hasher(QCryptographicHash::Sha1);

  // Embed video parameters into this hash
  hasher.addData(reinterpret_cast<const char*>(&params.effective_width()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&params.effective_height()), sizeof(int));
  hasher.addData(reinterpret_cast<const char*>(&params.format()), sizeof(PixelFormat::Format));

  if (n) {
    n->Hash(hasher, time);
  }

  return hasher.result();
}

RenderTicketPtr RenderManager::RenderFrame(ViewerOutput *viewer, const rational &time, RenderMode::Mode mode, bool prioritize)
{
  return RenderFrame(viewer, time, mode,
                     QSize(),
                     QMatrix4x4(),
                     prioritize);
}

RenderTicketPtr RenderManager::RenderFrame(ViewerOutput* viewer, const rational &time, RenderMode::Mode mode, const QSize &force_size, const QMatrix4x4 &matrix, bool prioritize)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("viewer", Node::PtrToValue(viewer));
  ticket->setProperty("time", QVariant::fromValue(time));
  ticket->setProperty("size", force_size);
  ticket->setProperty("matrix", matrix);
  ticket->setProperty("mode", mode);
  ticket->setProperty("type", kTypeVideo);

  // Queue appending the ticket and running the next job on our thread to make this function thread-safe
  QMetaObject::invokeMethod(this, "AddTicket", Qt::AutoConnection,
                            OLIVE_NS_ARG(RenderTicketPtr, ticket),
                            Q_ARG(bool, prioritize));

  return ticket;
}

RenderTicketPtr RenderManager::RenderAudio(ViewerOutput* viewer, const TimeRange &r, bool generate_waveforms, bool prioritize)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("viewer", Node::PtrToValue(viewer));
  ticket->setProperty("time", QVariant::fromValue(r));
  ticket->setProperty("type", kTypeAudio);
  ticket->setProperty("waveforms", generate_waveforms);

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

  ticket->setProperty("cache", Node::PtrToValue(cache));
  ticket->setProperty("frame", QVariant::fromValue(frame));
  ticket->setProperty("hash", hash);
  ticket->setProperty("type", kTypeVideoDownload);

  // Queue appending the ticket and running the next job on our thread to make this function thread-safe
  QMetaObject::invokeMethod(this, "AddTicket", Qt::AutoConnection,
                            OLIVE_NS_ARG(RenderTicketPtr, ticket),
                            Q_ARG(bool, prioritize));

  return ticket;
}

void RenderManager::RunTicket(RenderTicketPtr ticket) const
{
  RenderProcessor::Process(ticket, context_);
}

OLIVE_NAMESPACE_EXIT
