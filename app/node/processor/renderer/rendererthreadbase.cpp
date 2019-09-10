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

#include "rendererthreadbase.h"

#include <QDebug>

RendererThreadBase::RendererThreadBase(QOpenGLContext *share_ctx, const int &width, const int &height, const int &divider, const olive::PixelFormat &format, const olive::RenderMode &mode) :
  share_ctx_(share_ctx),
  width_(width),
  height_(height),
  divider_(divider),
  format_(format),
  mode_(mode),
  render_instance_(nullptr)
{
  connect(share_ctx_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Cancel()));
}

RenderInstance *RendererThreadBase::render_instance()
{
  return render_instance_;
}

void RendererThreadBase::run()
{
  // Lock mutex for main loop
  mutex_.lock();

  // Signal that main thread can continue now
  caller_mutex_.lock();
  wait_cond_.wakeAll();
  caller_mutex_.unlock();

  RenderInstance instance(width_, height_, divider_, format_, mode_);
  render_instance_ = &instance;

  instance.SetShareContext(share_ctx_);

  qDebug() << this << "is starting its OpenGL context";

  // Allocate and create resources
  if (instance.Start()) {

    qDebug() << this << "OpenGL context creation succeeded";

    // Main loop (use Cancel() to exit it)
    ProcessLoop();

  }

  // Free all resources
  render_instance_ = nullptr;
  instance.Stop();

  // Unlock mutex before exiting
  mutex_.unlock();
}

void RendererThreadBase::StartThread(QThread::Priority priority)
{
  caller_mutex_.lock();

  // Start the thread
  QThread::start(priority);

  // Wait for thread to finish completion
  wait_cond_.wait(&caller_mutex_);

  caller_mutex_.unlock();
}
