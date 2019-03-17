#include "framebufferobject.h"

#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QDebug>

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

  // create framebuffer object
  ctx->functions()->glGenFramebuffers(1, &buffer_);

  // create texture
  ctx->functions()->glGenTextures(1, &texture_);

  // bind framebuffer for attaching
  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer_);

  // bind texture
  ctx->functions()->glBindTexture(GL_TEXTURE_2D, texture_);

  // allocate storage for texture
  ctx->functions()->glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr
        );

  // set texture filtering to bilinear
  ctx->functions()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  ctx->functions()->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // attach texture to framebuffer
  ctx->extraFunctions()->glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0
        );

  // clear new texture
  ctx->functions()->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  ctx->functions()->glClear(GL_COLOR_BUFFER_BIT);

  // release texture
  ctx->functions()->glBindTexture(GL_TEXTURE_2D, 0);

  // release framebuffer
  ctx->functions()->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
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
  ctx_->functions()->glBindFramebuffer(GL_TEXTURE_2D, buffer_);
}

void FramebufferObject::ReleaseBuffer() const
{
  if (ctx_ == nullptr) {
    return;
  }
  ctx_->functions()->glBindFramebuffer(GL_TEXTURE_2D, 0);
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
