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

#include <OpenImageIO/imagebuf.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QPainter>

#include "common/define.h"
#include "render/backend/opengl/openglrenderfunctions.h"
#include "render/backend/opengl/openglshader.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

#ifdef Q_OS_LINUX
bool ViewerGLWidget::nouveau_check_done_ = false;
#endif

ViewerGLWidget::ViewerGLWidget(QWidget *parent) :
  ManagedDisplayWidget(parent),
  managed_copy_pipeline_(nullptr),
  has_image_(false),
  signal_cursor_color_(false),
  enable_display_referred_signal_(false)
{
}

ViewerGLWidget::~ViewerGLWidget()
{
  ContextCleanup();
}

void ViewerGLWidget::SetMatrix(const QMatrix4x4 &mat)
{
  matrix_ = mat;
  update();
}

void ViewerGLWidget::SetImage(const QString &fn)
{
  has_image_ = false;

  if (!fn.isEmpty() && QFileInfo::exists(fn)) {
    auto input = OIIO::ImageInput::open(fn.toStdString());

    if (input) {

      PixelFormat::Format image_format = PixelFormat::OIIOFormatToOliveFormat(input->spec().format,
                                                                              input->spec().nchannels == kRGBAChannels);

      // Ensure the following texture operations are done in our context (in case we're in a separate window for instance)
      makeCurrent();

      if (!texture_.IsCreated()
          || texture_.width() != input->spec().width
          || texture_.height() != input->spec().height
          || texture_.format() != image_format) {
        load_buffer_.destroy();
        texture_.Destroy();

        load_buffer_.set_video_params(VideoRenderingParams(input->spec().width, input->spec().height, image_format));
        load_buffer_.allocate();

        texture_.Create(context(), input->spec().width, input->spec().height, image_format);
      }

      input->read_image(input->spec().format, load_buffer_.data(), OIIO::AutoStride, load_buffer_.linesize_bytes());
      input->close();

      emit LoadedBuffer(&load_buffer_);

      texture_.Upload(load_buffer_.data(), load_buffer_.linesize_pixels());

      emit LoadedTexture(&texture_);

      doneCurrent();

      has_image_ = true;

#if OIIO_VERSION < 10903
      OIIO::ImageInput::destroy(input);
#endif

    } else {
      qWarning() << "OIIO Error:" << OIIO::geterror().c_str();
    }
  }

  update();

  if (has_image_) {
    emit LoadedBuffer(&load_buffer_);
  } else {
    emit LoadedBuffer(nullptr);
  }
}

void ViewerGLWidget::SetSignalCursorColorEnabled(bool e)
{
  signal_cursor_color_ = e;
  setMouseTracking(e);
}

void ViewerGLWidget::SetImageFromLoadBuffer(Frame *in_buffer)
{
  has_image_ = in_buffer;

  if (has_image_) {
    makeCurrent();

    if (!texture_.IsCreated()
        || texture_.width() != in_buffer->width()
        || texture_.height() != in_buffer->height()
        || texture_.format() != in_buffer->format()) {
      texture_.Create(context(), in_buffer->width(), in_buffer->height(), in_buffer->format(), in_buffer->data(), load_buffer_.linesize_pixels());
    } else {
      texture_.Upload(in_buffer->data(), load_buffer_.linesize_pixels());
    }

    doneCurrent();
  }

  update();
}

void ViewerGLWidget::SetEmitDrewManagedTextureEnabled(bool e)
{
  enable_display_referred_signal_ = e;

  if (!enable_display_referred_signal_) {
    // Destroy the texture now
    managed_texture_.Destroy();
    managed_copy_pipeline_ = nullptr;
    framebuffer_.Destroy();
  }
}

void ViewerGLWidget::ConnectSibling(ViewerGLWidget *sibling)
{
  connect(this, &ViewerGLWidget::LoadedBuffer, sibling, &ViewerGLWidget::SetImageFromLoadBuffer, Qt::QueuedConnection);
  sibling->SetImageFromLoadBuffer(&load_buffer_);
}

const ViewerSafeMarginInfo &ViewerGLWidget::GetSafeMargin() const
{
  return safe_margin_;
}

void ViewerGLWidget::SetSafeMargins(const ViewerSafeMarginInfo &safe_margin)
{
  safe_margin_ = safe_margin;

  update();
}

