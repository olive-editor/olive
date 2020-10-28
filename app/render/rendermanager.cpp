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
#include <QThread>

#include "config/config.h"
#include "core.h"
#include "render/backend/opengl/openglproxy.h"
#include "task/conform/conform.h"
#include "task/taskmanager.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

RenderManager* RenderManager::instance_ = nullptr;

RenderManager::RenderManager(QObject *parent) :
  ThreadPool(QThread::IdlePriority, 0, parent)
{
  // Initialize OpenGL service
  OpenGLProxy::CreateInstance();
}

RenderManager::~RenderManager()
{
  OpenGLProxy::DestroyInstance();
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

RenderTicketPtr RenderManager::RenderFrame(ViewerOutput* viewer, const rational &time, RenderMode::Mode mode, bool prioritize)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("viewer", Node::PtrToValue(viewer));
  ticket->setProperty("time", QVariant::fromValue(time));
  ticket->setProperty("mode", mode);
  ticket->setProperty("type", kTypeVideo);

  // Queue appending the ticket and running the next job on our thread to make this function thread-safe
  QMetaObject::invokeMethod(this, "AddTicket", Qt::AutoConnection,
                            OLIVE_NS_ARG(RenderTicketPtr, ticket),
                            Q_ARG(bool, prioritize));

  return ticket;
}

RenderTicketPtr RenderManager::RenderAudio(ViewerOutput* viewer, const TimeRange &r, bool prioritize)
{
  // Create ticket
  RenderTicketPtr ticket = std::make_shared<RenderTicket>();

  ticket->setProperty("viewer", Node::PtrToValue(viewer));
  ticket->setProperty("time", QVariant::fromValue(r));
  ticket->setProperty("type", kTypeAudio);

  // Queue appending the ticket and running the next job on our thread to make this function thread-safe
  QMetaObject::invokeMethod(this, "AddTicket", Qt::AutoConnection,
                            OLIVE_NS_ARG(RenderTicketPtr, ticket),
                            Q_ARG(bool, prioritize));

  return ticket;
}

void RenderManager::RunTicket(RenderTicketPtr ticket) const
{
  // Depending on the render ticket type, start a job
  TicketType type = ticket->property("type").value<TicketType>();

  switch (type) {
  case kTypeVideo:
    RenderFrameInternal(ticket);
    break;
  case kTypeAudio:
    RenderAudioInternal(ticket);
    break;
  default:
    // Fail
    ticket->Cancel();
  }
}

void RenderManager::RenderFrameInternal(RenderTicketPtr ticket)
{
  ViewerOutput* viewer = Node::ValueToPtr<ViewerOutput>(ticket->property("viewer"));
  rational time = ticket->property("time").value<rational>();

  ticket->Start();

  qDebug() << "STUB: Rendered" << time << "frames for" << viewer;

  FramePtr frame = Frame::Create();
  frame->set_video_params(viewer->video_params());
  frame->allocate();

  ticket->Finish(QVariant::fromValue(frame), false);
}

void RenderManager::RenderAudioInternal(RenderTicketPtr ticket)
{
  ViewerOutput* viewer = Node::ValueToPtr<ViewerOutput>(ticket->property("viewer"));
  TimeRange time = ticket->property("time").value<TimeRange>();

  ticket->Start();

  qDebug() << "STUB: Rendered" << time << "audio for" << viewer;

  ticket->Finish(QVariant::fromValue(SampleBuffer::CreateAllocated(viewer->audio_params(), time.length())), false);
}

void RenderManager::WorkerGeneratedWaveform(RenderTicketPtr ticket, TrackOutput *track, AudioVisualWaveform samples, TimeRange range)
{
  ViewerOutput* viewer = Node::ValueToPtr<ViewerOutput>(ticket->property("viewer"));

  QList<TimeRange> valid_ranges = viewer->audio_playback_cache()->GetValidRanges(range,
                                                                                              ticket->GetJobTime());
  if (!valid_ranges.isEmpty()) {
    // Generate visual waveform in this background thread
    track->waveform_lock()->lock();

    track->waveform().set_channel_count(viewer->audio_params().channel_count());

    foreach (const TimeRange& r, valid_ranges) {
      track->waveform().OverwriteSums(samples, r.in(), r.in() - range.in(), r.length());
    }

    track->waveform_lock()->unlock();

    emit track->PreviewChanged();
  }
}

OLIVE_NAMESPACE_EXIT
