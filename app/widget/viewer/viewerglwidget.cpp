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

#include "viewerglwidget.h"

#include <QMenu>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

#include "render/backend/opengl/functions.h"
#include "render/backend/opengl/openglshader.h"

ViewerGLWidget::ViewerGLWidget(QWidget *parent) :
  QOpenGLWidget(parent),
  texture_(0),
  ocio_lut_(0)
{
  connect(ColorManager::instance(), SIGNAL(ConfigChanged()), this, SLOT(ColorConfigChangedSlot()));

  RefreshColorSettings();

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));
}

ViewerGLWidget::~ViewerGLWidget()
{
  ContextCleanup();
}

void ViewerGLWidget::SetTexture(GLuint tex)
{
  // Update the texture
  texture_ = tex;

  // Paint the texture
  update();
}

void ViewerGLWidget::initializeGL()
{
  SetupPipeline();

  connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(ContextCleanup()), Qt::DirectConnection);
}

void ViewerGLWidget::paintGL()
{
  // Get functions attached to this context (they will already be initialized)
  QOpenGLFunctions* f = context()->functions();

  // Clear background to empty
  f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  f->glClear(GL_COLOR_BUFFER_BIT);

  // Check if we have a texture to draw
  if (texture_ > 0) {
    // Bind retrieved texture
    f->glBindTexture(GL_TEXTURE_2D, texture_);

    // Blit using the pipeline retrieved in initializeGL()
    olive::gl::OCIOBlit(pipeline_, ocio_lut_, true);

    // Release retrieved texture
    f->glBindTexture(GL_TEXTURE_2D, 0);
  }
}

void ViewerGLWidget::SetupPipeline()
{
  // Re-retrieve pipeline pertaining to this context
  pipeline_ = OpenGLShader::CreateOCIO(context(),
                                                   ocio_lut_,
                                                   color_service_->GetProcessor(),
                                                   true);
}

void ViewerGLWidget::RefreshColorSettings()
{
  // FIXME: Should probably check first whether the new config has the existing settings

  ocio_display_ = ColorManager::GetDefaultDisplay();
  ocio_view_ = ColorManager::GetDefaultView(ocio_display_);
  ocio_look_.clear();

  SetupColorProcessor();
}

void ViewerGLWidget::SetupColorProcessor()
{
  color_service_ = ColorProcessor::Create(OCIO::ROLE_SCENE_LINEAR, ocio_display_, ocio_view_, ocio_look_);
}

void ViewerGLWidget::ContextCleanup()
{
  makeCurrent();

  if (ocio_lut_ != 0) {
    context()->functions()->glDeleteTextures(1, &ocio_lut_);
    ocio_lut_ = 0;
  }

  pipeline_ = nullptr;

  doneCurrent();
}

void ViewerGLWidget::ShowContextMenu(const QPoint &pos)
{
  QMenu menu;

  QStringList displays = ColorManager::ListAvailableDisplays();
  QMenu* ocio_display_menu = menu.addMenu(tr("OCIO Display"));
  foreach (QString d, displays) {
    QAction* action = ocio_display_menu->addAction(d);
    action->setChecked(ocio_display_ == d);
  }

  QStringList views = ColorManager::ListAvailableViews(ocio_display_);
  QMenu* ocio_view_menu = menu.addMenu(tr("OCIO View"));
  foreach (QString v, views) {
    QAction* action = ocio_view_menu->addAction(v);
    action->setChecked(ocio_view_ == v);
  }

  QStringList looks = ColorManager::ListAvailableLooks();
  QMenu* ocio_look_menu = menu.addMenu(tr("OCIO Look"));
  foreach (QString l, looks) {
    QAction* action = ocio_look_menu->addAction(l);
    action->setChecked(ocio_look_ == l);
  }

  menu.exec(mapToGlobal(pos));
}

void ViewerGLWidget::ColorConfigChangedSlot()
{
  RefreshColorSettings();

  if (pipeline_ != nullptr) {
    SetupPipeline();
  }
}
