#ifndef OPENGLCOLORPROCESSOR_H
#define OPENGLCOLORPROCESSOR_H

#include "openglshader.h"
#include "render/colorprocessor.h"

class OpenGLColorProcessor;
using OpenGLColorProcessorPtr = std::shared_ptr<OpenGLColorProcessor>;

class OpenGLColorProcessor : public QObject, public ColorProcessor
{
  Q_OBJECT
public:
  OpenGLColorProcessor(OCIO::ConstConfigRcPtr config, const QString &source_space, const QString &dest_space);

  OpenGLColorProcessor(OCIO::ConstConfigRcPtr config,
                       const QString& source_space,
                       QString display,
                       QString view,
                       const QString& look);

  ~OpenGLColorProcessor();

  static OpenGLColorProcessorPtr CreateOpenGL(OCIO::ConstConfigRcPtr config, const QString& source_space, const QString& dest_space);

  static OpenGLColorProcessorPtr CreateOpenGL(OCIO::ConstConfigRcPtr config,
                                              const QString& source_space,
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

private slots:
  void ClearTexture();

};

#endif // OPENGLCOLORPROCESSOR_H
