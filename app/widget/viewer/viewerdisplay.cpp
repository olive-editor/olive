/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "config/config.h"
#include "core.h"
#include "node/traverser.h"

namespace olive {

#define super ManagedDisplayWidget

ViewerDisplayWidget::ViewerDisplayWidget(QWidget *parent) :
  super(parent),
  deinterlace_texture_(nullptr),
  signal_cursor_color_(false),
  gizmos_(nullptr),
  gizmo_click_(false),
  hand_dragging_(false),
  deinterlace_(false),
  show_fps_(false),
  frames_skipped_(0),
  show_widget_background_(false),
  push_mode_(kPushNull)
{
  connect(Core::instance(), &Core::ToolChanged, this, &ViewerDisplayWidget::UpdateCursor);

  connect(this, &ViewerDisplayWidget::InnerWidgetMouseMove, this, &ViewerDisplayWidget::EmitColorAtCursor);

  // Initializes cursor based on tool
  UpdateCursor();

  const int kFrameRateAverageCount = 8;
  frame_rate_averages_.resize(kFrameRateAverageCount);
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

void ViewerDisplayWidget::SetMatrixCrop(const QMatrix4x4 &mat)
{
  crop_matrix_ = mat;

  update();
}

void ViewerDisplayWidget::UpdateCursor()
{
  if (Core::instance()->tool() == Tool::kHand) {
    setCursor(Qt::OpenHandCursor);
  } else {
    unsetCursor();
  }
}

void ViewerDisplayWidget::SetSignalCursorColorEnabled(bool e)
{
  signal_cursor_color_ = e;
  inner_widget()->setMouseTracking(e);
}

void ViewerDisplayWidget::SetImage(const QVariant &buffer)
{
  load_frame_ = buffer;

  if (load_frame_.isNull()) {
    push_mode_ = kPushNull;
  } else {
    push_mode_ = kPushFrame;
  }

  update();
}

void ViewerDisplayWidget::SetBlank()
{
  push_mode_ = kPushBlank;

  update();
}

void ViewerDisplayWidget::SetDeinterlacing(bool e)
{
  deinterlace_ = e;

  if (!deinterlace_) {
    if (!deinterlace_shader_.isNull()) {
      renderer()->DestroyNativeShader(deinterlace_shader_);
      deinterlace_shader_.clear();
    }
    deinterlace_texture_ = nullptr;
  }

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

QPoint ViewerDisplayWidget::TransformViewerSpaceToBufferSpace(QPoint pos)
{
  /*
  * Inversion will only fail if the viewer has been scaled by 0 in any direction
  * which I think should never happen.
  */
  return pos * GenerateGizmoTransform().inverted();
}

void ViewerDisplayWidget::ResetFPSTimer()
{
  fps_timer_start_ = QDateTime::currentMSecsSinceEpoch();
  fps_timer_update_count_ = 0;
  frames_skipped_ = 0;
  frame_rate_average_count_ = 0;

  Core::instance()->ClearStatusBarMessage();
}

void ViewerDisplayWidget::IncrementSkippedFrames()
{
  frames_skipped_++;

  Core::instance()->ShowStatusBarMessage(tr("%n skipped frame(s) detected during playback", nullptr, frames_skipped_), 10000);
}

void ViewerDisplayWidget::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton && gizmos_
      && gizmos_->GizmoPress(gizmo_db_, NodeTraverser::GenerateGlobals(gizmo_params_, GenerateGizmoTime()), TransformViewerSpaceToBufferSpace(event->pos()))) {

    // Handle gizmo click
    gizmo_click_ = true;
    gizmo_drag_time_ = GetGizmoTime();
    gizmo_start_drag_ = event->pos();

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

    super::mousePressEvent(event);

  }
}

void ViewerDisplayWidget::mouseMoveEvent(QMouseEvent *event)
{
  // Handle hand dragging
  if (hand_dragging_) {

    // Emit movement
    emit HandDragMoved(event->x() - hand_last_drag_pos_.x(),
                       event->y() - hand_last_drag_pos_.y());

    hand_last_drag_pos_ = event->pos();

  } else if (gizmo_click_) {

    // Signal movement
    gizmos_->GizmoMove(TransformViewerSpaceToBufferSpace(event->pos()),
                       gizmo_drag_time_,
                       event->modifiers());
    gizmo_start_drag_ = event->pos();
    update();

  } else {

    // Default behavior
    super::mouseMoveEvent(event);

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
    MultiUndoCommand *command = new MultiUndoCommand();
    gizmos_->GizmoRelease(command);
    Core::instance()->undo_stack()->pushIfHasChildren(command);
    gizmo_click_ = false;

  } else {

    // Default behavior
    super::mouseReleaseEvent(event);

  }
}

void ViewerDisplayWidget::dragEnterEvent(QDragEnterEvent *event)
{
  emit DragEntered(event);

  if (!event->isAccepted()) {
    super::dragEnterEvent(event);
  }
}

void ViewerDisplayWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
  emit DragLeft(event);

