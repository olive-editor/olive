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

#include "rendererthread.h"

#include <QDebug>

RendererThread::RendererThread(QOpenGLContext *share_ctx, const int &width, const int &height, const olive::PixelFormat &format, const olive::RenderMode &mode) :
  share_ctx_(share_ctx),
  cancelled_(false),
  width_(width),
  height_(height),
  format_(format),
  mode_(mode),
  render_instance_(nullptr)
{
}

void RendererThread::Queue(const RenderThreadPath &path, const rational& time)
{
  // Wait for thread to be available
  mutex_.lock();

  // We can now change params without the other thread using them
  path_ = path;
  time_ = time;

  // Prepare to wait for thread to respond
  caller_mutex_.lock();

  // Wake up our main thread
  wait_cond_.wakeAll();
  mutex_.unlock();

  // Wait for thread to start before returning
  wait_cond_.wait(&caller_mutex_);
  caller_mutex_.unlock();
}

void RendererThread::Cancel()
{
  // Escape main loop
  cancelled_ = true;

  // Wait until thread is finished before returning
  wait();
}

RenderInstance *RendererThread::render_instance()
{
  return render_instance_;
}

void RendererThread::run()
{
  // Lock mutex for main loop
  mutex_.lock();

  // Signal that main thread can continue now
  caller_mutex_.lock();
  wait_cond_.wakeAll();
  caller_mutex_.unlock();

  RenderInstance instance(width_, height_, format_, mode_);
  render_instance_ = &instance;

  instance.SetShareContext(share_ctx_);

  // Allocate and create resources
  if (instance.Start()) {

    // Main loop (use Cancel() to exit it)
    while (!cancelled_) {
      // Main waiting condition
      wait_cond_.wait(&mutex_);

      // Wake up main thread
      caller_mutex_.lock();
      wait_cond_.wakeAll();
      caller_mutex_.unlock();

      // Process the Node
      for (int i=path_.size()-1;i>=0;i--) {
        path_.at(i)->Run();
      }

      emit FinishedPath();
    }
  }

  // Free all resources
  render_instance_ = nullptr;
  instance.Stop();

  // Unlock mutex before exiting
  mutex_.unlock();
}

void RendererThread::StartThread(QThread::Priority priority)
{
  path_.clear();

  caller_mutex_.lock();

  // Start the thread
  QThread::start(priority);

  // Wait for thread to finish completion
  wait_cond_.wait(&caller_mutex_);

  caller_mutex_.unlock();
}
