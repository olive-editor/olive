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

#include "viewerdisplay.h"

#include <OpenImageIO/imagebuf.h>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QPainter>

#include "common/define.h"
#include "common/functiontimer.h"
#include "gizmotraverser.h"
#include "render/pixelformat.h"
#include "core.h"

OLIVE_NAMESPACE_ENTER

ViewerDisplayWidget::ViewerDisplayWidget(QWidget *parent) :
  ManagedDisplayWidget(parent),
  signal_cursor_color_(false),
  gizmos_(nullptr),
  gizmo_click_(false),
  last_loaded_buffer_(nullptr),
  hand_dragging_(false),
  deinterlace_(false)
{
  connect(Core::instance(), &Core::ToolChanged, this, &ViewerDisplayWidget::UpdateCursor);

  // Initializes cursor based on tool
  UpdateCursor();
}

ViewerDisplayWidget::~ViewerDisplayWidget()
{
  OnDestroy();
}

void ViewerDisplayWidget::SetMatrixTranslate(const QMatrix4x4 &mat)
{
  translate_matrix_ = mat;

  UpdateMatrix();
}

void ViewerDisplayWidget::SetMatrixZoom(const QMatrix4x4 &mat)
{
  scale_matrix_ = mat;

  UpdateMatrix();
}

void ViewerDisplayWidget::UpdateCursor()
{
  if (Core::instance()->tool() == Tool::kHand) {
    setCursor(Qt::OpenHandCursor);
  } else {
    unsetCursor();
  }
}

QMatrix4x4 ViewerDisplayWidget::GetCompleteMatrixFlippedYTranslation()
{
  QMatrix4x4 mat = combined_matrix_;

  mat.scale(1, -1, 1);

  return mat;
}

void ViewerDisplayWidget::SetSignalCursorColorEnabled(bool e)
{
  signal_cursor_color_ = e;
  setMouseTracking(e);
}

void ViewerDisplayWidget::SetImage(FramePtr in_buffer)
{
  last_loaded_buffer_ = in_buffer;

  if (last_loaded_buffer_) {
    makeCurrent();

    if (!texture_
        || texture_->width() != in_buffer->width()
        || texture_->height() != in_buffer->height()
        || texture_->format() != in_buffer->format()) {
      texture_ = renderer()->CreateTexture(in_buffer->video_params(), in_buffer->data(), in_buffer->linesize_pixels());
    } else {
      texture_->Upload(in_buffer->data(), in_buffer->linesize_pixels());
    }

    doneCurrent();
  }

  update();
}

void ViewerDisplayWidget::SetDeinterlacing(bool e)
{
  deinterlace_ = e;
  update();
}

const ViewerSafeMarginInfo &ViewerDisplayWidget::GetSafeMargin() const
{
  return safe_margin_;
}

void ViewerDisplayWidget::SetSafeMargins(const ViewerSafeMarginInfo &safe_margin)
{
  if (safe_margin_ != safe_margin) {
    safe_margin_ = safe_margin;

    update();
  }
}

void ViewerDisplayWidget::SetGizmos(Node *node)
{
  if (gizmos_ != node) {
    gizmos_ = node;

    update();
  }
}

void ViewerDisplayWidget::SetVideoParams(const VideoParams &params)
{
  gizmo_params_ = params;

  if (gizmos_) {
    update();
  }
}

void ViewerDisplayWidget::SetTime(const rational &time)
{
  time_ = time;

  if (gizmos_) {
    update();
  }
}

FramePtr ViewerDisplayWidget::last_loaded_buffer() const
{
  return last_loaded_buffer_;
}

QPoint ViewerDisplayWidget::TransformViewerSpaceToBufferSpace(QPoint pos)
{
  /*
  * Inversion will only fail if the viewer has been scaled by 0 in any direction
  * which I think should never happen.
  */
  return pos * GenerateWorldTransform().inverted();
}

void ViewerDisplayWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton && gizmos_
      && gizmos_->GizmoPress(gizmo_db_, TransformViewerSpaceToBufferSpace(event->pos()),
                             QVector2D(GetTexturePosition(size())), size())) {

    // Handle gizmo click
    gizmo_click_ = true;
    gizmo_drag_time_ = GetGizmoTime();

  } else if (IsHandDrag(event)) {

    // Handle hand drag
    hand_last_drag_pos_ = event->pos();
    hand_dragging_ = true;
    emit HandDragStarted();
    setCursor(Qt::ClosedHandCursor);

  } else {

    if (event->button() == Qt::LeftButton) {
      // Handle standard drag
      emit DragStarted();
    }

    ManagedDisplayWidget::mousePressEvent(event);

  }
}

