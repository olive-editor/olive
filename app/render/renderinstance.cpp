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

#include "render/gl/shadergenerators.h"

RenderInstance::RenderInstance(const VideoRenderingParams& params) :
  share_ctx_(nullptr),
  params_(params)
{
  // Create offscreen surface
  surface_.create();
}

RenderInstance::~RenderInstance()
{
  // Destroy offscreen surface
  surface_.destroy();
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

  // Create context object
  ctx_ = new QOpenGLContext();

  // If we're sharing resources, set this up now
  if (share_ctx_ != nullptr) {
    ctx_->setShareContext(share_ctx_);
  }

  // Create OpenGL context (automatically destroys any existing if there is one)
  if (!ctx_->create()) {
    qWarning() << tr("Failed to create OpenGL context in thread %1").arg(reinterpret_cast<quintptr>(this));
    return false;
  }

  // Make context current on that surface
  if (!ctx_->makeCurrent(&surface_)) {
    qWarning() << tr("Failed to makeCurrent() on offscreen surface in thread %1").arg(reinterpret_cast<quintptr>(this));
    return false;
  }

  buffer_.Create(ctx_);

  // Set viewport to the compositing dimensions
  ctx_->functions()->glViewport(0, 0, params_.effective_width(), params_.effective_height());
  ctx_->functions()->glEnable(GL_BLEND);

  // Set up default pipeline
  default_pipeline_ = olive::ShaderGenerator::DefaultPipeline();

  return true;
}

void RenderInstance::Stop()
{
  if (IsStarted()) {
    return;
  }

  // Destroy pipeline
  default_pipeline_ = nullptr;

  // Destroy buffer
  buffer_.Destroy();

  // Destroy context
  delete ctx_;
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
  return ctx_;
}

const VideoRenderingParams &RenderInstance::params() const
{
  return params_;
}

ShaderPtr RenderInstance::default_pipeline() const
{
  return default_pipeline_;
}
