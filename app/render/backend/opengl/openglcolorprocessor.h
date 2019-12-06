#ifndef OPENGLCOLORPROCESSOR_H
#define OPENGLCOLORPROCESSOR_H

#include "openglshader.h"
#include "render/colorprocessor.h"

class OpenGLColorProcessor;
using OpenGLColorProcessorPtr = std::shared_ptr<OpenGLColorProcessor>;

class OpenGLColorProcessor : public ColorProcessor
{
public:
  OpenGLColorProcessor(const QString &source_space, const QString &dest_space);

  OpenGLColorProcessor(const QString& source_space,
                       QString display,
                       QString view,
                       const QString& look);

  ~OpenGLColorProcessor();

  static OpenGLColorProcessorPtr CreateOpenGL(const QString& source_space, const QString& dest_space);

  static OpenGLColorProcessorPtr CreateOpenGL(const QString& source_space,
                                              const QString& display,
                                              const QString& view,
                                              const QString& look);

  void Enable(QOpenGLContext* context, bool alpha_is_associated);
  bool IsEnabled() const;

  OpenGLShaderPtr pipeline() const;

  void ProcessOpenGL();

private:
  QOpenGLContext* context_;

  GLuint ocio_lut_;

  OpenGLShaderPtr pipeline_;

};

#endif // OPENGLCOLORPROCESSOR_H
