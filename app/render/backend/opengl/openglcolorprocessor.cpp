#include "openglcolorprocessor.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include "openglrenderfunctions.h"

void OpenGLColorProcessor::Enable(QOpenGLContext *context, bool alpha_is_associated)
{
  if (IsEnabled()) {
    return;
  }

  context_ = context;

  pipeline_ = OpenGLShader::CreateOCIO(context_,
                                       ocio_lut_,
                                       GetProcessor(),
                                       alpha_is_associated);

  connect(context_, &QOpenGLContext::aboutToBeDestroyed, this, &OpenGLColorProcessor::ClearTexture, Qt::DirectConnection);
}

bool OpenGLColorProcessor::IsEnabled() const
{
  return ocio_lut_;
}

OpenGLShaderPtr OpenGLColorProcessor::pipeline() const
{
  return pipeline_;
}

void OpenGLColorProcessor::ProcessOpenGL(bool flipped, const QMatrix4x4& matrix)
{
  OpenGLRenderFunctions::OCIOBlit(pipeline_, ocio_lut_, flipped, matrix);
}

void OpenGLColorProcessor::ClearTexture()
{
  if (IsEnabled()) {
    // Clean up OCIO LUT texture and shader
    context_->functions()->glDeleteTextures(1, &ocio_lut_);

    disconnect(context_, &QOpenGLContext::aboutToBeDestroyed, this, &OpenGLColorProcessor::ClearTexture);

    ocio_lut_ = 0;
    pipeline_ = nullptr;
  }
}

OpenGLColorProcessor::OpenGLColorProcessor(OCIO::ConstConfigRcPtr config, const QString &source_space, const QString &dest_space) :
  ColorProcessor(config, source_space, dest_space),
  ocio_lut_(0)
{
}

OpenGLColorProcessor::OpenGLColorProcessor(OCIO::ConstConfigRcPtr config, const QString &source_space, QString display, QString view, const QString &look) :
  ColorProcessor(config, source_space, display, view, look),
  ocio_lut_(0)
{
}

OpenGLColorProcessor::~OpenGLColorProcessor()
{
  ClearTexture();
}

OpenGLColorProcessorPtr OpenGLColorProcessor::CreateOpenGL(OCIO::ConstConfigRcPtr config, const QString &source_space, const QString &dest_space)
{
  return std::make_shared<OpenGLColorProcessor>(config, source_space, dest_space);
}

OpenGLColorProcessorPtr OpenGLColorProcessor::CreateOpenGL(OCIO::ConstConfigRcPtr config, const QString &source_space, const QString &display, const QString &view, const QString &look)
{
  return std::make_shared<OpenGLColorProcessor>(config, source_space, display, view, look);
}
