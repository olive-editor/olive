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

#include "histogram.h"

#include <QPainter>
#include <QtMath>

#include "node/node.h"
#include "render/backend/opengl/openglrenderfunctions.h"

OLIVE_NAMESPACE_ENTER

HistogramScope::HistogramScope(QWidget* parent) :
  ScopeBase(parent)
{
}

HistogramScope::~HistogramScope()
{
  CleanUp();

  if (context()) {
    disconnect(context(), &QOpenGLContext::aboutToBeDestroyed, this,
      &HistogramScope::CleanUp);
  }
}

void HistogramScope::initializeGL()
{
  ScopeBase::initializeGL();

  pipeline_secondary_ = CreateSecondaryShader();

  // framebuffer_.Create(context());

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this,
    &HistogramScope::CleanUp, Qt::DirectConnection);

    // doneCurrent();
  AssertAdditionalTextures();
  // UploadTextureFromBuffer();
}

void HistogramScope::AssertAdditionalTextures()
{
  if (!texture_row_sums_.IsCreated()
        || texture_row_sums_.width() != width()
        || texture_row_sums_.height() != height()) {
    texture_row_sums_.Destroy();
    texture_row_sums_.Create(context(), VideoRenderingParams(width(),
      height(), managed_tex_.format()));
  }

  if (!texture_histogram_.IsCreated()
        || texture_histogram_.width() != width()
        || texture_histogram_.height() != height()) {
    texture_histogram_.Destroy();
    texture_histogram_.Create(context(), VideoRenderingParams(width(),
      height(), managed_tex_.format()));
  }
}

void HistogramScope::CleanUp()
{
  makeCurrent();

  pipeline_secondary_ = nullptr;
  texture_row_sums_.Destroy();
  texture_histogram_.Destroy();

  doneCurrent();
}

OpenGLShaderPtr HistogramScope::CreateShader()
{
  OpenGLShaderPtr pipeline = OpenGLShader::Create();

  pipeline->create();
  pipeline->addShaderFromSourceCode(QOpenGLShader::Vertex,
    OpenGLShader::CodeDefaultVertex());
  pipeline->addShaderFromSourceCode(QOpenGLShader::Fragment,
    Node::ReadFileAsString(":/shaders/rgbhistogram.frag"));
  pipeline->link();

  return pipeline;
}

OpenGLShaderPtr HistogramScope::CreateSecondaryShader()
{
  OpenGLShaderPtr pipeline_secondary_ = OpenGLShader::Create();

  pipeline_secondary_->create();
  pipeline_secondary_->addShaderFromSourceCode(QOpenGLShader::Vertex,
    OpenGLShader::CodeDefaultVertex());
  pipeline_secondary_->addShaderFromSourceCode(QOpenGLShader::Fragment,
    Node::ReadFileAsString(":/shaders/rgbhistogram_secondary.frag"));
  pipeline_secondary_->link();

  return pipeline_secondary_;
}

void HistogramScope::DrawScope()
{
  float histogram_scale = 0.80f;
  float histogram_dim_x = width() * histogram_scale;
  float histogram_dim_y = height() * histogram_scale;
  float histogram_start_dim_x = (width() - histogram_dim_x) / 2.0f;
  float histogram_start_dim_y = (height() - histogram_dim_y) / 2.0f;
  float histogram_end_dim_x = width() - histogram_start_dim_x;
  float histogram_end_dim_y = height() - histogram_start_dim_y;

  pipeline()->bind();
  pipeline()->setUniformValue("ove_resolution", managed_tex().width(),
    managed_tex().height());
  pipeline()->setUniformValue("ove_viewport", width(), height());
  pipeline()->release();

  AssertAdditionalTextures();
  framebuffer_.Attach(&texture_row_sums_);
  framebuffer_.Bind();

  managed_tex().Bind();

  OpenGLRenderFunctions::Blit(pipeline());

  managed_tex().Release();

  framebuffer_.Release();
  framebuffer_.Detach();

  pipeline_secondary_->bind();
  pipeline_secondary_->setUniformValue("ove_resolution",
    texture_row_sums_.width(), texture_row_sums_.height());
  pipeline_secondary_->setUniformValue("ove_viewport", width(), height());
  pipeline_secondary_->setUniformValue(
    "histogram_region",
    histogram_start_dim_x, histogram_start_dim_y,
    histogram_end_dim_x, histogram_end_dim_y);
  pipeline_secondary_->release();

  texture_row_sums_.Bind();

  OpenGLRenderFunctions::Blit(pipeline_secondary_);

  texture_row_sums_.Release();
}

OLIVE_NAMESPACE_EXIT
