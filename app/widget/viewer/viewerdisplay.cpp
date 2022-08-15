/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QPainter>
#include <QPushButton>
#include <QScreen>
#include <QTextEdit>

#include "common/html.h"
#include "common/qtutils.h"
#include "config/config.h"
#include "core.h"
#include "node/block/subtitle/subtitle.h"
#include "node/gizmo/path.h"
#include "node/gizmo/point.h"
#include "node/gizmo/polygon.h"
#include "node/gizmo/screen.h"
#include "viewertexteditor.h"

namespace olive {

#define super ManagedDisplayWidget

ViewerDisplayWidget::ViewerDisplayWidget(QWidget *parent) :
  super(parent),
  deinterlace_texture_(nullptr),
  signal_cursor_color_(false),
  gizmos_(nullptr),
  current_gizmo_(nullptr),
  gizmo_drag_started_(false),
  show_subtitles_(true),
  subtitle_tracks_(nullptr),
  hand_dragging_(false),
  deinterlace_(false),
  show_fps_(false),
  frames_skipped_(0),
  show_widget_background_(false),
  playback_speed_(0),
  push_mode_(kPushNull),
  add_band_(false),
  queue_starved_(false)
{
  connect(Core::instance(), &Core::ToolChanged, this, &ViewerDisplayWidget::ToolChanged);

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
    this->inner_widget()->setCursor(Qt::OpenHandCursor);
  } else if (Core::instance()->tool() == Tool::kAdd) {
    this->inner_widget()->setCursor(Qt::CrossCursor);
  } else {
    this->inner_widget()->unsetCursor();
  }
}

