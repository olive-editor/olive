#include "opengltexturecache.h"

OpenGLTextureCache::~OpenGLTextureCache()
{
  foreach (Reference* ref, existing_references_) {
    ref->ParentKilled();
  }
}

OpenGLTextureCache::ReferencePtr OpenGLTextureCache::Get(QOpenGLContext* ctx, const VideoRenderingParams &params, const void *data)
{
  OpenGLTexturePtr texture = nullptr;

  lock_.lock();

  // Iterate through textures and see if we have one that matches these parameters
  for (int i=0;i<available_textures_.size();i++) {
    OpenGLTexturePtr test = available_textures_.at(i);

    if (test->width() == params.effective_width()
        && test->height() == params.effective_height()
        && test->format() == params.format()) {
      texture = test;
      available_textures_.removeAt(i);
      break;
    }
  }

  lock_.unlock();

  // If we didn't find a texture, we'll need to create one
  if (!texture) {
    texture = std::make_shared<OpenGLTexture>();
    texture->Create(ctx, params.effective_width(), params.effective_height(), params.format(), data);
  } else if (data) {
    texture->Upload(data);
  }

  ReferencePtr ref = std::make_shared<Reference>(this, texture);
  existing_references_.append(ref.get());
  return ref;
}

void OpenGLTextureCache::Relinquish(OpenGLTextureCache::Reference *ref)
{
  OpenGLTexturePtr tex = ref->texture();

  lock_.lock();

  existing_references_.removeOne(ref);
  available_textures_.append(tex);

  lock_.unlock();
}

OpenGLTextureCache::Reference::Reference(OpenGLTextureCache *parent, OpenGLTexturePtr texture) :
  parent_(parent),
  texture_(texture)
{
}

OpenGLTextureCache::Reference::~Reference()
{
  if (parent_) {
    parent_->Relinquish(this);
  }
}

OpenGLTexturePtr OpenGLTextureCache::Reference::texture()
{
  return texture_;
}

void OpenGLTextureCache::Reference::ParentKilled()
{
  parent_ = nullptr;
}