void ViewerDisplayWidget::mouseMoveEvent(QMouseEvent *event)
{
  // Do this no matter what, emits signal to any pixel samplers
  if (signal_cursor_color_) {
    Color reference, display;

    if (last_loaded_buffer_) {
      QVector3D pixel_pos(static_cast<float>(event->x()) / static_cast<float>(width()) * 2.0f - 1.0f,
                          static_cast<float>(event->y()) / static_cast<float>(height()) * 2.0f - 1.0f,
                          0);

      pixel_pos = GenerateWorldTransform().inverted() * pixel_pos;

      int frame_x = qRound((pixel_pos.x() + 1.0f) * 0.5f * last_loaded_buffer_->width());
      int frame_y = qRound((pixel_pos.y() + 1.0f) * 0.5f * last_loaded_buffer_->height());

      reference = last_loaded_buffer_->get_pixel(frame_x, frame_y);
      display = color_service()->ConvertColor(reference);
    }

    emit CursorColor(reference, display);
  }

  // Handle hand dragging
  if (hand_dragging_) {

    // Emit movement
    emit HandDragMoved(event->x() - hand_last_drag_pos_.x(),
                       event->y() - hand_last_drag_pos_.y());

    hand_last_drag_pos_ = event->pos();

  } else if (gizmo_click_) {

    // Signal movement
    gizmos_->GizmoMove(TransformViewerSpaceToBufferSpace(event->pos()),
                       QVector2D(GetTexturePosition(size())), gizmo_drag_time_);
    update();

  } else {

    // Default behavior
    ManagedDisplayWidget::mouseMoveEvent(event);

  }
}

void ViewerDisplayWidget::mouseReleaseEvent(QMouseEvent *event)
{
  if (hand_dragging_) {

    // Handle hand drag
    emit HandDragEnded();
    hand_dragging_ = false;
    UpdateCursor();

  } else if (gizmo_click_) {

    // Handle gizmo
    gizmos_->GizmoRelease();
    gizmo_click_ = false;

  } else {

    // Default behavior
    ManagedDisplayWidget::mouseReleaseEvent(event);

  }
}

void ViewerDisplayWidget::OnInit()
{
  ManagedDisplayWidget::OnInit();
}

void ViewerDisplayWidget::OnPaint()
{
  // Clear background to empty
  renderer()->ClearDestination();

  // We only draw if we have a pipeline
  if (last_loaded_buffer_ && color_service()) {

    if (deinterlace_) {
      qDebug() << "FIXME: Deinterlacing is currently broken, we're working on this...";
      //color_service()->pipeline()->setUniformValue("ove_resolution", texture_.width(), texture_.height());
      //color_service()->pipeline()->setUniformValue("ove_deinterlace", deinterlace_);
    }

    // Draw texture through color transform
    renderer()->BlitColorManaged(color_service(), texture_, true,
                                 VideoParams(width(), height(), PixelFormat::PIX_FMT_RGBA16F),
                                 GetCompleteMatrixFlippedYTranslation());
  }

  QTransform world_transform = GenerateWorldTransform();

  // Draw gizmos if we have any
  if (gizmos_) {
    GizmoTraverser gt(QSize(gizmo_params_.width(), gizmo_params_.height()));

    rational node_time = GetGizmoTime();

    gizmo_db_ = gt.GenerateDatabase(gizmos_, TimeRange(node_time, node_time));

    QPainter p(inner_widget());
    p.setWorldTransform(world_transform);
    gizmos_->DrawGizmos(gizmo_db_, &p, QVector2D(GetTexturePosition(size())), size());
  }

  // Draw action/title safe areas
  if (safe_margin_.is_enabled()) {
    QPainter p(inner_widget());
    p.setWorldTransform(world_transform);

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

    QLine lines[] = {QLine(rect().center().x() - cross, rect().center().y(),rect().center().x() + cross, rect().center().y()),
                     QLine(rect().center().x(), rect().center().y() - cross, rect().center().x(), rect().center().y() + cross)};

    p.drawLines(lines, 2);
  }
}

void ViewerDisplayWidget::OnDestroy()
{
  ManagedDisplayWidget::OnDestroy();

  texture_ = nullptr;
}

QMatrix4x4 ViewerDisplayWidget::GetMatrixTranslate()
{
  return translate_matrix_;
}

QPointF ViewerDisplayWidget::GetTexturePosition(const QPoint &screen_pos)
{
  return GetTexturePosition(screen_pos.x(), screen_pos.y());
}

QPointF ViewerDisplayWidget::GetTexturePosition(const QSize &size)
{
  return GetTexturePosition(size.width(), size.height());
}

QPointF ViewerDisplayWidget::GetTexturePosition(const double &x, const double &y)
{
  return QPointF(x / gizmo_params_.width(),
                 y / gizmo_params_.height());
}

rational ViewerDisplayWidget::GetGizmoTime()
{
  return GetAdjustedTime(GetTimeTarget(), gizmos_, time_, NodeParam::kInput);
}

bool ViewerDisplayWidget::IsHandDrag(QMouseEvent *event) const
{
  return event->button() == Qt::MiddleButton || Core::instance()->tool() == Tool::kHand;
}

void ViewerDisplayWidget::UpdateMatrix()
{
  combined_matrix_ = scale_matrix_ * translate_matrix_;

  update();
}

QTransform ViewerDisplayWidget::GenerateWorldTransform()
{
  /*
   * Get matrix elements (roughly) as below in column major order
   *
   * | Sx 0  0  Tx |
   * | 0  Sy 0  Ty |
   * | 0  0  Sz Tz |
   * | 0  0  0  1  |
   */
  float *d = combined_matrix_.data();
  QTransform world;
  // Move corner of canvas to correct point
  world.translate(width() * 0.5 - width() * *(d)*0.5, height() * 0.5 - height() * *(d + 5) * 0.5);
  // Scale
  world.scale(*(d), *(d + 5));
  // Translate for mouse movement
  world.translate(*(d + 12) * width() * 0.5 / *(d), *(d + 13) * height() * 0.5 / *(d + 5));

  return world;
}

OLIVE_NAMESPACE_EXIT