void ViewerDisplayWidget::SetSignalCursorColorEnabled(bool e)
{
  signal_cursor_color_ = e;
  SetInnerMouseTracking(e);
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

void ViewerDisplayWidget::ToolChanged()
{
  UpdateCursor();
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

void ViewerDisplayWidget::SetSubtitleTracks(Sequence *list)
{
  if (subtitle_tracks_) {
    disconnect(subtitle_tracks_, &Sequence::SubtitlesChanged, this, &ViewerDisplayWidget::SubtitlesChanged);
  }

  subtitle_tracks_ = list;

  if (subtitle_tracks_) {
    connect(subtitle_tracks_, &Sequence::SubtitlesChanged, this, &ViewerDisplayWidget::SubtitlesChanged);
  }

  update();
}

QPointF ViewerDisplayWidget::TransformViewerSpaceToBufferSpace(const QPointF &pos)
{
  /*
  * Inversion will only fail if the viewer has been scaled by 0 in any direction
  * which I think should never happen.
  */
  return pos * GenerateDisplayTransform().inverted();
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

bool ViewerDisplayWidget::eventFilter(QObject *o, QEvent *e)
{
  if (o != this->inner_widget()) {
    return super::eventFilter(o, e);
  }

  switch (e->type()) {
  case QEvent::MouseButtonPress:
  {
    QMouseEvent *mouse = static_cast<QMouseEvent*>(e);
    if (!(mouse->flags() & Qt::MouseEventCreatedDoubleClick)) {
      if (OnMousePress(mouse)) {
        return true;
      }
    }
    break;
  }
  case QEvent::MouseMove:
    EmitColorAtCursor(static_cast<QMouseEvent*>(e));
    if (OnMouseMove(static_cast<QMouseEvent*>(e))) {
      return true;
    }
    break;
  case QEvent::MouseButtonRelease:
    if (OnMouseRelease(static_cast<QMouseEvent*>(e))) {
      return true;
    }
    break;
  case QEvent::MouseButtonDblClick:
    if (OnMouseDoubleClick(static_cast<QMouseEvent*>(e))) {
      return true;
    }
    break;
  case QEvent::DragEnter:
    emit DragEntered(static_cast<QDragEnterEvent*>(e));
    break;
  case QEvent::DragLeave:
    emit DragLeft(static_cast<QDragLeaveEvent*>(e));
    break;
  case QEvent::Drop:
    emit Dropped(static_cast<QDropEvent*>(e));
    break;
  default:
    break;
  }

  return super::eventFilter(o, e);
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
    VideoParams::Format device_format = static_cast<VideoParams::Format>(OLIVE_CONFIG("OfflinePixelFormat").toInt());
    VideoParams device_params(device_width, device_height, device_format, VideoParams::kInternalChannelCount);

    if (push_mode_ == kPushBlank) {
      if (blank_shader_.isNull()) {
        blank_shader_ = renderer()->CreateNativeShader(ShaderCode());
      }

      ShaderJob job;
      job.Insert(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, combined_matrix_flipped_));
      job.Insert(QStringLiteral("ove_cropmatrix"), NodeValue(NodeValue::kMatrix, crop_matrix_));

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
        job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, QVector2D(texture_to_draw->width(), texture_to_draw->height())));
        job.Insert(QStringLiteral("ove_maintex"), NodeValue(NodeValue::kTexture, QVariant::fromValue(texture_to_draw)));

        renderer()->BlitToTexture(deinterlace_shader_, job, deinterlace_texture_.get());

        texture_to_draw = deinterlace_texture_;
      }

      ColorTransformJob ctj;
      ctj.SetColorProcessor(color_service());
      ctj.SetInputTexture(texture_to_draw);
      ctj.SetInputAlphaAssociation(OLIVE_CONFIG("ReassocLinToNonLin").toBool() ? kAlphaAssociated : kAlphaNone);
      ctj.SetClearDestinationEnabled(false);
      ctj.SetTransformMatrix(combined_matrix_flipped_);
      ctj.SetCropMatrix(crop_matrix_);

      renderer()->BlitColorManaged(ctj, device_params);
    }
  }

  // Draw gizmos if we have any
  if (gizmos_) {
    NodeTraverser gt;
    gt.SetCacheVideoParams(gizmo_params_);

    TimeRange range = GenerateGizmoTime();
    gizmo_db_ = gt.GenerateRow(gizmos_, range);

    QPainter p(paint_device());
    gizmo_last_draw_transform_ = GenerateGizmoTransform(gt, range);
    p.setWorldTransform(gizmo_last_draw_transform_);

    gizmos_->UpdateGizmoPositions(gizmo_db_, NodeTraverser::GenerateGlobals(gizmo_params_, range));
    foreach (NodeGizmo *gizmo, gizmos_->GetGizmos()) {
      if (gizmo->IsVisible()) {
        gizmo->Draw(&p);
      }
    }
  }

  // Draw action/title safe areas
  if (safe_margin_.is_enabled()) {
    QPainter p(paint_device());
    p.setWorldTransform(GenerateWorldTransform());

    p.setPen(QPen(Qt::lightGray, 0));
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
      QPainter p(paint_device());

      double average = 0.0;
      for (int i=0; i<frame_rate_averages_.size(); i++) {
        average += frame_rate_averages_[i];
      }
      average /= double(frame_rate_averages_.size());

      DrawTextWithCrudeShadow(&p, GetInnerRect(), tr("%1 FPS").arg(QString::number(average, 'f', 1)));

      if (frames_skipped_ > 0) {
        DrawTextWithCrudeShadow(&p, GetInnerRect().adjusted(0, p.fontMetrics().height(), 0, 0),
                                tr("%1 frames skipped").arg(frames_skipped_));
      }
    }
  }

  // Extraordinarily basic subtitle renderer. Hoping to swap this out with libass at some point.
  DrawSubtitleTracks();

  if (add_band_) {
    QPainter p(paint_device());
    QColor highlight = palette().highlight().color();
    p.setPen(highlight);
    highlight.setAlpha(128);
    p.setBrush(highlight);
    p.drawRect(QRect(add_band_start_, add_band_end_).normalized());
  }
}

