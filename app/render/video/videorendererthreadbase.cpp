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

#include "videorendererthreadbase.h"

#include <QDebug>

VideoRendererThreadBase::VideoRendererThreadBase(QOpenGLContext *share_ctx, const int &width, const int &height, const int &divider, const olive::PixelFormat &format, const olive::RenderMode &mode) :
  share_ctx_(share_ctx),
  render_instance_(width, height, divider, format, mode)
{
  connect(share_ctx_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Cancel()));
}

RenderInstance *VideoRendererThreadBase::render_instance()
{
  return &render_instance_;
}

void VideoRendererThreadBase::run()
{
  // Lock mutex for main loop
  mutex_.lock();

  render_instance_.SetShareContext(share_ctx_);

  // Allocate and create resources
  bool started = render_instance_.Start();

  // Signal that main thread can continue now
  WakeCaller();

  if (started) {

    // Main loop (use Cancel() to exit it)
    ProcessLoop();

  }

  // Free all resources
  render_instance_.Stop();

  // Unlock mutex before exiting
  mutex_.unlock();
}

void VideoRendererThreadBase::WakeCaller()
{
  // Signal that main thread can continue now
  caller_mutex_.lock();
  wait_cond_.wakeAll();
  caller_mutex_.unlock();
}

void VideoRendererThreadBase::StartThread(QThread::Priority priority)
{
  caller_mutex_.lock();

  // Start the thread
  QThread::start(priority);

  // Wait for thread to finish completion
  wait_cond_.wait(&caller_mutex_);

  caller_mutex_.unlock();
}
