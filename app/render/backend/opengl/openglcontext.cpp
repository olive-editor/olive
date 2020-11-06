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

#include "openglcontext.h"

#include <QDebug>

OLIVE_NAMESPACE_ENTER

OpenGLContext::OpenGLContext(QObject* parent) :
  RenderContext(parent)
{
}

OpenGLContext::~OpenGLContext()
{
}

bool OpenGLContext::Init()
{
  surface_.create();

  context_ = new QOpenGLContext();
  if (!context_->create()) {
    qCritical() << "Failed to create OpenGL context";
    return false;
  }

  context_->moveToThread(this->thread());

  return true;
}

void OpenGLContext::PostInit()
{
  // Make context current on that surface
  if (!context_->makeCurrent(&surface_)) {
    qCritical() << "Failed to makeCurrent() on offscreen surface in thread" << thread();
    return;
  }

  // Store OpenGL functions instance
  functions_ = context_->functions();
  functions_->glBlendFunc(GL_ONE, GL_ZERO);
}

void OpenGLContext::Destroy()
{
  delete context_;
  surface_.destroy();
}

QVariant OpenGLContext::CreateTexture(const VideoParams &p, void *data, int linesize)
{
  GLuint texture;
  functions_->glGenTextures(1, &texture);
  texture_params_.insert(texture, p);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  functions_->glTexImage2D(GL_TEXTURE_2D, 0, GetInternalFormat(p.format()),
                           p.width(), p.height(), 0, GetPixelFormat(p.format()),
                           GetPixelType(p.format()), data);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  return texture;
}

void OpenGLContext::DestroyTexture(QVariant texture)
{
  GLuint t = texture.value<GLuint>();
  functions_->glDeleteTextures(1, &t);
  texture_params_.remove(t);
}

void OpenGLContext::UploadToTexture(QVariant texture, void *data, int linesize)
{
  GLuint t = texture.value<GLuint>();
  const VideoParams& p = texture_params_.value(t);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, linesize);

  functions_->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                              p.effective_width(), p.effective_height(),
                              GetPixelFormat(p.format()), GetPixelType(p.format()),
                              data);

  functions_->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
}

void OpenGLContext::DownloadFromTexture(QVariant texture, void *data, int linesize)
{
  GLuint t = texture.value<GLuint>();
  const VideoParams& p = texture_params_.value(t);

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, linesize);

  functions_->glReadPixels(0,
                           0,
                           p.width(),
                           p.height(),
                           GetPixelFormat(p.format()),
                           GetPixelType(p.format()),
                           data);

  functions_->glPixelStorei(GL_PACK_ROW_LENGTH, 0);
}

VideoParams OpenGLContext::GetParamsFromTexture(QVariant texture)
{
  GLuint t = texture.value<GLuint>();

  return texture_params_.value(t);
}

GLint OpenGLContext::GetInternalFormat(PixelFormat::Format format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGB8:
    return GL_RGB8;
  case PixelFormat::PIX_FMT_RGBA8:
    return GL_RGBA8;
  case PixelFormat::PIX_FMT_RGB16U:
    return GL_RGB16;
  case PixelFormat::PIX_FMT_RGBA16U:
    return GL_RGBA16;
  case PixelFormat::PIX_FMT_RGB16F:
    return GL_RGB16F;
  case PixelFormat::PIX_FMT_RGBA16F:
    return GL_RGBA16F;
  case PixelFormat::PIX_FMT_RGB32F:
    return GL_RGB32F;
  case PixelFormat::PIX_FMT_RGBA32F:
    return GL_RGBA32F;

  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return GL_INVALID_VALUE;
}

GLenum OpenGLContext::GetPixelFormat(PixelFormat::Format format)
{
  if (PixelFormat::FormatHasAlphaChannel(format)) {
    return GL_RGBA;
  } else {
    return GL_RGB;
  }
}

GLenum OpenGLContext::GetPixelType(PixelFormat::Format format)
{
  switch (format) {
  case PixelFormat::PIX_FMT_RGB8:
  case PixelFormat::PIX_FMT_RGBA8:
    return GL_UNSIGNED_BYTE;
  case PixelFormat::PIX_FMT_RGB16U:
  case PixelFormat::PIX_FMT_RGBA16U:
    return GL_UNSIGNED_SHORT;
  case PixelFormat::PIX_FMT_RGB16F:
  case PixelFormat::PIX_FMT_RGBA16F:
    return GL_HALF_FLOAT;
  case PixelFormat::PIX_FMT_RGB32F:
  case PixelFormat::PIX_FMT_RGBA32F:
    return GL_FLOAT;

  case PixelFormat::PIX_FMT_INVALID:
  case PixelFormat::PIX_FMT_COUNT:
    break;
  }

  return GL_INVALID_VALUE;
}

OLIVE_NAMESPACE_EXIT