void ViewerDisplayWidget::OnDestroy()
{
  if (!deinterlace_shader_.isNull()) {
    renderer()->DestroyNativeShader(deinterlace_shader_);
    deinterlace_shader_.clear();
  }
  if (!blank_shader_.isNull()) {
    renderer()->DestroyNativeShader(blank_shader_);
    blank_shader_.clear();
  }

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

void ViewerDisplayWidget::DrawTextWithCrudeShadow(QPainter *painter, const QRect &rect, const QString &text, const QTextOption &opt)
{
  painter->setPen(Qt::black);
  painter->drawText(rect.adjusted(1, 1, 0, 0), text, opt);
  painter->setPen(Qt::white);
  painter->drawText(rect, text, opt);
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

QTransform ViewerDisplayWidget::GenerateDisplayTransform()
{
  QVector2D viewer_scale(GetTexturePosition(size()));
  QTransform gizmo_transform = GenerateWorldTransform();
  gizmo_transform.scale(viewer_scale.x(), viewer_scale.y());
  gizmo_transform.scale(gizmo_params_.pixel_aspect_ratio().flipped().toDouble(), 1);
  return gizmo_transform;
}

QTransform ViewerDisplayWidget::GenerateGizmoTransform(NodeTraverser &gt, const TimeRange &range)
{
  QTransform t = GenerateDisplayTransform();
  if (GetTimeTarget()) {
    Node *target = GetTimeTarget();
    if (ViewerOutput *v = dynamic_cast<ViewerOutput *>(target)) {
      if (Node *n = v->GetConnectedTextureOutput()) {
        target = n;
      }
    }

    QTransform nt;
    gt.Transform(&nt, gizmos_, target, range);

    t.translate(gizmo_params_.width()*0.5, gizmo_params_.height()*0.5);
    t.scale(gizmo_params_.width(), gizmo_params_.height());

    t = nt * t;

    t.scale(1.0 / gizmo_params_.width(), 1.0 / gizmo_params_.height());
    t.translate(-gizmo_params_.width()*0.5, -gizmo_params_.height()*0.5);
  }

  return t;
}

NodeGizmo *ViewerDisplayWidget::TryGizmoPress(const NodeValueRow &row, const QPointF &p)
{
  for (auto it=gizmos_->GetGizmos().crbegin(); it!=gizmos_->GetGizmos().crend(); it++) {
    NodeGizmo *gizmo = *it;
    if (gizmo->IsVisible()) {
      if (PointGizmo *point = dynamic_cast<PointGizmo*>(gizmo)) {
        if (point->GetClickingRect(gizmo_last_draw_transform_).contains(p)) {
          return point;
        }
      } else if (PolygonGizmo *poly = dynamic_cast<PolygonGizmo*>(gizmo)) {
        if (poly->GetPolygon().containsPoint(p, Qt::OddEvenFill)) {
          return poly;
        }
      } else if (PathGizmo *path = dynamic_cast<PathGizmo*>(gizmo)) {
        if (path->GetPath().contains(p)) {
          return path;
        }
      } else if (ScreenGizmo *screen = dynamic_cast<ScreenGizmo*>(gizmo)) {
        // NOTE: Perhaps this should limit to the actual visible screen space? We'll see.
        return screen;
      }
    }
  }

  return nullptr;
}

void ViewerDisplayWidget::OpenTextGizmo(TextGizmo *text, QMouseEvent *event)
{
  QTransform gizmo_transform = GenerateDisplayTransform();

  // Create popup container for text and toolbar
  auto popup = new QWidget(this);
  popup->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
  popup->setAttribute(Qt::WA_DeleteOnClose);
  popup->setAttribute(Qt::WA_TranslucentBackground);

  // Create text editor
  ViewerTextEditor *text_edit = new ViewerTextEditor(gizmo_transform.m11(), popup);
  Html::HtmlToDoc(text_edit->document(), text->GetHtml());
  text_edit->setProperty("gizmo", reinterpret_cast<quintptr>(text));
  connect(text_edit, &ViewerTextEditor::textChanged, this, &ViewerDisplayWidget::TextEditChanged);
  connect(text_edit, &ViewerTextEditor::destroyed, this, &ViewerDisplayWidget::TextEditDestroyed);

  emit text->Activated();

  // Get on screen text rect (this will be the text editor's global geometry)
  QRect global_text_area = gizmo_transform.map(text->GetRect()).boundingRect().toRect();
  global_text_area = QRect(mapToGlobal(global_text_area.topLeft()), mapToGlobal(global_text_area.bottomRight()));

  QRect global_popup_area = global_text_area;

  // Create toolbar
  ViewerTextEditorToolBar *toolbar = new ViewerTextEditorToolBar(popup);
  text_edit->ConnectToolBar(toolbar);

  // Work out which corner of the text editor to anchor the toolbar to based on screen limitations
  bool top = true;
  bool left = true;
  for (QScreen *screen : qApp->screens()) {
    // Look for screen that contains text area
    if (screen->geometry().contains(global_text_area)) {
      if (global_text_area.left() + toolbar->width() > screen->geometry().right()) {
        left = false;
      }
      if (global_text_area.top() - toolbar->height() < screen->geometry().top()) {
        top = false;
      }
      break;
    }
  }


  QPoint toolbar_pos;

  if (top) {
    global_popup_area.adjust(0, -toolbar->height(), 0, 0);
    toolbar_pos.setY(0);
  } else {
    global_popup_area.adjust(0, 0, 0, toolbar->height());
    toolbar_pos.setY(global_text_area.height());
  }

  if (toolbar->width() > global_popup_area.width()) {
    int diff = toolbar->width() - global_popup_area.width();
    if (left) {
      global_popup_area.adjust(0, 0, diff, 0);
    } else {
      global_popup_area.adjust(-diff, 0, 0, 0);
    }
    toolbar_pos.setX(0);
  } else {
    if (left) {
      toolbar_pos.setX(0);
    } else {
      toolbar_pos.setX(global_popup_area.width() - toolbar->width());
    }
  }

  toolbar->move(toolbar_pos);

  popup->setGeometry(global_popup_area);

  text_edit->setGeometry(QRect(text_edit->mapFromGlobal(global_text_area.topLeft()), text_edit->mapFromGlobal(global_text_area.bottomRight())));

  popup->show();

  // Store click pos from event so we can use it later to set the initial text cursor position
  QPoint click_pos;
  if (event) {
    click_pos = event->globalPos();
  }

  // Ensure text edit is actually focused rather than the toolbar
  connect(toolbar, &ViewerTextEditorToolBar::FirstPaint, this, [text_edit, click_pos]{
    // Grab focus back from the toolbar
    text_edit->setFocus();

    // Start text cursor where the user clicked
    if (!click_pos.isNull()) {
      text_edit->setTextCursor(text_edit->cursorForPosition(text_edit->mapFromGlobal(click_pos)));
    }
  });
}

bool ViewerDisplayWidget::OnMousePress(QMouseEvent *event)
{
  if (IsHandDrag(event)) {

    // Handle hand drag
    hand_last_drag_pos_ = event->pos();
    hand_dragging_ = true;
    emit HandDragStarted();
    setCursor(Qt::ClosedHandCursor);

    return true;

  } else if (event->button() == Qt::LeftButton) {

    if (Core::instance()->tool() == Tool::kAdd
        && (Core::instance()->GetSelectedAddableObject() == Tool::kAddableShape || Core::instance()->GetSelectedAddableObject() == Tool::kAddableTitle)) {

      add_band_start_ = event->pos();
      add_band_end_ = add_band_start_;
      add_band_ = true;

    } else if (gizmos_
               && (gizmo_last_draw_transform_inverted_ = gizmo_last_draw_transform_.inverted(),
                   current_gizmo_ = TryGizmoPress(gizmo_db_, gizmo_last_draw_transform_inverted_.map(event->pos())))) {

      // Handle gizmo click
      gizmo_start_drag_ = event->pos();
      gizmo_last_drag_ = gizmo_start_drag_;
      current_gizmo_->SetGlobals(NodeTraverser::GenerateGlobals(gizmo_params_, GenerateGizmoTime()));

    } else {

      // Handle standard drag
      emit DragStarted();

    }

    return true;

  }

  return false;
}

bool ViewerDisplayWidget::OnMouseMove(QMouseEvent *event)
{
  // Handle hand dragging
  if (hand_dragging_) {

    // Emit movement
    emit HandDragMoved(event->x() - hand_last_drag_pos_.x(),
                       event->y() - hand_last_drag_pos_.y());

    hand_last_drag_pos_ = event->pos();

    return true;

  } else if (add_band_) {

    add_band_end_ = event->pos();
    update();
    return true;

  } else if (current_gizmo_) {

    // Signal movement
    if (DraggableGizmo *draggable = dynamic_cast<DraggableGizmo*>(current_gizmo_)) {
      if (!gizmo_drag_started_) {
        QPointF start = gizmo_start_drag_ * gizmo_last_draw_transform_inverted_;

        rational gizmo_time = GetGizmoTime();
        NodeTraverser t;
        t.SetCacheVideoParams(gizmo_params_);
        NodeValueRow row = t.GenerateRow(gizmos_, TimeRange(gizmo_time, gizmo_time + gizmo_params_.frame_rate_as_time_base()));

        draggable->DragStart(row, start.x(), start.y(), gizmo_time);
        gizmo_drag_started_ = true;
      }

      QPointF v = event->pos() * gizmo_last_draw_transform_inverted_;
      switch (draggable->GetDragValueBehavior()) {
      case DraggableGizmo::kAbsolute:
        // Above value is correct
        break;
      case DraggableGizmo::kDeltaFromPrevious:
        v -= gizmo_last_drag_ * gizmo_last_draw_transform_inverted_;
        gizmo_last_drag_ = event->pos();
        break;
      case DraggableGizmo::kDeltaFromStart:
        v -= gizmo_start_drag_ * gizmo_last_draw_transform_inverted_;
        break;
      }

      draggable->DragMove(v.x(), v.y(), event->modifiers());

      return true;
    }

  }

  return false;
}

bool ViewerDisplayWidget::OnMouseRelease(QMouseEvent *e)
{
  if (hand_dragging_) {

    // Handle hand drag
    emit HandDragEnded();
    hand_dragging_ = false;
    UpdateCursor();

    return true;

  } else if (add_band_) {

    QRect band_rect = QRect(add_band_start_, add_band_end_).normalized();
    if (band_rect.width() > 1 && band_rect.height() > 1) {
      QRectF r = GenerateDisplayTransform().inverted().mapRect(band_rect);
      emit CreateAddableAt(r);
    }

    add_band_ = false;
    return true;

  } else if (current_gizmo_) {

    // Handle gizmo
    if (gizmo_drag_started_) {
      MultiUndoCommand *command = new MultiUndoCommand();
      if (DraggableGizmo *draggable = dynamic_cast<DraggableGizmo*>(current_gizmo_)) {
        draggable->DragEnd(command);
      }
      Core::instance()->undo_stack()->pushIfHasChildren(command);
      gizmo_drag_started_ = false;
    }
    current_gizmo_ = nullptr;

    return true;

  }

  return false;
}

bool ViewerDisplayWidget::OnMouseDoubleClick(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton && gizmos_) {
    QPointF ptr = TransformViewerSpaceToBufferSpace(event->pos());
    foreach (NodeGizmo *g, gizmos_->GetGizmos()) {
      if (TextGizmo *text = dynamic_cast<TextGizmo*>(g)) {
        if (text->GetRect().contains(ptr)) {
          OpenTextGizmo(text, event);
          return true;
        }
      }
    }
  }

  return false;
}

void ViewerDisplayWidget::EmitColorAtCursor(QMouseEvent *e)
{
  // Do this no matter what, emits signal to any pixel samplers
  if (signal_cursor_color_) {
    Color reference, display;

    if (texture_) {
      QPointF pixel_pos = GenerateDisplayTransform().inverted().map(e->pos());
      pixel_pos /= texture_->params().divider();

      makeCurrent();

      reference = renderer()->GetPixelFromTexture(texture_.get(), pixel_pos);
      display = color_service()->ConvertColor(reference);
    }

    emit CursorColor(reference, display);
  }
}

void ViewerDisplayWidget::DrawSubtitleTracks()
{
  if (!show_subtitles_ || !subtitle_tracks_) {
    return;
  }

  const QVector<Track*> &subtitle_tracklist = subtitle_tracks_->track_list(Track::kSubtitle)->GetTracks();
  if (subtitle_tracklist.empty()) {
    return;
  }

  // Scale font size by transform
  QTransform display_transform = GenerateDisplayTransform();
  qreal font_sz = OLIVE_CONFIG("DefaultSubtitleSize").toInt();
  font_sz *= display_transform.m11();
  if (qIsNaN(font_sz)) {
    return;
  }

  QPainterPath path;

  QTransform transform = GenerateWorldTransform();
  QRect bounding_box = transform.mapRect(rect());

  QFont f;
  f.setPointSizeF(font_sz);

  QString family = OLIVE_CONFIG("DefaultSubtitleFamily").toString();
  if (!family.isEmpty()) {
    f.setFamily(family);
  }

  f.setWeight(OLIVE_CONFIG("DefaultSubtitleWeight").toInt());

  bounding_box.adjust(bounding_box.width()/10, bounding_box.height()/10, -bounding_box.width()/10, -bounding_box.height()/10);

  QFontMetrics fm(f);

  for (int j=subtitle_tracklist.size()-1; j>=0; j--) {
    Track *sub_track = subtitle_tracklist.at(j);
    if (!sub_track->IsMuted()) {
      if (SubtitleBlock *sub = dynamic_cast<SubtitleBlock*>(sub_track->BlockAtTime(time_))) {
        // Split into lines
        QStringList list = QtUtils::WordWrapString(sub->GetText(), fm, bounding_box.width());

        for (int i=list.size()-1; i>=0; i--) {
          int w = QtUtils::QFontMetricsWidth(fm, list.at(i));
          path.addText(bounding_box.width()/2 - w/2, bounding_box.height() - fm.height() * (list.size() - i) + fm.ascent(), f, list.at(i));
        }
      }
    }
  }

  bool antialias = OLIVE_CONFIG("AntialiasSubtitles").toBool();

  QPixmap *aa_pixmap;
  QPainter *text_painter;
  if (antialias) {
    // QPainter only supports anti-aliasing in software, so to achieve it, we draw to a
    // software buffer first and then draw that onto the hardware
    aa_pixmap = new QPixmap(bounding_box.width(), bounding_box.height());
    aa_pixmap->fill(Qt::transparent);
    text_painter = new QPainter(aa_pixmap);
  } else {
    // Just draw straight to the hardware
    text_painter = new QPainter(paint_device());

    // Offset path by however much is necessary
    path.translate(bounding_box.x(), bounding_box.y());
  }

  text_painter->setPen(QPen(Qt::black, f.pointSizeF() / 16));
  text_painter->setBrush(Qt::white);
  text_painter->setRenderHint(QPainter::Antialiasing);

  text_painter->drawPath(path);

  delete text_painter;

  if (antialias) {
    // We just drew to a software buffer, now draw this image onto the hardware device
    QPainter p(paint_device());
    p.drawPixmap(bounding_box.x(), bounding_box.y(), *aa_pixmap);
    delete aa_pixmap;
  }
}

void ViewerDisplayWidget::SetShowFPS(bool e)
{
  show_fps_ = e;

  update();
}

void ViewerDisplayWidget::RequestStartEditingText()
{
  if (gizmos_) {
    foreach (NodeGizmo *gizmo, gizmos_->GetGizmos()) {
      if (TextGizmo *text = dynamic_cast<TextGizmo*>(gizmo)) {
        OpenTextGizmo(text);
        break;
      }
    }
  }
}

void ViewerDisplayWidget::Play(const int64_t &start_timestamp, const int &playback_speed, const rational &timebase)
{
  playback_timebase_ = timebase;
  playback_speed_ = playback_speed;

  timer_.Start(start_timestamp, playback_speed, timebase.toDouble());

  connect(this, &ViewerDisplayWidget::frameSwapped, this, &ViewerDisplayWidget::UpdateFromQueue);

  update();
}

void ViewerDisplayWidget::Pause()
{
  disconnect(this, &ViewerDisplayWidget::frameSwapped, this, &ViewerDisplayWidget::UpdateFromQueue);

  queue_.clear();
  queue_starved_ = false;
}

void ViewerDisplayWidget::UpdateFromQueue()
{
  int64_t t = timer_.GetTimestampNow();

  rational time = Timecode::timestamp_to_time(t, playback_timebase_);

  bool popped = false;

  if (queue_.empty()) {
    queue_starved_ = true;
    emit QueueStarved();
  } else {
    while (!queue_.empty()) {
      const ViewerPlaybackFrame& pf = queue_.front();

      if (pf.timestamp == time) {

        // Frame was in queue, no need to decode anything
        SetImage(pf.frame);

        if (queue_starved_) {
          queue_starved_ = false;
          emit QueueNoLongerStarved();
        }
        return;

      } else if ((pf.timestamp > time) == (playback_speed_ > 0)) {

        // The next frame in the queue is too new, so just do a regular update. Either the
        // frame we want will arrive in time, or we'll just have to skip it.
        break;

      } else {

        queue_.pop_front();

        if (popped) {
          // We've already popped a frame in this loop, meaning a frame has been skipped
          IncrementSkippedFrames();
        } else {
          // Shown a frame and progressed to the next one
          IncrementFrameCount();
          popped = true;
        }

        if (queue_.empty()) {
          queue_starved_ = true;
          emit QueueStarved();
          break;
        }

      }
    }
  }

  update();
}

void ViewerDisplayWidget::TextEditChanged()
{
  ViewerTextEditor *editor = static_cast<ViewerTextEditor *>(sender());

  TextGizmo *gizmo = reinterpret_cast<TextGizmo*>(editor->property("gizmo").value<quintptr>());

  QString html = Html::DocToHtml(editor->document());
  gizmo->UpdateInputHtml(html, GetGizmoTime());
}

void ViewerDisplayWidget::TextEditDestroyed()
{
  TextGizmo *gizmo = reinterpret_cast<TextGizmo*>(sender()->property("gizmo").value<quintptr>());
  emit gizmo->Deactivated();
}

void ViewerDisplayWidget::SubtitlesChanged(const TimeRange &r)
{
  if (time_ >= r.in() && time_ < r.out()) {
    update();
  }
}

}
