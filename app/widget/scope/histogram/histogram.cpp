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

#include "common/qtutils.h"
#include "node/node.h"

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

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this,
    &HistogramScope::CleanUp, Qt::DirectConnection);
}

void HistogramScope::AssertAdditionalTextures()
{
  if (!texture_row_sums_.IsCreated()
        || texture_row_sums_.width() != width()
        || texture_row_sums_.height() != height()) {
    texture_row_sums_.Destroy();
    texture_row_sums_.Create(context(), VideoParams(width(),
      height(), managed_tex().format()));
  }
}

void HistogramScope::CleanUp()
{
  makeCurrent();

  pipeline_secondary_ = nullptr;
  texture_row_sums_.Destroy();

  doneCurrent();
}

QVariant HistogramScope::CreateShader()
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
  OpenGLShaderPtr shader = OpenGLShader::Create();

  shader->create();
  shader->addShaderFromSourceCode(QOpenGLShader::Vertex,
    Node::ReadFileAsString(":/shaders/rgbhistogram.vert"));
  shader->addShaderFromSourceCode(QOpenGLShader::Fragment,
    Node::ReadFileAsString(":/shaders/rgbhistogram_secondary.frag"));
  shader->link();

  return shader;
}

void HistogramScope::DrawScope()
{
  float histogram_scale = 0.80f;
  // This value is eyeballed for usefulness. Until we have a geometry
  // shader approach, it is impossible to normalize against a peak
  // sum of image values.
  float histogram_base = 2.5f;
  float histogram_power = 1.0f / histogram_base;

  pipeline()->bind();
  pipeline()->setUniformValue("ove_resolution", managed_tex().width(),
    managed_tex().height());
  pipeline()->setUniformValue("ove_viewport", width(), height());
  pipeline()->setUniformValue("histogram_scale", histogram_scale);
  pipeline()->release();

  AssertAdditionalTextures();

  framebuffer().Attach(&texture_row_sums_, true);
  framebuffer().Bind();

  managed_tex().Bind();

  OpenGLRenderFunctions::Blit(pipeline());

  managed_tex().Release();

  framebuffer().Release();
  framebuffer().Detach();

  pipeline_secondary_->bind();
  pipeline_secondary_->setUniformValue("ove_resolution",
    texture_row_sums_.width(), texture_row_sums_.height());
  pipeline_secondary_->setUniformValue("ove_viewport", width(), height());
  pipeline_secondary_->setUniformValue("histogram_scale", histogram_scale);
  pipeline_secondary_->setUniformValue("histogram_power", histogram_power);
  pipeline_secondary_->release();

  texture_row_sums_.Bind();

  OpenGLRenderFunctions::Blit(pipeline_secondary_);

  texture_row_sums_.Release();

  // Draw line overlays
  QPainter p(this);
  QFont font = p.font();
  font.setPixelSize(10);
  QFontMetrics font_metrics = QFontMetrics(font);
  QString label;
  std::vector<float> histogram_increments = {
    0.00,
    0.25,
    0.50,
    1.0
  };

  int histogram_steps = histogram_increments.size();
  QVector<QLine> histogram_lines(histogram_steps + 1);
  int font_x_offset = 0;
  int font_y_offset = font_metrics.capHeight() / 2.0f;

  p.setCompositionMode(QPainter::CompositionMode_Plus);

  p.setPen(QColor(0.0, 0.6 * 255.0, 0.0));
  p.setFont(font);

  float histogram_dim_x = ceil((width() - 1.0) * histogram_scale);
  float histogram_dim_y = ceil((height() - 1.0) * histogram_scale);
  float histogram_start_dim_x =
    ((width() - 1.0) - histogram_dim_x) / 2.0f;
  float histogram_start_dim_y =
    ((height() - 1.0) - histogram_dim_y) / 2.0f;
  float histogram_end_dim_x = (width() - 1.0) - histogram_start_dim_x;

  // for (int i=0; i <= histogram_steps; i++) {
  for(std::vector<float>::iterator it = histogram_increments.begin();
    it != histogram_increments.end(); it++) {
    histogram_lines[it - histogram_increments.begin()].setLine(
      histogram_start_dim_x,
      (histogram_dim_y * pow(1.0 - *it, histogram_base)) +
        histogram_start_dim_y,
      histogram_end_dim_x,
      (histogram_dim_y * pow(1.0 - *it, histogram_base)) +
        histogram_start_dim_y);
      label = QString::number(
        *it * 100, 'f', 1) + "%";
      font_x_offset = QFontMetricsWidth(font_metrics, label) + 4;

      p.drawText(
        histogram_start_dim_x - font_x_offset,
        (histogram_dim_y * pow(1.0 - *it, histogram_base)) +
          histogram_start_dim_y + font_y_offset, label);
  }
  p.drawLines(histogram_lines);
}

OLIVE_NAMESPACE_EXIT