  if (!event->isAccepted()) {
    super::dragLeaveEvent(event);
  }
}

void ViewerDisplayWidget::dropEvent(QDropEvent *event)
{
  emit Dropped(event);

  if (!event->isAccepted()) {
    super::dropEvent(event);
  }
}

void ViewerDisplayWidget::showEvent(QShowEvent *event)
{
  emit VisibilityChanged(true);

  super::showEvent(event);
}

void ViewerDisplayWidget::hideEvent(QHideEvent *event)
{
  emit VisibilityChanged(false);

  super::hideEvent(event);
}

void ViewerDisplayWidget::OnPaint()
{
  // Clear background to empty
  QColor bg_color = show_widget_background_ ? palette().window().color() : Qt::black;
  renderer()->ClearDestination(nullptr, bg_color.redF(), bg_color.greenF(), bg_color.blueF());

  // We only draw if we have a pipeline
  if (push_mode_ != kPushNull) {
    // Draw texture through color transform
    int device_width = width() * devicePixelRatioF();
    int device_height = height() * devicePixelRatioF();
    VideoParams::Format device_format = static_cast<VideoParams::Format>(Config::Current()["OfflinePixelFormat"].toInt());
    VideoParams device_params(device_width, device_height, device_format, VideoParams::kInternalChannelCount);

    if (push_mode_ == kPushBlank) {
      if (blank_shader_.isNull()) {
        blank_shader_ = renderer()->CreateNativeShader(ShaderCode());
      }

      ShaderJob job;
      job.InsertValue(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, combined_matrix_flipped_));
      job.InsertValue(QStringLiteral("ove_cropmatrix"), NodeValue(NodeValue::kMatrix, crop_matrix_));

      renderer()->Blit(blank_shader_, job, device_params, false);
    } else if (color_service()) {
      if (FramePtr frame = load_frame_.value<FramePtr>()) {
        // This is a CPU frame, upload it now
        if (!texture_
            || texture_->renderer() != renderer() // Some implementations don't like it if we upload to a texture created in another (albeit shared) context
            || texture_->width() != frame->width()
            || texture_->height() != frame->height()
            || texture_->format() != frame->format()
            || texture_->channel_count() != frame->channel_count()) {
          texture_ = renderer()->CreateTexture(frame->video_params(), frame->data(), frame->linesize_pixels());
        } else {
          texture_->Upload(frame->data(), frame->linesize_pixels());
        }
      } else if (TexturePtr texture = load_frame_.value<TexturePtr>()) {
        // This is a GPU texture, switch to it directly
        texture_ = texture;
      }

      emit TextureChanged(texture_);

      push_mode_ = kPushUnnecessary;

      TexturePtr texture_to_draw = texture_;

      if (deinterlace_) {
        if (deinterlace_shader_.isNull()) {
          deinterlace_shader_ = renderer()->CreateNativeShader(ShaderCode(FileFunctions::ReadFileAsString(QStringLiteral(":/shaders/deinterlace.frag"))));
        }

        if (!deinterlace_texture_
            || deinterlace_texture_->params() != texture_to_draw->params()) {
          // (Re)create texture
          deinterlace_texture_ = renderer()->CreateTexture(texture_to_draw->params());
        }

        ShaderJob job;
        job.InsertValue(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, QVector2D(texture_to_draw->width(), texture_to_draw->height())));
        job.InsertValue(QStringLiteral("ove_maintex"), NodeValue(NodeValue::kTexture, QVariant::fromValue(texture_to_draw)));

        renderer()->BlitToTexture(deinterlace_shader_, job, deinterlace_texture_.get());

        texture_to_draw = deinterlace_texture_;
      }

      renderer()->BlitColorManaged(color_service(), texture_to_draw,
                                   Config::Current()[QStringLiteral("ReassocLinToNonLin")].toBool() ? Renderer::kAlphaAssociated : Renderer::kAlphaNone,
                                   device_params, false,
                                   combined_matrix_flipped_, crop_matrix_);
    }
  }

  // Draw gizmos if we have any
  if (gizmos_) {
    NodeTraverser gt;
    gt.SetCacheVideoParams(gizmo_params_);

    TimeRange range = GenerateGizmoTime();
    gizmo_db_ = gt.GenerateRow(gizmos_, range);

    QPainter p(inner_widget());
    p.setWorldTransform(GenerateGizmoTransform());
    gizmos_->DrawGizmos(gizmo_db_, NodeTraverser::GenerateGlobals(gizmo_params_, range), &p);
  }

  // Draw action/title safe areas
  if (safe_margin_.is_enabled()) {
    QPainter p(inner_widget());
    p.setWorldTransform(GenerateWorldTransform());

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

  if (show_fps_) {
    {
      qint64 now = QDateTime::currentMSecsSinceEpoch();
      double frame_rate;
      if (now == fps_timer_start_) {
        // This will cause a divide by zero, so we do nothing here
        frame_rate = 0;
      } else {
        frame_rate = double(fps_timer_update_count_) / double((now - fps_timer_start_)/1000.0);
      }

      if (frame_rate > 0) {
        frame_rate_averages_[frame_rate_average_count_%frame_rate_averages_.size()] = frame_rate;
        frame_rate_average_count_++;
      }
    }

    if (frame_rate_average_count_ >= frame_rate_averages_.size()) {
      QPainter p(inner_widget());

      double average = 0.0;
      for (int i=0; i<frame_rate_averages_.size(); i++) {
        average += frame_rate_averages_[i];
      }
      average /= double(frame_rate_averages_.size());

      DrawTextWithCrudeShadow(&p, inner_widget()->rect(), tr("%1 FPS").arg(QString::number(average, 'f', 1)));

      if (frames_skipped_ > 0) {
        DrawTextWithCrudeShadow(&p, inner_widget()->rect().adjusted(0, p.fontMetrics().height(), 0, 0),
                                tr("%1 frames skipped").arg(frames_skipped_));
      }
    }
  }
}

