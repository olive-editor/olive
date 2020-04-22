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

#include "opengltexturecache.h"

OLIVE_NAMESPACE_ENTER

OpenGLTextureCache::~OpenGLTextureCache()
{
  foreach (Reference* ref, existing_references_) {
    ref->ParentKilled();
  }
}

OpenGLTextureCache::ReferencePtr OpenGLTextureCache::Get(QOpenGLContext* ctx, const VideoRenderingParams &params, const void *data, int linesize)
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

  // If we didn't find a texture, we'll need to create one
  if (!texture) {
    texture = std::make_shared<OpenGLTexture>();
    texture->Create(ctx, params.effective_width(), params.effective_height(), params.format());
  }

  ReferencePtr ref = std::make_shared<Reference>(this, texture);
  existing_references_.append(ref.get());

  lock_.unlock();

  if (data) {
    texture->Upload(data, linesize);
  }

  return ref;
}

OpenGLTextureCache::ReferencePtr OpenGLTextureCache::Get(QOpenGLContext *ctx, const VideoRenderingParams &params)
{
  return Get(ctx, params, nullptr, 0);
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

OLIVE_NAMESPACE_EXIT
