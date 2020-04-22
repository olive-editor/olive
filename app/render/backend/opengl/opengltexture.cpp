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
#include <QtMath>

#include "openglrenderfunctions.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

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

void OpenGLTexture::Create(QOpenGLContext *ctx, int width, int height, const PixelFormat::Format &format, const void* data, int linesize)
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

  connect(created_ctx_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Destroy()), Qt::DirectConnection);

  // Create main texture
  CreateInternal(created_ctx_, &texture_, data, linesize);
}

void OpenGLTexture::Create(QOpenGLContext *ctx, int width, int height, const PixelFormat::Format &format)
{
  Create(ctx, width, height, format, nullptr, 0);
}

void OpenGLTexture::Create(QOpenGLContext *ctx, FramePtr frame)
{
  Create(ctx, frame->width(), frame->height(), frame->format(), frame->data(), frame->linesize_pixels());
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
  created_ctx_->functions()->glBindTexture(GL_TEXTURE_2D, texture_);
}

void OpenGLTexture::Release()
{
  created_ctx_->functions()->glBindTexture(GL_TEXTURE_2D, 0);
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

void OpenGLTexture::Upload(const void *data, int linesize)
{
  if (!IsCreated()) {
    qWarning() << "OpenGLTexture::Upload() called while it wasn't created";
    return;
  }

  Bind();

  created_ctx_->functions()->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  created_ctx_->functions()->glTexSubImage2D(GL_TEXTURE_2D,
                                             0,
                                             0,
                                             0,
                                             width_,
                                             height_,
                                             OpenGLRenderFunctions::GetPixelFormat(format_),
                                             OpenGLRenderFunctions::GetPixelType(format_),
                                             data);

  created_ctx_->functions()->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  Release();
}

void OpenGLTexture::CreateInternal(QOpenGLContext* create_ctx, GLuint* tex, const void *data, int linesize)
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

  // Set linesize
  f->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  // Allocate storage for texture
  f->glTexImage2D(GL_TEXTURE_2D,
                  0,
                  OpenGLRenderFunctions::GetInternalFormat(format_),
                  width_,
                  height_,
                  0,
                  OpenGLRenderFunctions::GetPixelFormat(format_),
                  OpenGLRenderFunctions::GetPixelType(format_),
                  data);

  // Return linesize to default
  f->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  // Set texture filtering to bilinear
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Release texture
  f->glBindTexture(GL_TEXTURE_2D, 0);
}

OLIVE_NAMESPACE_EXIT
