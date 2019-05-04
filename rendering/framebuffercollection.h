#ifndef FRAMEBUFFERCOLLECTION_H
#define FRAMEBUFFERCOLLECTION_H

#include <QVector>

#include "framebufferobject.h"

class FramebufferCollection
{
public:
  FramebufferCollection();

  void Create(QOpenGLContext *ctx, int width, int height, int count);
  void Destroy();

  GLuint CurrentTexture();
  const FramebufferObject& CurrentFramebuffer();
  const FramebufferObject& NextFramebuffer();
  bool TextureBelongsToCollection(GLuint tex);

  bool IsCreated();
private:
  QVector<FramebufferObject> fbo_;
  int fbo_index_;
};

#endif // FRAMEBUFFERCOLLECTION_H
