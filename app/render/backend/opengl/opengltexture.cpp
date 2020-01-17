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

#include "opengltexture.h"

#include <QDateTime>
#include <QDebug>

#include "render/pixelservice.h"

OpenGLTexture::OpenGLTexture() :
  created_ctx_(nullptr),
  texture_(0),
  width_(0),
  height_(0),
  format_(PixelFormat::PIX_FMT_INVALID)
{
}

OpenGLTexture::~OpenGLTexture()
{
  Destroy();
}

bool OpenGLTexture::IsCreated() const
{
  return (texture_);
}

void OpenGLTexture::Create(QOpenGLContext *ctx, int width, int height, const PixelFormat::Format &format, const void* data)
{
  if (!ctx) {
    qWarning() << "OpenGLTexture::Create was passed an invalid context";
    return;
  }

  Destroy();

  created_ctx_ = ctx;
  width_ = width;
  height_ = height;
  format_ = format;

  connect(created_ctx_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Destroy()));

  // Create main texture
  CreateInternal(created_ctx_, &texture_, data);
}

void OpenGLTexture::Create(QOpenGLContext *ctx, FramePtr frame)
{
  Create(ctx, frame->width(), frame->height(), frame->format(), frame->data());
}

void OpenGLTexture::Destroy()
{
  if (created_ctx_) {
    disconnect(created_ctx_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Destroy()));

    created_ctx_->functions()->glDeleteTextures(1, &texture_);
    texture_ = 0;

    created_ctx_ = nullptr;
  }
}

void OpenGLTexture::Bind()
{
  QOpenGLContext* context = QOpenGLContext::currentContext();

  if (!context) {
    qWarning() << "OpenGLTexture::Bind() called with an invalid context";
    return;
  }

  context->functions()->glBindTexture(GL_TEXTURE_2D, texture_);
}

void OpenGLTexture::Release()
{
  QOpenGLContext* context = QOpenGLContext::currentContext();

  if (!context) {
    qWarning() << "OpenGLTexture::Release() called with an invalid context";
    return;
  }

  context->functions()->glBindTexture(GL_TEXTURE_2D, 0);
}

const int &OpenGLTexture::width() const
{
  return width_;
}

const int &OpenGLTexture::height() const
{
  return height_;
}

const PixelFormat::Format &OpenGLTexture::format() const
{
  return format_;
}

const GLuint &OpenGLTexture::texture() const
{
  return texture_;
}

void OpenGLTexture::Upload(const void *data)
{
  if (!IsCreated()) {
    qWarning() << "OpenGLTexture::Upload() called while it wasn't created";
    return;
  }

  QOpenGLContext* context = QOpenGLContext::currentContext();

  if (!context) {
    qWarning() << "OpenGLTexture::Release() called with an invalid context";
    return;
  }

  Bind();

  PixelFormat::Info info = PixelService::GetPixelFormatInfo(format_);

  context->functions()->glTexSubImage2D(GL_TEXTURE_2D,
                                        0,
                                        0,
                                        0,
                                        width_,
                                        height_,
                                        info.pixel_format,
                                        info.gl_pixel_type,
                                        data);

  Release();
}

void OpenGLTexture::Lock()
{
  mutex_.lock();
}

void OpenGLTexture::Unlock()
{
  mutex_.unlock();
}

void OpenGLTexture::CreateInternal(QOpenGLContext* create_ctx, GLuint* tex, const void *data)
{
  QOpenGLFunctions* f = create_ctx->functions();

  // Create texture
  f->glGenTextures(1, tex);

  // Verify texture
  if (texture_ == 0) {
    qWarning() << "OpenGL texture creation failed";
    return;
  }

  // Bind texture
  f->glBindTexture(GL_TEXTURE_2D, *tex);

  // Allocate storage for texture
  const PixelFormat::Info& bit_depth = PixelService::GetPixelFormatInfo(format_);

  f->glTexImage2D(GL_TEXTURE_2D,
                  0,
                  bit_depth.internal_format,
                  width_,
                  height_,
                  0,
                  bit_depth.pixel_format,
                  bit_depth.gl_pixel_type,
                  data);

  // Set texture filtering to bilinear
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Release texture
  f->glBindTexture(GL_TEXTURE_2D, 0);
}
