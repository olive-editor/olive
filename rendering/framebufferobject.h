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

  const GLuint& buffer() const;
  const GLuint& texture() const;

  void BindBuffer() const;
  void ReleaseBuffer() const;

  void BindTexture() const;
  void ReleaseTexture() const;
private:
  QOpenGLContext* ctx_;
  GLuint buffer_;
  GLuint texture_;
};

#endif // FRAMEBUFFEROBJECT_H
