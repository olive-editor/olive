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

RendererThreadWrapper::RendererThreadWrapper(Renderer *inner, QObject *parent) :
  Renderer(parent),
  inner_(inner),
  thread_(nullptr)
{
}

bool RendererThreadWrapper::Init()
{
  // Init context in main thread
  if (!inner_->Init()) {
    return false;
  }

  // Create thread
  thread_ = new QThread(this);
  thread_->start(QThread::IdlePriority);

  // Move context to thread
  inner_->moveToThread(thread_);

  // Queue post-init in new thread
  QMetaObject::invokeMethod(inner_, "PostInit", Qt::BlockingQueuedConnection);

  return true;
}

void RendererThreadWrapper::PostInit()
{
  // Do nothing
}

void RendererThreadWrapper::Destroy()
{
  if (thread_) {
    QMetaObject::invokeMethod(inner_, "Destroy", Qt::BlockingQueuedConnection);
    inner_ = nullptr;

    thread_->quit();
    thread_->wait();
    delete thread_;
    thread_ = nullptr;
  }
}

void RendererThreadWrapper::ClearDestination(double r, double g, double b, double a)
{
  QMetaObject::invokeMethod(inner_, "ClearDestination", Qt::BlockingQueuedConnection,
                            Q_ARG(double, r),
                            Q_ARG(double, g),
                            Q_ARG(double, b),
                            Q_ARG(double, a));
}

QVariant RendererThreadWrapper::CreateNativeTexture(VideoParams param, void *data, int linesize)
{
  QVariant v;

  QMetaObject::invokeMethod(inner_, "CreateNativeTexture", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, v),
                            OLIVE_NS_ARG(VideoParams, param),
                            Q_ARG(void*, data),
                            Q_ARG(int, linesize));

  return v;
}

void RendererThreadWrapper::DestroyNativeTexture(QVariant texture)
{
  QMetaObject::invokeMethod(inner_, "DestroyNativeTexture", Qt::BlockingQueuedConnection,
                            Q_ARG(QVariant, texture));
}

QVariant RendererThreadWrapper::CreateNativeShader(ShaderCode code)
{
  QVariant v;

  QMetaObject::invokeMethod(inner_, "CreateNativeShader", Qt::BlockingQueuedConnection,
                            Q_RETURN_ARG(QVariant, v),
                            OLIVE_NS_ARG(ShaderCode, code));

  return v;
}

void RendererThreadWrapper::DestroyNativeShader(QVariant shader)
{
  QMetaObject::invokeMethod(inner_, "DestroyNativeShader", Qt::BlockingQueuedConnection,
                            Q_ARG(QVariant, shader));
}

void RendererThreadWrapper::UploadToTexture(Renderer::Texture *texture, void *data, int linesize)
{
  QMetaObject::invokeMethod(inner_, "UploadToTexture", Qt::BlockingQueuedConnection,
                            OLIVE_NS_ARG(Renderer::Texture*, texture),
                            Q_ARG(void*, data),
                            Q_ARG(int, linesize));
}

void RendererThreadWrapper::DownloadFromTexture(Renderer::Texture *texture, void *data, int linesize)
{
  QMetaObject::invokeMethod(inner_, "DownloadFromTexture", Qt::BlockingQueuedConnection,
                            OLIVE_NS_ARG(Renderer::Texture*, texture),
                            Q_ARG(void*, data),
                            Q_ARG(int, linesize));
}

void RendererThreadWrapper::Blit(QVariant shader, ShaderJob job, Renderer::Texture *destination, VideoParams destination_params)
{
  Renderer::TexturePtr tex;

  QMetaObject::invokeMethod(inner_, "Blit", Qt::BlockingQueuedConnection,
                            Q_ARG(QVariant, shader),
                            OLIVE_NS_ARG(ShaderJob, job),
                            OLIVE_NS_ARG(Renderer::Texture*, destination),
                            OLIVE_NS_ARG(VideoParams, destination_params));
}

OLIVE_NAMESPACE_EXIT