void ViewerDisplayWidget::OnDestroy()
{
  renderer()->DestroyNativeShader(deinterlace_shader_);
  deinterlace_shader_.clear();
  renderer()->DestroyNativeShader(blank_shader_);
  blank_shader_.clear();

  super::OnDestroy();

  texture_ = nullptr;
  deinterlace_texture_ = nullptr;
  if (load_frame_.isNull()) {
    push_mode_ = kPushNull;
  } else {
    push_mode_ = kPushFrame;
  }
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

void ViewerDisplayWidget::DrawTextWithCrudeShadow(QPainter *painter, const QRect &rect, const QString &text)
{
  painter->setPen(Qt::black);
  painter->drawText(rect.adjusted(1, 1, 0, 0), text);
  painter->setPen(Qt::white);
  painter->drawText(rect, text);
}

rational ViewerDisplayWidget::GetGizmoTime()
{
  return GetAdjustedTime(GetTimeTarget(), gizmos_, time_, true);
}

bool ViewerDisplayWidget::IsHandDrag(QMouseEvent *event) const
{
  return event->button() == Qt::MiddleButton || Core::instance()->tool() == Tool::kHand;
}

void ViewerDisplayWidget::UpdateMatrix()
{
  combined_matrix_ = scale_matrix_ * translate_matrix_;

  combined_matrix_flipped_.setToIdentity();
  combined_matrix_flipped_.scale(1.0, -1.0, 1.0);
  combined_matrix_flipped_ *= combined_matrix_;

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

QTransform ViewerDisplayWidget::GenerateGizmoTransform()
{
  QVector2D viewer_scale(GetTexturePosition(size()));
  QTransform gizmo_transform = GenerateWorldTransform();
  gizmo_transform.scale(viewer_scale.x(), viewer_scale.y());
  gizmo_transform.scale(gizmo_params_.pixel_aspect_ratio().flipped().toDouble(), 1);
  return gizmo_transform;
}

void ViewerDisplayWidget::EmitColorAtCursor(QMouseEvent *e)
{
  // Do this no matter what, emits signal to any pixel samplers
  if (signal_cursor_color_) {
    Color reference, display;

    if (texture_) {
      QPointF pixel_pos = GenerateGizmoTransform().inverted().map(e->pos());
      pixel_pos /= texture_->params().divider();

      reference = renderer()->GetPixelFromTexture(texture_.get(), pixel_pos);
      display = color_service()->ConvertColor(reference);
    }

    emit CursorColor(reference, display);
  }
}

void ViewerDisplayWidget::SetShowFPS(bool e)
{
  show_fps_ = e;

  update();
}

}
