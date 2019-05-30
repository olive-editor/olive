/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "framebufferobject.h"

#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QDebug>

#include "global/config.h"
#include "global/global.h"
#include "pixelformats.h"

FramebufferObject::FramebufferObject() :
  buffer_(0),
  texture_(0),
  ctx_(nullptr)
{}

FramebufferObject::~FramebufferObject()
{
  Destroy();
}

bool FramebufferObject::IsCreated()
{
  return ctx_ != nullptr;
}

void FramebufferObject::Create(QOpenGLContext *ctx, int width, int height)
{
  // free any previous textures
  Destroy();

  // set context to new context provided
  ctx_ = ctx;

  QOpenGLFunctions* f = ctx->functions();

  // create framebuffer object
  f->glGenFramebuffers(1, &buffer_);

  // create texture
  f->glGenTextures(1, &texture_);

  // bind framebuffer for attaching
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer_);

  // bind texture
  f->glBindTexture(GL_TEXTURE_2D, texture_);

  // allocate storage for texture
  const olive::PixelFormatInfo& bit_depth = olive::pixel_formats.at(olive::Global->effective_bit_depth());

  ctx->functions()->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        bit_depth.internal_format,
        width,
        height,
        0,
        bit_depth.pixel_format,
        bit_depth.pixel_type,
        nullptr
        );

  // set texture filtering to bilinear
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // attach texture to framebuffer
  ctx->extraFunctions()->glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0
        );

  // clear new texture
  ctx->functions()->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);

  // release texture
  f->glBindTexture(GL_TEXTURE_2D, 0);

  // release framebuffer
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FramebufferObject::Destroy()
{
  if (ctx_ != nullptr) {
    ctx_->functions()->glDeleteFramebuffers(1, &buffer_);

    ctx_->functions()->glDeleteTextures(1, &texture_);
  }

  ctx_ = nullptr;
}

void FramebufferObject::BindBuffer() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer_);
}

void FramebufferObject::ReleaseBuffer() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void FramebufferObject::BindTexture() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindTexture(GL_TEXTURE_2D, texture_);
}

void FramebufferObject::ReleaseTexture() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindTexture(GL_TEXTURE_2D, 0);
}

const GLuint &FramebufferObject::buffer() const
{
  return buffer_;
}

const GLuint &FramebufferObject::texture() const
{
  return texture_;
}
