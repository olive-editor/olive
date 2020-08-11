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

#include "scopebase.h"

#include "render/backend/opengl/openglrenderfunctions.h"

OLIVE_NAMESPACE_ENTER

ScopeBase::ScopeBase(QWidget* parent) :
  ManagedDisplayWidget(parent),
  buffer_(nullptr)
{
  EnableDefaultContextMenu();
  control_ui_ = new QWidget();
}

ScopeBase::~ScopeBase()
{
  CleanUp();

  if (context()) {
    disconnect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ScopeBase::CleanUp);
  }
}

QWidget* ScopeBase::ControlUI()
{
  return control_ui_;
}

void ScopeBase::SetBuffer(Frame *frame)
{
  buffer_ = frame;

  UploadTextureFromBuffer();
}

void ScopeBase::showEvent(QShowEvent* e)
{
  ManagedDisplayWidget::showEvent(e);

  UploadTextureFromBuffer();
}

OpenGLShaderPtr ScopeBase::CreateShader()
{
  return OpenGLShader::CreateDefault();
}

void ScopeBase::DrawScope()
{
  managed_tex().Bind();

  OpenGLRenderFunctions::Blit(pipeline());

  managed_tex().Release();
}

void ScopeBase::UploadTextureFromBuffer()
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
      managed_tex_.Create(context(), buffer_->video_params());
    } else {
      texture_.Upload(buffer_);
    }

    doneCurrent();
  }

  update();
}

void ScopeBase::CleanUp()
{
  makeCurrent();

  pipeline_ = nullptr;
  texture_.Destroy();
  managed_tex_.Destroy();
  framebuffer_.Destroy();

  doneCurrent();
}

void ScopeBase::initializeGL()
{
  ManagedDisplayWidget::initializeGL();

  pipeline_ = CreateShader();

  framebuffer_.Create(context());

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ScopeBase::CleanUp, Qt::DirectConnection);

  UploadTextureFromBuffer();
}

void ScopeBase::paintGL()
{
  QOpenGLFunctions* f = context()->functions();

  f->glClearColor(0, 0, 0, 0);
  f->glClear(GL_COLOR_BUFFER_BIT);

  if (buffer_ && pipeline() && texture_.IsCreated()) {
    // Convert reference frame to display space
    framebuffer_.Attach(&managed_tex_);
    framebuffer_.Bind();

    texture_.Bind();

    f->glViewport(0, 0, texture_.width(), texture_.height());

    color_service()->ProcessOpenGL();

    texture_.Release();

    framebuffer_.Release();
    framebuffer_.Detach();

    f->glViewport(0, 0, width(), height());

    DrawScope();
  }
}

void ScopeBase::SetupControlUI()
{
  control_ui_ = new QWidget();
}

OLIVE_NAMESPACE_EXIT
