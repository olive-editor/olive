#ifndef FRAMEBUFFEROBJECT_H
#define FRAMEBUFFEROBJECT_H

#include <QOpenGLContext>

class FramebufferObject
{
public:
  FramebufferObject();
  ~FramebufferObject();

  bool IsCreated();
  void Create(QOpenGLContext* ctx, int width, int height);
  void Destroy();

  const GLuint& buffer();
  const GLuint& texture();
private:
  QOpenGLContext* ctx_;
  GLuint buffer_;
  GLuint texture_;
};

#endif // FRAMEBUFFEROBJECT_H
