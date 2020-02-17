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

#include <QMessageBox>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

#include "render/backend/opengl/openglrenderfunctions.h"
#include "render/backend/opengl/openglshader.h"

#ifdef Q_OS_LINUX
bool ViewerGLWidget::nouveau_check_done_ = false;
#endif

ViewerGLWidget::ViewerGLWidget(QWidget *parent) :
  QOpenGLWidget(parent),
  texture_(0),
  ocio_lut_(0),
  color_manager_(nullptr)
{
  setContextMenuPolicy(Qt::CustomContextMenu);
}

ViewerGLWidget::~ViewerGLWidget()
{
  ContextCleanup();
}

void ViewerGLWidget::ConnectColorManager(ColorManager *color_manager)
{
  if (color_manager_ != nullptr) {
    disconnect(color_manager_, SIGNAL(ConfigChanged()), this, SLOT(RefreshColorPipeline()));
  }

  color_manager_ = color_manager;

  if (color_manager_ != nullptr) {
    connect(color_manager_, SIGNAL(ConfigChanged()), this, SLOT(RefreshColorPipeline()));
  }

  RefreshColorPipeline();
}

void ViewerGLWidget::DisconnectColorManager()
{
  ConnectColorManager(nullptr);
}

void ViewerGLWidget::SetMatrix(const QMatrix4x4 &mat)
{
  matrix_ = mat;
  update();
}

void ViewerGLWidget::SetOCIODisplay(const QString &display)
{
  ocio_display_ = display;
  SetupColorProcessor();
  update();
}

void ViewerGLWidget::SetOCIOView(const QString &view)
{
  ocio_view_ = view;
  SetupColorProcessor();
  update();
}

void ViewerGLWidget::SetOCIOLook(const QString &look)
{
  ocio_look_ = look;
  SetupColorProcessor();
  update();
}

ColorManager *ViewerGLWidget::color_manager() const
{
  return color_manager_;
}

const QString &ViewerGLWidget::ocio_display() const
{
  return ocio_display_;
}

const QString &ViewerGLWidget::ocio_view() const
{
  return ocio_view_;
}

const QString &ViewerGLWidget::ocio_look() const
{
  return ocio_look_;
}

void ViewerGLWidget::SetTexture(OpenGLTexturePtr tex)
{
  // Update the texture
  texture_ = tex;

  // Paint the texture
  update();
}

void ViewerGLWidget::SetOCIOParameters(const QString &display, const QString &view, const QString &look)
{
  ocio_display_ = display;
  ocio_view_ = view;
  ocio_look_ = look;
  SetupColorProcessor();
  update();
}

void ViewerGLWidget::initializeGL()
{
  SetupColorProcessor();

  connect(context(), SIGNAL(aboutToBeDestroyed()), this, SLOT(ContextCleanup()), Qt::DirectConnection);

#ifdef Q_OS_LINUX
  if (!nouveau_check_done_) {
    const char* vendor = reinterpret_cast<const char*>(context()->functions()->glGetString(GL_VENDOR));

    if (!strcmp(vendor, "nouveau")) {
      // Working with Qt widgets in this function segfaults, so we queue the messagebox for later
      QMetaObject::invokeMethod(this,
                                "ShowNouveauWarning",
                                Qt::QueuedConnection);
    }

    nouveau_check_done_ = true;
  }
#endif
}

void ViewerGLWidget::paintGL()
{
  // Get functions attached to this context (they will already be initialized)
  QOpenGLFunctions* f = context()->functions();

  // Clear background to empty
  f->glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  f->glClear(GL_COLOR_BUFFER_BIT);

  // We only draw if we have a pipeline
  if (!pipeline_ || !texture_) {
    return;
  }

  // Bind retrieved texture
  f->glBindTexture(GL_TEXTURE_2D, texture_->texture());

  // Blit using the pipeline retrieved in initializeGL()
  OpenGLRenderFunctions::OCIOBlit(pipeline_, ocio_lut_, true, matrix_);

  // Release retrieved texture
  f->glBindTexture(GL_TEXTURE_2D, 0);
}

void ViewerGLWidget::RefreshColorPipeline()
{
  if (!color_manager_) {
    color_service_ = nullptr;
    pipeline_ = nullptr;
    return;
  }

  QStringList displays = color_manager_->ListAvailableDisplays();
  if (!displays.contains(ocio_display_)) {
    ocio_display_ = color_manager_->GetDefaultDisplay();
  }

  QStringList views = color_manager_->ListAvailableViews(ocio_display_);
  if (!views.contains(ocio_view_)) {
    ocio_view_ = color_manager_->GetDefaultView(ocio_display_);
  }

  QStringList looks = color_manager_->ListAvailableLooks();
  if (!looks.contains(ocio_look_)) {
    ocio_look_.clear();
  }

  SetupColorProcessor();
  update();
}

#ifdef Q_OS_LINUX
void ViewerGLWidget::ShowNouveauWarning()
{
  QMessageBox::warning(this,
                       tr("Driver Warning"),
                       tr("Olive has detected your system is using the Nouveau graphics driver.\n\nThis driver is "
                          "known to have stability and performance issues with Olive. It is highly recommended "
                          "you install the proprietary NVIDIA driver before continuing to use Olive."),
                       QMessageBox::Ok);
}
#endif

void ViewerGLWidget::SetupColorProcessor()
{
  if (!context()) {
    return;
  }

  ClearOCIOLutTexture();

  if (color_manager_) {
    // (Re)create color processor

    try {
      ColorProcessorPtr new_service = ColorProcessor::Create(color_manager_->GetConfig(),
                                                             OCIO::ROLE_SCENE_LINEAR,
                                                             ocio_display_,
                                                             ocio_view_,
                                                             ocio_look_);

      color_service_ = ColorProcessor::Create(color_manager_->GetConfig(),
                                              OCIO::ROLE_SCENE_LINEAR,
                                              ocio_display_,
                                              ocio_view_,
                                              ocio_look_);

      // (Re)create pipeline from color processor
      pipeline_ = OpenGLShader::CreateOCIO(context(),
                                           ocio_lut_,
                                           color_service_->GetProcessor(),
                                           true);
    } catch (OCIO::Exception& e) {
      QMessageBox::critical(this,
                            tr("OpenColorIO Error"),
                            tr("Failed to set color configuration: %1").arg(e.what()),
                            QMessageBox::Ok);
    }

  } else {
    color_service_ = nullptr;
    pipeline_ = nullptr;
  }
}

void ViewerGLWidget::ClearOCIOLutTexture()
{
  if (ocio_lut_ > 0) {
    context()->functions()->glDeleteTextures(1, &ocio_lut_);
    ocio_lut_ = 0;
  }
}

void ViewerGLWidget::ContextCleanup()
{
  makeCurrent();

  ClearOCIOLutTexture();

  pipeline_ = nullptr;

  doneCurrent();
}