void ViewerGLWidget::mousePressEvent(QMouseEvent *event)
{
  QOpenGLWidget::mousePressEvent(event);

  emit DragStarted();
}

void ViewerGLWidget::mouseMoveEvent(QMouseEvent *event)
{
  QOpenGLWidget::mouseMoveEvent(event);

  if (signal_cursor_color_) {
    Color reference, display;

    if (has_image_) {
      QVector3D pixel_pos(static_cast<float>(event->x()) / static_cast<float>(width()) * 2.0f - 1.0f,
                          static_cast<float>(event->y()) / static_cast<float>(height()) * 2.0f - 1.0f,
                          0);

      pixel_pos = pixel_pos * matrix_.inverted();

      int frame_x = qRound((pixel_pos.x() + 1.0f) * 0.5f * load_buffer_.width());
      int frame_y = qRound((pixel_pos.y() + 1.0f) * 0.5f * load_buffer_.height());

      reference = load_buffer_.get_pixel(frame_x, frame_y);
      display = color_service()->ConvertColor(reference);
    }

    emit CursorColor(reference, display);
  }
}

void ViewerGLWidget::initializeGL()
{
  ManagedDisplayWidget::initializeGL();

  connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &ViewerGLWidget::ContextCleanup, Qt::DirectConnection);

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
  if (has_image_ && color_service() && texture_.IsCreated()) {

    // If we're distributing our display-referred final buffer, we'll have to make a copy of it
    if (enable_display_referred_signal_) {

      if (!managed_texture_.IsCreated()
          || managed_texture_.width() != texture_.width()
          || managed_texture_.height() != texture_.height()
          || managed_texture_.format() != texture_.format()) {
        managed_texture_.Destroy();

        managed_texture_.Create(context(), texture_.width(), texture_.height(), texture_.format());
      }

      if (!managed_copy_pipeline_) {
        managed_copy_pipeline_ = OpenGLShader::CreateDefault();
      }

      if (!framebuffer_.IsCreated()) {
        framebuffer_.Create(context());
      }

      framebuffer_.Attach(&managed_texture_);
      framebuffer_.Bind();

      f->glViewport(0, 0, managed_texture_.width(), managed_texture_.height());

    }

    // Bind retrieved texture
    f->glBindTexture(GL_TEXTURE_2D, texture_.texture());

    // Blit using the color service
    color_service()->ProcessOpenGL(true, matrix_);

    // Release retrieved texture
    f->glBindTexture(GL_TEXTURE_2D, 0);

    if (enable_display_referred_signal_) {

      framebuffer_.Release();
      framebuffer_.Detach();

      emit DrewManagedTexture(&managed_texture_);

      // Bind retrieved texture
      managed_texture_.Bind();

      f->glViewport(0, 0, width(), height());

      OpenGLRenderFunctions::Blit(managed_copy_pipeline_);

      // Bind retrieved texture
      managed_texture_.Release();

    }
  }

  // Draw action/title safe areas
  if (safe_margin_.is_enabled()) {
    QPainter p(this);
    p.setPen(Qt::lightGray);
    p.setBrush(Qt::NoBrush);

    int x = 0, y = 0, w = width(), h = height();

    if (safe_margin_.custom_ratio()) {
      double widget_ar = static_cast<double>(width()) / static_cast<double>(height());

      if (widget_ar > safe_margin_.ratio()) {
        // Widget is wider than margins
        w = h * safe_margin_.ratio();
        x = width() / 2 - w / 2;
      } else {
        h = w / safe_margin_.ratio();
        y = height() / 2 - h / 2;
      }
    }

    p.drawRect(w / 20 + x, h / 20 + y, w / 10 * 9, h / 10 * 9);
    p.drawRect(w / 10 + x, h / 10 + y, w / 10 * 8, h / 10 * 8);

    int cross = qMin(w, h) / 32;

    QLine lines[] = {QLine(rect().center().x() - cross, rect().center().y(), rect().center().x() + cross, rect().center().y()),
                    QLine(rect().center().x(), rect().center().y() - cross, rect().center().x(), rect().center().y() + cross)};

    p.drawLines(lines, 2);
  }
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

void ViewerGLWidget::ContextCleanup()
{
  makeCurrent();

  managed_copy_pipeline_ = nullptr;
  texture_.Destroy();
  managed_texture_.Destroy();
  framebuffer_.Destroy();

  doneCurrent();
}

OLIVE_NAMESPACE_EXIT
