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

#include "rendererthreadwrapper.h"

OLIVE_NAMESPACE_ENTER

/*RendererThreadWrapper::RendererThreadWrapper(Renderer *inner, QObject *parent) :
  Renderer(parent),
  inner_(inner),
  thread_(nullptr)
{
  inner_->setParent(this);
}

bool RendererThreadWrapper::Init()
{
  // Init context in main thread
  if (!inner_->Init()) {
    return false;
  }

  // Create thread
  QThread* thread = new QThread(this);
  thread->start(QThread::IdlePriority);

  // Move context to thread
  inner_->moveToThread(thread);

  // Queue post-init in new thread
  QMetaObject::invokeMethod(inner_, "PostInit", Qt::BlockingQueuedConnection);

  return true;
}

void RendererThreadWrapper::Destroy()
{
  if (thread_) {
    QMetaObject::invokeMethod(inner_, "Destroy", Qt::BlockingQueuedConnection);

    thread_->quit();
    thread_->wait();
    delete thread_;
    thread_ = nullptr;
  }
}

QVariant RendererThreadWrapper::CreateTexture(const VideoParams &param, void *data, int linesize)
{
  QVariant v;

  QMetaObject::invokeMethod(inner_, "CreateTexture", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, v),
                            OLIVE_NS_CONST_ARG(VideoParams&, param),
                            Q_ARG(void*, data),
                            Q_ARG(int, linesize));

  return v;
}

void RendererThreadWrapper::DestroyTexture(QVariant texture)
{
  QMetaObject::invokeMethod(inner_, "DestroyTexture", Qt::BlockingQueuedConnection,
                            Q_ARG(QVariant, texture));
}

void RendererThreadWrapper::UploadToTexture(QVariant texture, void *data, int linesize)
{
  QMetaObject::invokeMethod(inner_, "UploadToTexture", Qt::BlockingQueuedConnection,
                            Q_ARG(QVariant, texture),
                            Q_ARG(void*, data),
                            Q_ARG(int, linesize));
}

void RendererThreadWrapper::DownloadFromTexture(QVariant texture, void *data, int linesize)
{
  QMetaObject::invokeMethod(inner_, "DownloadFromTexture", Qt::BlockingQueuedConnection,
                            Q_ARG(QVariant, texture),
                            Q_ARG(void*, data),
                            Q_ARG(int, linesize));
}

QVariant RendererThreadWrapper::ProcessShader(const Node *node, const TimeRange &range, const ShaderJob &job, const VideoParams &params)
{
  QVariant v;

  QMetaObject::invokeMethod(inner_, "ProcessShader", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, v),
                            OLIVE_NS_CONST_ARG(Node*, node),
                            OLIVE_NS_CONST_ARG(TimeRange&, range),
                            OLIVE_NS_CONST_ARG(ShaderJob&, job),
                            OLIVE_NS_CONST_ARG(VideoParams&, params));

  return v;
}

QVariant RendererThreadWrapper::TransformColor(QVariant texture, ColorProcessorPtr processor)
{
  QVariant v;

  QMetaObject::invokeMethod(inner_, "ProcessShader", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, v),
                            Q_ARG(QVariant, texture),
                            OLIVE_NS_ARG(ColorProcessorPtr, processor));

  return v;
}

VideoParams RendererThreadWrapper::GetParamsFromTexture(QVariant texture)
{
  VideoParams p;

  QMetaObject::invokeMethod(inner_, "GetParamsFromTexture", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(VideoParams, p),
                            Q_ARG(QVariant, texture));

  return p;
}*/

OLIVE_NAMESPACE_EXIT
