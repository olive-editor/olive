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

#include "renderinstance.h"

#include <QDebug>

RenderInstance::RenderInstance(const int& width,
                               const int& height,
                               const olive::PixelFormat& format,
                               const olive::RenderMode& mode) :
  share_ctx_(nullptr),
  width_(width),
  height_(height),
  format_(format),
  mode_(mode)
{
}

void RenderInstance::SetShareContext(QOpenGLContext *share)
{
  Q_ASSERT(!IsStarted());

  share_ctx_ = share;
}

bool RenderInstance::Start()
{
  if (IsStarted()) {
    return true;
  }

  // If we're sharing resources, set this up now
  if (share_ctx_ != nullptr) {
    ctx_.setShareContext(share_ctx_);
  }

  // Create OpenGL context (automatically destroys any existing if there is one)
  if (!ctx_.create()) {
    qWarning() << tr("Failed to create OpenGL context in thread %1").arg(reinterpret_cast<quintptr>(this));
    return false;
  }

  // Create offscreen surface
  surface_.create();

  // Make context current on that surface
  if (!ctx_.makeCurrent(&surface_)) {
    qWarning() << tr("Failed to makeCurrent() on offscreen surface in thread %1").arg(reinterpret_cast<quintptr>(this));
    surface_.destroy();
    return false;
  }

  buffer_.Create(&ctx_);

  ctx_.functions()->glViewport(0, 0, width_, height_);

  return true;
}

void RenderInstance::Stop()
{
  // Destroy buffer
  buffer_.Destroy();

  // Release OpenGL context
  ctx_.doneCurrent();

  // Destroy offscreen surface
  surface_.destroy();
}

bool RenderInstance::IsStarted()
{
  return buffer_.IsCreated();
}

RenderFramebuffer *RenderInstance::buffer()
{
  return &buffer_;
}

QOpenGLContext *RenderInstance::context()
{
  return &ctx_;
}

const int &RenderInstance::width() const
{
  return width_;
}

const int &RenderInstance::height() const
{
  return height_;
}

const olive::PixelFormat &RenderInstance::format() const
{
  return format_;
}

const olive::RenderMode &RenderInstance::mode() const
{
  return mode_;
}
