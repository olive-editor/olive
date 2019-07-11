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

RendererThread::RendererThread() :
  cancelled_(false)
{
}

bool RendererThread::Queue(Node *n, const rational& time)
{
  // If the thread is inactive, tryLock() will succeed
  if (mutex_.tryLock()) {

    // The mutex is locked in the calling thread now, so we can change the active Node
    node_ = n;
    time_ = time;

    // We can now wake up our main thread
    wait_cond_.wakeAll();
    mutex_.unlock();

    // Wait for thread to start before returning
    caller_mutex_.lock();
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

void RendererThread::run()
{
  // Create OpenGL context (automatically destroys any existing if there is one)
  if (!ctx_.create()) {
    qWarning() << tr("Failed to create OpenGL context in thread %1").arg(reinterpret_cast<quintptr>(this));
    return;
  }

  // Create offscreen surface
  surface_.create();

  // Make context current on that surface
  if (!ctx_.makeCurrent(&surface_)) {
    qWarning() << tr("Failed to makeCurrent() on offscreen surface in thread %1").arg(reinterpret_cast<quintptr>(this));
    surface_.destroy();
    return;
  }

  // Lock mutex for main loop
  mutex_.lock();

  // Main loop (use Cancel() to exit it)
  while (!cancelled_) {
    // Lock the caller mutex (used in Queue() for thread synchronization)
    caller_mutex_.lock();

    // Main waiting condition
    wait_cond_.wait(&mutex_);

    // Unlock the caller mutex
    caller_mutex_.unlock();

    // Process the Node
    node_->Process(time_);
  }

  // Release OpenGL context
  ctx_.doneCurrent();

  // Destroy offscreen surface
  surface_.destroy();

  // Unlock mutex before exiting
  mutex_.unlock();
}
