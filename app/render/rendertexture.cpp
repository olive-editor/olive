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

#include "rendertexture.h"

#include <QDateTime>
#include <QDebug>

#include "render/pixelservice.h"

RenderTexture::RenderTexture() :
  context_(nullptr),
  texture_(0)
{

}

RenderTexture::~RenderTexture()
{
  Destroy();
}

bool RenderTexture::IsCreated() const
{
  return (texture_ != 0);
}

void RenderTexture::Create(QOpenGLContext *ctx, int width, int height, const olive::PixelFormat &format, void* data)
{
  Create(ctx, width, height, format, kSingleBuffer, data);
}

void RenderTexture::Create(QOpenGLContext *ctx, int width, int height, const olive::PixelFormat &format, const RenderTexture::Type &type, void *data)
{
  if (ctx == nullptr) {
    qWarning() << tr("RenderTexture::Create was passed an invalid context");
    return;
  }

  Destroy();

  context_ = ctx;
  width_ = width;
  height_ = height;
  format_ = format;

  // Create main texture
  CreateInternal(data);

  if (type == kDoubleBuffer) {
    // Create back texture
    CreateInternal(nullptr);
  }
}

void RenderTexture::Destroy()
{
  if (context_ != nullptr) {
    context_->functions()->glDeleteTextures(1, &texture_);
    texture_ = 0;

    context_->functions()->glDeleteTextures(1, &back_texture_);
    back_texture_ = 0;

    context_ = nullptr;
  }
}

void RenderTexture::Bind()
{
  if (context_ == nullptr) {
    qWarning() << "RenderTexture::Bind() called with an invalid context";
    return;
  }

  context_->functions()->glBindTexture(GL_TEXTURE_2D, texture_);
}

void RenderTexture::Release()
{
  if (context_ == nullptr) {
    qWarning() << "RenderTexture::Release() called with an invalid context";
    return;
  }

  context_->functions()->glBindTexture(GL_TEXTURE_2D, 0);
}

const int &RenderTexture::width() const
{
  return width_;
}

const int &RenderTexture::height() const
{
  return height_;
}

const olive::PixelFormat &RenderTexture::format() const
{
  return format_;
}

QOpenGLContext *RenderTexture::context() const
{
  return context_;
}

const GLuint &RenderTexture::texture() const
{
  return texture_;
}

const GLuint &RenderTexture::back_texture() const
{
  return back_texture_;
}

void RenderTexture::SwapFrontAndBack()
{
  GLuint temp = texture_;
  texture_ = back_texture_;
  back_texture_ = temp;
}

void RenderTexture::Upload(const void *data)
{
  if (!IsCreated()) {
    qWarning() << tr("RenderTexture::Upload() called while it wasn't created");
    return;
  }

  Bind();

  PixelFormatInfo info = PixelService::GetPixelFormatInfo(format_);

  context_->functions()->glTexSubImage2D(GL_TEXTURE_2D,
                                         0,
                                         0,
                                         0,
                                         width_,
                                         height_,
                                         info.pixel_format,
                                         info.pixel_type,
                                         data);

  Release();
}

uchar *RenderTexture::Download() const
{
  if (!IsCreated()) {
    qWarning() << tr("RenderTexture::Download() called while it wasn't created");
    return nullptr;
  }

  QOpenGLFunctions* f = context_->functions();

  GLuint read_fbo;

  f->glGenFramebuffers(1, &read_fbo);

  f->glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);

  context_->extraFunctions()->glFramebufferTexture2D(
        GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0
        );

  PixelFormatInfo format_info = PixelService::GetPixelFormatInfo(format_);

  uchar* data = new uchar[PixelService::GetBufferSize(format_, width_, height_)];

  f->glReadPixels(0, 0, width_, height_, format_info.pixel_format, format_info.pixel_type, data);

  f->glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

  f->glDeleteFramebuffers(1, &read_fbo);

  return data;
}

void RenderTexture::CreateInternal(void *data)
{
  QOpenGLFunctions* f = context_->functions();

  // Create texture
  f->glGenTextures(1, &texture_);

  // Verify texture
  if (texture_ == 0) {
    qWarning() << tr("OpenGL texture creation failed");
    return;
  }

  // Bind texture
  f->glBindTexture(GL_TEXTURE_2D, texture_);

  // Allocate storage for texture
  const PixelFormatInfo& bit_depth = PixelService::GetPixelFormatInfo(format_);

  f->glTexImage2D(
        GL_TEXTURE_2D,
        0,
        bit_depth.internal_format,
        width_,
        height_,
        0,
        bit_depth.pixel_format,
        bit_depth.pixel_type,
        data
        );

  // Set texture filtering to bilinear
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Release texture
  f->glBindTexture(GL_TEXTURE_2D, 0);
}
