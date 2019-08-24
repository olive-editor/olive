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
  width_(width),
  height_(height),
  format_(format),
  mode_(mode)
{
}

bool RenderInstance::Start()
{
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

  buffer_.Create(&ctx_, format_, width_, height_);

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

TextureBuffer *RenderInstance::buffer()
{
  return &buffer_;
}
