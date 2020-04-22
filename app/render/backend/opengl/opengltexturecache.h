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

#ifndef OPENGLTEXTURECACHE_H
#define OPENGLTEXTURECACHE_H

#include <QMutex>

#include "openglframebuffer.h"
#include "opengltexture.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class OpenGLTextureCache
{
public:
  class Reference {
  public:
    Reference(OpenGLTextureCache* parent, OpenGLTexturePtr texture);
    ~Reference();

    DISABLE_COPY_MOVE(Reference)

    OpenGLTexturePtr texture();

    void ParentKilled();

  private:
    OpenGLTextureCache* parent_;

    OpenGLTexturePtr texture_;
  };

  using ReferencePtr = std::shared_ptr<Reference>;

  OpenGLTextureCache() = default;

  ~OpenGLTextureCache();

  DISABLE_COPY_MOVE(OpenGLTextureCache)

  ReferencePtr Get(QOpenGLContext *ctx, const VideoRenderingParams& params, const void *data, int linesize);
  ReferencePtr Get(QOpenGLContext *ctx, const VideoRenderingParams& params);

private:
  void Relinquish(Reference* ref);

  QMutex lock_;

  QList<OpenGLTexturePtr> available_textures_;

  QList<Reference*> existing_references_;

};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::OpenGLTextureCache::ReferencePtr)

#endif // OPENGLTEXTURECACHE_H
