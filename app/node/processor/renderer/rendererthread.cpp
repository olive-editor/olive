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

RendererThread::RendererThread(const int &width, const int &height, const olive::PixelFormat &format, const olive::RenderMode &mode) :
  cancelled_(false),
  width_(width),
  height_(height),
  format_(format),
  mode_(mode),
  render_instance_(nullptr)
{
}

bool RendererThread::Queue(Node *n, const rational& time)
{
  // If the thread is inactive, tryLock() will succeed
  if (mutex_.tryLock()) {

    qDebug() << "[RendererThread Main] tryLock succeeded";

    // The mutex is locked in the calling thread now, so we can change the active Node
    node_ = n;
    time_ = time;

    // We can now wake up our main thread
    qDebug() << "[RendererThread Main] Waking thread";
    wait_cond_.wakeAll();
    qDebug() << "[RendererThread Main] Unlocking mutex";
    mutex_.unlock();

    // Wait for thread to start before returning
    qDebug() << "[RendererThread Main] Waiting for caller mutex";
    caller_mutex_.lock();
    qDebug() << "[RendererThread Main] Caller mutex arrived";
    caller_mutex_.unlock();

    return true;
  }

  return false;
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

  RenderInstance instance(width_, height_, format_, mode_);
  render_instance_ = &instance;

  // Allocate and create resources
  if (instance.Start()) {

    // Main loop (use Cancel() to exit it)
    while (!cancelled_) {
      // Lock the caller mutex (used in Queue() for thread synchronization)
      caller_mutex_.lock();

      // Main waiting condition
      wait_cond_.wait(&mutex_);

      // Unlock the caller mutex
      caller_mutex_.unlock();

      // Process the Node
      node_->Run(time_);
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
  queue_.clear();

  // Start the thread (the thread will unlock caller_mutex_)
  QThread::start(priority);
}
