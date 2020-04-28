
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

  if (!pipeline_ || !texture_.IsCreated()) {
    return;
  }

  {
    // Convert reference frame to display space
    framebuffer_.Attach(&managed_tex_);
    framebuffer_.Bind();

    texture_.Bind();

    f->glViewport(0, 0, texture_.width(), texture_.height());

    color_service()->ProcessOpenGL();

    QVector<QLine> ire_lines(6);

    for (int i=1; i <= 5; i++) {
      ire_lines[i].setLine(
        0.0, height() * (i * 0.20), width(), height() *
        (i * 0.20));
    }
    ire_lines[0].setLine(0.0, 0.0, width(), 0.0);
    ire_lines[5].setLine(0.0, height() - 1.0, width(), height() - 1.0);

    QPainter p(this);
    p.setCompositionMode(QPainter::CompositionMode_Plus);

    p.setPen(Qt::red);
    p.drawLines(ire_lines);

    texture_.Release();

    framebuffer_.Release();
    framebuffer_.Detach();
  }

  {
    // Draw waveform through shader
    pipeline_->bind();
    pipeline_->setUniformValue("ove_resolution", texture_.width(), texture_.height());
    pipeline_->setUniformValue("ove_viewport", width(), height());

    GLfloat luma[3] = {0.0, 0.0, 0.0};

    color_manager()->GetDefaultLumaCoefs(luma);
    // qDebug() << "LUMA" << luma[0] << " " << luma[1] << " " << luma[2];
    pipeline_->setUniformValue("luma_coeffs", luma[0], luma[1], luma[2]);

    pipeline_->release();

    f->glViewport(0, 0, width(), height());

    managed_tex_.Bind();

    OpenGLRenderFunctions::Blit(pipeline_);

    managed_tex_.Release();
  }
}

void WaveformScope::UploadTextureFromBuffer()
{
  if (!buffer_ || !isVisible()) {
    return;
  }

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
