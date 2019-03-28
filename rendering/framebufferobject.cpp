#include "framebufferobject.h"

#include <QOpenGLFunctions>

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
  f->glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
        );

  // set texture filtering to bilinear
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // attach texture to framebuffer
  f->glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0
        );

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

const GLuint &FramebufferObject::buffer()
{
  return buffer_;
}

const GLuint &FramebufferObject::texture()
{
  return texture_;
}
