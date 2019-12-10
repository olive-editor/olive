#ifndef OPENGLTEXTURECACHE_H
#define OPENGLTEXTURECACHE_H

#include <QMutex>

#include "opengltexture.h"
#include "render/videoparams.h"

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

  ReferencePtr Get(const VideoRenderingParams& params);

private:
  void Relinquish(Reference* ref);

  QMutex lock_;

  QList<OpenGLTexturePtr> available_textures_;

  QList<Reference*> existing_references_;

};

#endif // OPENGLTEXTURECACHE_H
