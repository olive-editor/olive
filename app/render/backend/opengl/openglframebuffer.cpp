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

#include "openglframebuffer.h"

#include <QDebug>
#include <QOpenGLExtraFunctions>

OpenGLFramebuffer::OpenGLFramebuffer() :
  context_(nullptr),
  buffer_(0),
  texture_(nullptr)
{

}

OpenGLFramebuffer::~OpenGLFramebuffer()
{
  Destroy();
}

void OpenGLFramebuffer::Create(QOpenGLContext *ctx)
{
  if (ctx == nullptr) {
    qWarning() << tr("RenderTexture::Create was passed an invalid context");
    return;
  }

  // Free any previous framebuffer
  Destroy();

  context_ = ctx;

  connect(context_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Destroy()));

  // Create framebuffer object
  context_->functions()->glGenFramebuffers(1, &buffer_);
}

void OpenGLFramebuffer::Destroy()
{
  if (context_ != nullptr) {
    disconnect(context_, SIGNAL(aboutToBeDestroyed()), this, SLOT(Destroy()));

    context_->functions()->glDeleteFramebuffers(1, &buffer_);

    buffer_ = 0;

    context_ = nullptr;
  }
}

bool OpenGLFramebuffer::IsCreated() const
{
  return (buffer_ > 0);
}

void OpenGLFramebuffer::Bind()
{
  if (context_ == nullptr) {
    return;
  }
  context_->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer_);
}

void OpenGLFramebuffer::Release()
{
  if (context_ == nullptr) {
    return;
  }
  context_->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void OpenGLFramebuffer::Attach(RenderTexturePtr texture)
{
  if (context_ == nullptr) {
    return;
  }

  texture_ = texture;
  AttachInternal(texture_->texture(), false);
}

void OpenGLFramebuffer::AttachBackBuffer(RenderTexturePtr texture)
{
  if (context_ == nullptr) {
    return;
  }

  texture_ = texture;
  AttachInternal(texture_->back_texture(), true);
}

void OpenGLFramebuffer::Detach()
{
  if (context_ == nullptr) {
    return;
  }

  QOpenGLFunctions* f = context_->functions();

  // bind framebuffer for attaching
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer_);

  context_->extraFunctions()->glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0
        );

  // release framebuffer
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

  texture_ = nullptr;
}

const GLuint &OpenGLFramebuffer::buffer() const
{
  return buffer_;
}

void OpenGLFramebuffer::AttachInternal(GLuint tex, bool clear)
{
  Detach();

  QOpenGLFunctions* f = context_->functions();

  // bind framebuffer for attaching
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer_);

  context_->extraFunctions()->glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0
        );

  if (clear) {
    context_->functions()->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    context_->functions()->glClear(GL_COLOR_BUFFER_BIT);
  }

  // release framebuffer
  f->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}
