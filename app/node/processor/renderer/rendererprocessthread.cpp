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

#include "rendererprocessthread.h"

#include "renderer.h"

RendererProcessThread::RendererProcessThread(RendererProcessor* parent,
                                             QOpenGLContext *share_ctx,
                                             const int &width,
                                             const int &height,
                                             const olive::PixelFormat &format,
                                             const olive::RenderMode &mode) :
  RendererThreadBase(share_ctx, width, height, format, mode),
  parent_(parent)
{

}

bool RendererProcessThread::Queue(const NodeDependency& dep, bool wait)
{
  if (wait) {
    // Wait for thread to be available
    mutex_.lock();
  } else if (!mutex_.tryLock()) {
    return false;
  }

  // We can now change params without the other thread using them
  path_ = dep;

  // Prepare to wait for thread to respond
  caller_mutex_.lock();

  // Wake up our main thread
  wait_cond_.wakeAll();
  mutex_.unlock();

  // Wait for thread to start before returning
  wait_cond_.wait(&caller_mutex_);
  caller_mutex_.unlock();

  return true;
}

const QByteArray &RendererProcessThread::hash()
{
  return hash_;
}

RenderTexturePtr RendererProcessThread::texture()
{
  return texture_;
}

void RendererProcessThread::ProcessLoop()
{
  while (!Cancelled()) {
    // Main waiting condition
    wait_cond_.wait(&mutex_);

    // Wake up main thread
    caller_mutex_.lock();
    wait_cond_.wakeAll();
    caller_mutex_.unlock();

    // Process the Node
    NodeOutput* output_to_process = path_.node();
    Node* node_to_process = output_to_process->parent();

    node_to_process->Lock();

    QList<Node*> all_deps = node_to_process->GetDependencies();
    foreach (Node* dep, all_deps) {
      dep->Lock();
    }

    // Check hash
    QCryptographicHash hasher(QCryptographicHash::Sha1);
    node_to_process->Hash(&hasher, output_to_process, path_.time());
    hash_ = hasher.result();

    texture_ = nullptr;

    if (!parent_->HasHash(hash_)) {
      QList<NodeDependency> deps = node_to_process->RunDependencies(output_to_process, path_.time());

      // Ask for other threads to run these deps while we're here
      if (!deps.isEmpty()) {
        for (int i=1;i<deps.size();i++) {
          emit RequestSibling(deps.at(i));
        }
      }

      // Get the requested value
      texture_ = output_to_process->get_value(path_.time()).value<RenderTexturePtr>();

      render_instance()->context()->functions()->glFinish();
    }

    foreach (Node* dep, all_deps) {
      dep->Unlock();
    }

    node_to_process->Unlock();

    emit FinishedPath();
  }
}
