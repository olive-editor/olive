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

#include "openglbackend.h"

#include "openglworker.h"

OLIVE_NAMESPACE_ENTER

OpenGLBackend::OpenGLBackend(QObject* parent) :
  RenderBackend(parent),
  proxy_(nullptr)
{

}

OpenGLBackend::~OpenGLBackend()
{
  Close();

  ClearProxy();
}

RenderWorker *OpenGLBackend::CreateNewWorker()
{
  if (!proxy_) {
    proxy_ = new OpenGLProxy();

    QThread* proxy_thread = new QThread();
    proxy_thread->start(QThread::IdlePriority);
    proxy_->moveToThread(proxy_thread);

    if (!proxy_->Init()) {
      ClearProxy();
      return nullptr;
    }
  }

  return new OpenGLWorker(proxy_);
}

void OpenGLBackend::ClearProxy()
{
  if (proxy_) {
    proxy_->thread()->quit();
    proxy_->thread()->wait();
    proxy_->thread()->deleteLater();
    proxy_->deleteLater();
    proxy_ = nullptr;
  }
}

OLIVE_NAMESPACE_EXIT
