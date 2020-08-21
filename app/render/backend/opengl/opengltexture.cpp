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
  texture_(0)
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

void OpenGLTexture::Create(QOpenGLContext *ctx, const VideoParams &params, const void* data, int linesize)
{
  if (!ctx) {
    qWarning() << "OpenGLTexture::Create was passed an invalid context";
    return;
  }

  Destroy();

  created_ctx_ = ctx;
  params_ = params;

  connect(created_ctx_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Destroy()), Qt::DirectConnection);

  // Create main texture
  CreateInternal(created_ctx_, &texture_, data, linesize);
}

void OpenGLTexture::Create(QOpenGLContext *ctx, const VideoParams &params)
{
  Create(ctx, params, nullptr, 0);
}

void OpenGLTexture::Create(QOpenGLContext *ctx, FramePtr frame)
{
  Create(ctx, frame.get());
}

void OpenGLTexture::Create(QOpenGLContext *ctx, Frame *frame)
{
  Create(ctx, frame->video_params(), frame->data(), frame->linesize_pixels());
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

void OpenGLTexture::SetPixelAspectRatio(const rational &r)
{
  params_ = VideoParams(params_.width(),
                        params_.height(),
                        params_.time_base(),
                        params_.format(),
                        r,
                        params_.interlacing(),
                        params_.divider());
}

void OpenGLTexture::Upload(FramePtr frame)
{
  Upload(frame.get());
}

void OpenGLTexture::Upload(Frame *frame)
{
  Upload(frame->data(), frame->linesize_pixels());
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
                                             width(),
                                             height(),
                                             OpenGLRenderFunctions::GetPixelFormat(format()),
                                             OpenGLRenderFunctions::GetPixelType(format()),
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
                  OpenGLRenderFunctions::GetInternalFormat(format()),
                  width(),
                  height(),
                  0,
                  OpenGLRenderFunctions::GetPixelFormat(format()),
                  OpenGLRenderFunctions::GetPixelType(format()),
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
