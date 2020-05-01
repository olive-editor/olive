
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

#include "waveform.h"

#include <QPainter>
#include <QtMath>
#include <QDebug>

#include "common/qtutils.h"
#include "node/node.h"
#include "render/backend/opengl/openglrenderfunctions.h"

OLIVE_NAMESPACE_ENTER

WaveformScope::WaveformScope(QWidget* parent) :
  ManagedDisplayWidget(parent),
  buffer_(nullptr)
{
  EnableDefaultContextMenu();
}

WaveformScope::~WaveformScope()
{
  CleanUp();

  if (context()) {
    disconnect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &WaveformScope::CleanUp);
  }
}

void WaveformScope::SetBuffer(Frame *frame)
{
  buffer_ = frame;

  UploadTextureFromBuffer();
}

void WaveformScope::showEvent(QShowEvent* e)
{
  ManagedDisplayWidget::showEvent(e);

  UploadTextureFromBuffer();
}

void WaveformScope::initializeGL()
{
  ManagedDisplayWidget::initializeGL();

  pipeline_ = OpenGLShader::Create();
  pipeline_->create();
  pipeline_->addShaderFromSourceCode(QOpenGLShader::Vertex, OpenGLShader::CodeDefaultVertex());
  pipeline_->addShaderFromSourceCode(QOpenGLShader::Fragment, Node::ReadFileAsString(":/shaders/rgbwaveform.frag"));
  pipeline_->link();

  framebuffer_.Create(context());

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &WaveformScope::CleanUp, Qt::DirectConnection);

  UploadTextureFromBuffer();
}

void WaveformScope::paintGL()
{
  QOpenGLFunctions* f = context()->functions();

  f->glClearColor(0, 0, 0, 0);
  f->glClear(GL_COLOR_BUFFER_BIT);

  float waveform_scale = 0.80f;
  float waveform_dim_x = width() * waveform_scale;
  float waveform_dim_y = height() * waveform_scale;
  float waveform_start_dim_x = (width() - waveform_dim_x) / 2.0f;
  float waveform_start_dim_y = (height() - waveform_dim_y) / 2.0f;
  float waveform_end_dim_x = width() - waveform_start_dim_x;
  float waveform_end_dim_y = height() - waveform_start_dim_y;

  if (buffer_ && pipeline_ && texture_.IsCreated()) {
    // Convert reference frame to display space
    framebuffer_.Attach(&managed_tex_);
    framebuffer_.Bind();

    texture_.Bind();

    f->glViewport(0, 0, texture_.width(), texture_.height());

    color_service()->ProcessOpenGL();

    texture_.Release();

    framebuffer_.Release();
    framebuffer_.Detach();

    // Draw waveform through shader
    pipeline_->bind();
    pipeline_->setUniformValue("ove_resolution", texture_.width(), texture_.height());
    pipeline_->setUniformValue("ove_viewport", width(), height());
    GLfloat luma[3] = {0.0, 0.0, 0.0};
    color_manager()->GetDefaultLumaCoefs(luma);
    pipeline_->setUniformValue("luma_coeffs", luma[0], luma[1], luma[2]);

    // Scale of the waveform relative to the viewport surface.
    pipeline_->setUniformValue("waveform_scale", waveform_scale);
    pipeline_->setUniformValue(
      "waveform_dims", waveform_dim_x, waveform_dim_y);

    pipeline_->setUniformValue(
      "waveform_region",
      waveform_start_dim_x, waveform_start_dim_y,
      waveform_end_dim_x, waveform_end_dim_y);

    float waveform_start_uv_x = waveform_start_dim_x / width();
    float waveform_start_uv_y = waveform_start_dim_y / height();
    float waveform_end_uv_x = waveform_end_dim_x / width();
    float waveform_end_uv_y = waveform_end_dim_y / height();
    pipeline_->setUniformValue(
      "waveform_uv",
      waveform_start_uv_x, waveform_start_uv_y,
      waveform_end_uv_x, waveform_end_uv_y);

    pipeline_->release();

    f->glViewport(0, 0, width(), height());

    managed_tex_.Bind();

    OpenGLRenderFunctions::Blit(pipeline_);

    managed_tex_.Release();
  }

  // Draw line overlays
  QPainter p(this);
  QFontMetrics font_metrics = QFontMetrics(QFont());
  QString label;
  float ire_increment = 0.1f;
  float ire_steps = int(1.0 / ire_increment);
  QVector<QLine> ire_lines(ire_steps + 1);
  int font_x_offset = 0;
  int font_y_offset = font_metrics.capHeight() / 2.0f;

  p.setCompositionMode(QPainter::CompositionMode_Plus);

  p.setPen(QColor(0.0, 0.6 * 255.0, 0.0));
  p.setFont(QFont());

  for (int i=0; i <= ire_steps; i++) {
    ire_lines[i].setLine(
      waveform_start_dim_x,
      (waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y,
      waveform_end_dim_x,
      (waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y);
      label = QString::number(1.0 - (i * ire_increment), 'f', 1);
      font_x_offset = QFontMetricsWidth(font_metrics, label) + 4;

      p.drawText(
        waveform_start_dim_x - font_x_offset,
        (waveform_dim_y * (i * ire_increment)) + waveform_start_dim_y + font_y_offset,
        label);
  }
  p.drawLines(ire_lines);
}

void WaveformScope::UploadTextureFromBuffer()
{
  if (!isVisible()) {
    return;
  }

  if (buffer_) {
    makeCurrent();

    if (!texture_.IsCreated()
        || texture_.width() != buffer_->width()
        || texture_.height() != buffer_->height()
        || texture_.format() != buffer_->format()) {
      texture_.Destroy();
      managed_tex_.Destroy();

      texture_.Create(context(), buffer_);
      managed_tex_.Create(context(), buffer_->width(), buffer_->height(), buffer_->format());
    } else {
      texture_.Upload(buffer_);
    }

    doneCurrent();
  }

  update();
}

void WaveformScope::CleanUp()
{
  makeCurrent();

  pipeline_ = nullptr;
  texture_.Destroy();
  managed_tex_.Destroy();
  framebuffer_.Destroy();

  doneCurrent();
}

OLIVE_NAMESPACE_EXIT
