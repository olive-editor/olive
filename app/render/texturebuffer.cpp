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

#include "texturebuffer.h"

#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>

#include "render/pixelservice.h"

TextureBuffer::TextureBuffer() :
  ctx_(nullptr),
  buffer_(0),
  texture_(0)
{}

TextureBuffer::~TextureBuffer()
{
  Destroy();
}

bool TextureBuffer::IsCreated()
{
  return (ctx_ != nullptr);
}

void TextureBuffer::Create(QOpenGLContext *ctx, const olive::PixelFormat &format, int width, int height, void* data)
{
  // free any previous textures
  Destroy();

  // set context to new context provided
  ctx_ = ctx;

  // Store frame metadata
  width_ = width;
  height_ = height;
  format_ = format;

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
  const PixelFormatInfo& bit_depth = PixelService::GetPixelFormatInfo(format);

  ctx->functions()->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        bit_depth.internal_format,
        width,
        height,
        0,
        bit_depth.pixel_format,
        bit_depth.pixel_type,
        data
        );

  // set texture filtering to bilinear
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // attach texture to framebuffer
  ctx->extraFunctions()->glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0
        );

  // clear new texture (doesn't seem to be necessary)
  /*if (data == nullptr) {
    ctx->functions()->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);
  }*/

  // release texture
  f->glBindTexture(GL_TEXTURE_2D, 0);

  // release framebuffer
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void TextureBuffer::Destroy()
{
  if (ctx_ != nullptr) {
    ctx_->functions()->glDeleteFramebuffers(1, &buffer_);

    ctx_->functions()->glDeleteTextures(1, &texture_);

    ctx_ = nullptr;
  }
}

void TextureBuffer::Upload(void *data)
{
  if (!IsCreated()) {
    return;
  }

  BindTexture();

  PixelFormatInfo info = PixelService::GetPixelFormatInfo(format_);

  glTexSubImage2D(GL_TEXTURE_2D,
                  0,
                  0,
                  0,
                  width_,
                  height_,
                  info.pixel_format,
                  info.pixel_type,
                  data);

  ReleaseTexture();
}

void TextureBuffer::BindBuffer() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer_);
}

void TextureBuffer::ReleaseBuffer() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void TextureBuffer::BindTexture() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindTexture(GL_TEXTURE_2D, texture_);
}

void TextureBuffer::ReleaseTexture() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindTexture(GL_TEXTURE_2D, 0);
}

const GLuint &TextureBuffer::buffer() const
{
  return buffer_;
}

const GLuint &TextureBuffer::texture() const
{
  return texture_;
}
