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

#include "openglcolorprocessor.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions>

#include "openglrenderfunctions.h"

OLIVE_NAMESPACE_ENTER

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

OpenGLColorProcessor::OpenGLColorProcessor(ColorManager* config, const QString &source_space, const QString &dest_space) :
  ColorProcessor(config, source_space, dest_space),
  ocio_lut_(0)
{
}

OpenGLColorProcessor::OpenGLColorProcessor(ColorManager* config, const QString &source_space, QString display, QString view, const QString &look, Direction dir) :
  ColorProcessor(config, source_space, display, view, look, dir),
  ocio_lut_(0)
{
}

OpenGLColorProcessor::~OpenGLColorProcessor()
{
  ClearTexture();
}

OpenGLColorProcessorPtr OpenGLColorProcessor::Create(ColorManager *config, const QString &source_space, const QString &dest_space)
{
  return std::make_shared<OpenGLColorProcessor>(config, source_space, dest_space);
}

OpenGLColorProcessorPtr OpenGLColorProcessor::Create(ColorManager *config, const QString &source_space, const QString &display, const QString &view, const QString &look, Direction dir)
{
  return std::make_shared<OpenGLColorProcessor>(config, source_space, display, view, look, dir);
}

OLIVE_NAMESPACE_EXIT
