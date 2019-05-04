#include "framebuffercollection.h"

FramebufferCollection::FramebufferCollection()
{

}

void FramebufferCollection::Create(QOpenGLContext* ctx,
                                   int width,
                                   int height,
                                   int count)
{
  Q_ASSERT(count > 1);

  fbo_.resize(count);
  for (int i=0;i<fbo_.size();i++) {
    fbo_[i].Create(ctx, width, height);
  }
  fbo_index_ = -1;
}

void FramebufferCollection::Destroy()
{
  fbo_.clear();
}

GLuint FramebufferCollection::CurrentTexture()
{
  if (!IsCreated() || fbo_index_ < 0) {
    return 0;
  }

  return fbo_.at(fbo_index_%fbo_.size()).texture();
}

const FramebufferObject& FramebufferCollection::CurrentFramebuffer()
{
  Q_ASSERT(IsCreated());

  return fbo_.at(fbo_index_%fbo_.size());
}

const FramebufferObject& FramebufferCollection::NextFramebuffer()
{
  fbo_index_++;

  return CurrentFramebuffer();
}

bool FramebufferCollection::TextureBelongsToCollection(GLuint tex)
{
  if (tex == 0) {
    return false;
  }

  for (int i=0;i<fbo_.size();i++) {
    if (fbo_.at(i).texture() == tex) {
      return true;
    }
  }

  return false;
}

bool FramebufferCollection::IsCreated()
{
  return !fbo_.isEmpty();
}
