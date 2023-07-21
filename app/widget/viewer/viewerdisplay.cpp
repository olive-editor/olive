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

#include "common/define.h"
#include "common/html.h"
#include "common/qtutils.h"
#include "config/config.h"
#include "core.h"
#include "node/block/subtitle/subtitle.h"
#include "node/gizmo/path.h"
#include "node/gizmo/point.h"
#include "node/gizmo/polygon.h"
#include "node/gizmo/screen.h"
#include "window/mainwindow/mainwindow.h"

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
  queue_starved_(false),
  text_edit_(nullptr)
{
  connect(Core::instance(), &Core::ToolChanged, this, &ViewerDisplayWidget::ToolChanged);

  // Initializes cursor based on tool
  UpdateCursor();

  const int kFrameRateAverageCount = 8;
  frame_rate_averages_.resize(kFrameRateAverageCount);

  inner_widget()->setAcceptDrops(true);
}

ViewerDisplayWidget::~ViewerDisplayWidget()
{
  delete text_edit_;

  MANAGEDDISPLAYWIDGET_DEFAULT_DESTRUCTOR_INNER;
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

void ViewerDisplayWidget::SetAudioParams(const AudioParams &params)
{
  gizmo_audio_params_ = params;

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
  if (o == this->inner_widget()) {
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
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
      if (OnKeyPress(static_cast<QKeyEvent*>(e))) {
        return true;
      }
      break;
    case QEvent::KeyRelease:
      if (OnKeyRelease(static_cast<QKeyEvent*>(e))) {
        return true;
      }
      break;
    case QEvent::DragEnter:
    {
      auto drag_enter = static_cast<QDragEnterEvent*>(e);
      if (text_edit_) {
        ForwardDragEventToTextEdit(drag_enter);
      } else {
        emit DragEntered(drag_enter);
      }

      if (drag_enter->isAccepted()) {
        return true;
      }
      break;
    }
    case QEvent::DragMove:
    {
      auto drag_move = static_cast<QDragMoveEvent*>(e);
      if (text_edit_) {
        ForwardDragEventToTextEdit(drag_move);
      }

      if (drag_move->isAccepted()) {
        return true;
      }
      break;
    }
    case QEvent::DragLeave:
    {
      auto drag_leave = static_cast<QDragLeaveEvent*>(e);
      if (text_edit_) {
        ForwardDragEventToTextEdit(drag_leave);
      } else {
        emit DragLeft(drag_leave);
      }

      if (drag_leave->isAccepted()) {
        return true;
      }
      break;
    }
    case QEvent::Drop:
    {
      auto drop = static_cast<QDropEvent*>(e);
      if (text_edit_) {
        ForwardDragEventToTextEdit(drop);
      } else {
        emit Dropped(drop);
      }

      if (drop->isAccepted()) {
        return true;
      }
      break;
    }
    default:
      break;
    }
  } else if (o == text_edit_) {
    switch (e->type()) {
    case QEvent::Paint:
      update();
      return true;
    default:
      break;
    }
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
    VideoParams device_params = GetViewportParams();

    if (push_mode_ == kPushBlank) {
      DrawBlank(device_params);
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
      } else {
        texture_ = LoadCustomTextureFromFrame(load_frame_);
      }

      emit TextureChanged(texture_);

      push_mode_ = kPushUnnecessary;

      TexturePtr texture_to_draw = texture_;

      if (!texture_to_draw || texture_to_draw->IsDummy()) {
        DrawBlank(device_params);
      } else {
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
        ctj.SetForceOpaque(true);

        renderer()->BlitColorManaged(ctj, device_params);
      }
    }
  }

  // Draw gizmos if we have any
  if (gizmos_) {
    QPainter p(paint_device());

    GenerateGizmoTransforms();

    p.setWorldTransform(gizmo_last_draw_transform_);

    gizmos_->UpdateGizmoPositions(gizmo_db_, NodeGlobals(gizmo_params_, gizmo_audio_params_, gizmo_draw_time_, LoopMode::kLoopModeOff));
    foreach (NodeGizmo *gizmo, gizmos_->GetGizmos()) {
      if (gizmo->IsVisible()) {
        gizmo->Draw(&p);
      }
    }

    if (text_edit_) {
      QPixmap pm(text_edit_->width(), text_edit_->height());
      pm.fill(Qt::transparent);

      QPainter pixp(&pm);
      text_edit_->Paint(&pixp, active_text_gizmo_->GetVerticalAlignment());

      p.drawPixmap(text_edit_pos_, pm);
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
  return GetAdjustedTime(GetTimeTarget(), gizmos_, time_, Node::kTransformTowardsInput);
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
  if (!gizmos_) {
    return nullptr;
  }

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
  GenerateGizmoTransforms();
  gizmos_->UpdateGizmoPositions(gizmo_db_, NodeGlobals(gizmo_params_, gizmo_audio_params_, gizmo_draw_time_, LoopMode::kLoopModeOff));

  active_text_gizmo_ = text;
  connect(active_text_gizmo_, &TextGizmo::RectChanged, this, &ViewerDisplayWidget::UpdateActiveTextGizmoSize);
  text_transform_ = GenerateGizmoTransform();
  text_transform_inverted_ = text_transform_.inverted();

  // Create text editor
  text_edit_ = new ViewerTextEditor(text_transform_.m11(), this);

  // Set text editor's gizmo property for later use
  text_edit_->setProperty("gizmo", reinterpret_cast<quintptr>(text));

  // Install ourselves as event filter so we can receive the text editor's paint events
  text_edit_->installEventFilter(this);

  // Disable focus on text editor
  text_edit_->setFocusPolicy(Qt::NoFocus);

  // Disable mouse events on text editor
  text_edit_->setAttribute(Qt::WA_TransparentForMouseEvents);

  // "Show" text editor so that it throws paint events, even though its paint event is disabled
  text_edit_->show();

  // Convert HTML to Qt document
  Html::HtmlToDoc(text_edit_->document(), text->GetHtml());

  // Connect text change event to propagate back to node
  connect(text_edit_, &ViewerTextEditor::textChanged, this, &ViewerDisplayWidget::TextEditChanged);

  // Connect destroyed signal to cleanup after destruction
  connect(text_edit_, &ViewerTextEditor::destroyed, this, &ViewerDisplayWidget::TextEditDestroyed);

  // Set text editor's size to logical size
  QRectF text_rect = UpdateActiveTextGizmoSize();

  // Emit text gizmo activation signal
  emit text->Activated();

  // Create toolbar
  text_toolbar_ = new ViewerTextEditorToolBar(text_edit_);
  text_toolbar_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
  connect(text_toolbar_, &ViewerTextEditorToolBar::VerticalAlignmentChanged, text, &TextGizmo::SetVerticalAlignment);
  connect(text, &TextGizmo::VerticalAlignmentChanged, text_toolbar_, &ViewerTextEditorToolBar::SetVerticalAlignment);
  text_toolbar_->SetVerticalAlignment(text->GetVerticalAlignment());
  text_edit_->ConnectToolBar(text_toolbar_);

  QPoint toolbar_pos = mapToGlobal(text_transform_.map(text_edit_pos_).toPoint());
  if (QScreen *screen = qApp->screenAt(toolbar_pos)) {
    // Determine whether to anchor to the top of the rect of the bottom
    if (toolbar_pos.y() - text_toolbar_->height() >= screen->geometry().top()) {
      toolbar_pos.setY(toolbar_pos.y() - text_toolbar_->height());
    } else {
      toolbar_pos.setY(toolbar_pos.y() + text_transform_.map(text_rect).boundingRect().height());
    }

    // Clamp X
    if (toolbar_pos.x() + text_toolbar_->width() > screen->geometry().right()) {
      toolbar_pos.setX(screen->geometry().right() - text_toolbar_->width());
    }

    // Clamp Y
    if (toolbar_pos.y() + text_toolbar_->height() > screen->geometry().bottom()) {
      toolbar_pos.setY(screen->geometry().bottom() - text_toolbar_->height());
    }
  } else {
    // Fallback
    toolbar_pos.setY(toolbar_pos.y() - text_toolbar_->height());
  }

  text_toolbar_->move(toolbar_pos);
  text_toolbar_->show();

  // Allow widget to take keyboard focus
  inner_widget()->setFocusPolicy(Qt::StrongFocus);
  inner_widget()->setMouseTracking(true);

  connect(qApp, &QApplication::focusChanged, this, &ViewerDisplayWidget::FocusChanged);

  // Start text cursor where the user clicked
  if (event) {
    QPoint click_pos = text_transform_inverted_.map(event->pos()) - text_edit_pos_.toPoint();
    text_edit_->setTextCursor(text_edit_->cursorForPosition(click_pos));
  }

  // Grab focus back from the toolbar
  connect(text_toolbar_, &ViewerTextEditorToolBar::FirstPaint, this, [this]{
    Core::instance()->main_window()->activateWindow();
    inner_widget()->setFocus();
  });
}

bool ViewerDisplayWidget::OnMousePress(QMouseEvent *event)
{
  if (IsHandDrag(event)) {

    // Handle hand drag
    hand_last_drag_pos_ = event->pos();
    hand_dragging_ = true;
    emit HandDragStarted();
    inner_widget()->setCursor(Qt::ClosedHandCursor);

    return true;

  } else if (text_edit_ && ForwardMouseEventToTextEdit(event, true)) {

    return true;

  } else if (event->button() == Qt::LeftButton) {

    if (Core::instance()->tool() == Tool::kAdd
        && (Core::instance()->GetSelectedAddableObject() == Tool::kAddableShape || Core::instance()->GetSelectedAddableObject() == Tool::kAddableTitle)) {

      add_band_start_ = event->pos();
      add_band_end_ = add_band_start_;
      add_band_ = true;

    } else if ((current_gizmo_ = TryGizmoPress(gizmo_db_, gizmo_last_draw_transform_inverted_.map(event->pos())))) {

      // Handle gizmo click
      gizmo_start_drag_ = event->pos();
      gizmo_last_drag_ = gizmo_start_drag_;
      current_gizmo_->SetGlobals(NodeGlobals(gizmo_params_, gizmo_audio_params_, GenerateGizmoTime(), LoopMode::kLoopModeOff));

    } else {

      // Handle standard drag
      emit DragStarted(event->pos());

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

  } else if (text_edit_ && ForwardMouseEventToTextEdit(event)) {

    return true;

  } else if (add_band_) {

    add_band_end_ = event->pos();
    update();
    return true;

  } else if (current_gizmo_) {

    // Signal movement
    if (DraggableGizmo *draggable = dynamic_cast<DraggableGizmo*>(current_gizmo_)) {
      if (!gizmo_drag_started_) {
        QPointF start = ScreenToScenePoint(gizmo_start_drag_);

        rational gizmo_time = GetGizmoTime();
        NodeTraverser t;
        t.SetCacheVideoParams(gizmo_params_);
        t.SetCacheAudioParams(gizmo_audio_params_);
        NodeValueRow row = t.GenerateRow(gizmos_, TimeRange(gizmo_time, gizmo_time + gizmo_params_.frame_rate_as_time_base()));

        draggable->DragStart(row, start.x(), start.y(), gizmo_time);
        gizmo_drag_started_ = true;
      }

      QPointF v = ScreenToScenePoint(event->pos());
      switch (draggable->GetDragValueBehavior()) {
      case DraggableGizmo::kAbsolute:
        // Above value is correct
        break;
      case DraggableGizmo::kDeltaFromPrevious:
        v -= ScreenToScenePoint(gizmo_last_drag_);
        gizmo_last_drag_ = event->pos();
        break;
      case DraggableGizmo::kDeltaFromStart:
        v -= ScreenToScenePoint(gizmo_start_drag_);
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

  } else if (text_edit_ && ForwardMouseEventToTextEdit(e)) {

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
      Core::instance()->undo_stack()->push(command, tr("Dragged Gizmo"));
      gizmo_drag_started_ = false;
    }
    current_gizmo_ = nullptr;

    return true;

  }

  return false;
}

bool ViewerDisplayWidget::OnMouseDoubleClick(QMouseEvent *event)
{
  if (text_edit_ && ForwardMouseEventToTextEdit(event)) {
    return true;
  } else if (event->button() == Qt::LeftButton && gizmos_) {
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

bool ViewerDisplayWidget::OnKeyPress(QKeyEvent *e)
{
  if (text_edit_) {
    if (e->key() == Qt::Key_Escape) {
      CloseTextEditor();
      return true;
    } else {
      return ForwardEventToTextEdit(e);
    }
  }
  return false;
}

bool ViewerDisplayWidget::OnKeyRelease(QKeyEvent *e)
{
  if (text_edit_) {
    return ForwardEventToTextEdit(e);
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

  f.setWeight(static_cast<QFont::Weight>(OLIVE_CONFIG("DefaultSubtitleWeight").toInt()));

  bounding_box.adjust(bounding_box.width()/10, bounding_box.height()/10, -bounding_box.width()/10, -bounding_box.height()/10);

  QFontMetrics fm(f);

  for (int j=subtitle_tracklist.size()-1; j>=0; j--) {
    Track *sub_track = subtitle_tracklist.at(j);
    if (!sub_track->IsMuted()) {
      if (SubtitleBlock *sub = dynamic_cast<SubtitleBlock*>(sub_track->VisibleBlockAtTime(time_))) {
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

template <typename T>
void ViewerDisplayWidget::ForwardDragEventToTextEdit(T *e)
{
  // HACK: Absolutely filthy hack. We need to be able to transform the mouse coordinates for our
  //       proxied QTextEdit, however unlike QMouseEvents, Qt's drag events don't allow modifying
  //       the position after construction. Unhelpfully, Qt also explicitly forbids users creating
  //       their own drag events because they "rely on Qt's internal state". So in order to forward
  //       drag events, we defy this by creating our own events, but DON'T process them through Qt's
  //       event queue and instead just send them directly to the widget (requiring its protected
  //       drag events to be made public). That way Qt stays happy, because as far as it's
  //       concerned it's only interfacing with this widget, and the QTextEdit gets to receive
  //       transformed events. It's a terrible hack, but seems to work.

  if constexpr (std::is_same_v<T, QDragLeaveEvent>) {
    text_edit_->dragLeaveEvent(e);
  } else {

    T relay(AdjustPosByVAlign(GetVirtualPosForTextEdit(e->pos())).toPoint(),
            e->possibleActions(),
            e->mimeData(),
            e->mouseButtons(),
            e->keyboardModifiers());

    if (e->type() == QEvent::DragEnter) {
      text_edit_->dragEnterEvent(static_cast<QDragEnterEvent*>(&relay));
    } else if (e->type() == QEvent::DragMove) {
      text_edit_->dragMoveEvent(static_cast<QDragMoveEvent*>(&relay));
    } else if (e->type() == QEvent::Drop) {
      text_edit_->dropEvent(&relay);
    }

    if (relay.isAccepted()) {
      e->accept();
    }
  }
}

bool ViewerDisplayWidget::ForwardMouseEventToTextEdit(QMouseEvent *event, bool check_if_outside)
{
  if (current_gizmo_) {
    return false;
  }

  // Transform screen mouse coords to world mouse coords
  QPointF local_pos = GetVirtualPosForTextEdit(event->pos());

  if (event->type() == QEvent::MouseMove && event->buttons() == Qt::NoButton) {
    QPointF mapped = text_transform_inverted_.map(event->pos()) - text_edit_pos_;
    if (mapped.x() >= 0 && mapped.y() >= 0 && mapped.x() < text_edit_->width() && mapped.y() < text_edit_->height()) {
      inner_widget()->setCursor(Qt::IBeamCursor);
    } else {
      inner_widget()->unsetCursor();
    }
  }

  if (check_if_outside) {
    if (local_pos.x() < 0 || local_pos.x() >= text_edit_->width() || local_pos.y() < 0 || local_pos.y() >= text_edit_->height()) {
      // Allow clicking other gizmos so the user can resize while the text editor is active
      if ((current_gizmo_ = TryGizmoPress(gizmo_db_, gizmo_last_draw_transform_inverted_.map(event->pos())))) {
        return false;
      } else {
        CloseTextEditor();
        return true;
      }
    }
  }

  local_pos = AdjustPosByVAlign(local_pos);

  QMouseEvent derived(event->type(), local_pos, event->windowPos(), event->screenPos(), event->button(), event->buttons(), event->modifiers(), event->source());
  return ForwardEventToTextEdit(&derived);
}

bool ViewerDisplayWidget::ForwardEventToTextEdit(QEvent *event)
{
  qApp->sendEvent(text_edit_->viewport(), event);
  bool e = event->isAccepted();
  if (e) {
    update();
  }
  return e;
}

QPointF ViewerDisplayWidget::AdjustPosByVAlign(QPointF p)
{
  switch (active_text_gizmo_->GetVerticalAlignment()) {
  case Qt::AlignTop:
    // Do nothing
    break;
  case Qt::AlignVCenter:
    p.setY(p.y() - text_edit_->height()/2 + text_edit_->document()->size().height()/2);
    break;
  case Qt::AlignBottom:
    p.setY(p.y() - text_edit_->height() + text_edit_->document()->size().height());
    break;
  }

  return p;
}

void ViewerDisplayWidget::CloseTextEditor()
{
  text_edit_->deleteLater();
  text_edit_ = nullptr;

  disconnect(active_text_gizmo_, &TextGizmo::RectChanged, this, &ViewerDisplayWidget::UpdateActiveTextGizmoSize);
  active_text_gizmo_ = nullptr;
}

void ViewerDisplayWidget::GenerateGizmoTransforms()
{
  NodeTraverser gt;
  gt.SetCacheVideoParams(gizmo_params_);
  gt.SetCacheAudioParams(gizmo_audio_params_);

  gizmo_draw_time_ = GenerateGizmoTime();

  if (gizmos_) {
    gizmo_db_ = gt.GenerateRow(gizmos_, gizmo_draw_time_);
  }

  gizmo_last_draw_transform_ = GenerateGizmoTransform(gt, gizmo_draw_time_);
  gizmo_last_draw_transform_inverted_ = gizmo_last_draw_transform_.inverted();
}

void ViewerDisplayWidget::DrawBlank(const VideoParams &device_params)
{
  if (blank_shader_.isNull()) {
    blank_shader_ = renderer()->CreateNativeShader(ShaderCode());
  }

  ShaderJob job;
  job.Insert(QStringLiteral("ove_mvpmat"), NodeValue(NodeValue::kMatrix, combined_matrix_flipped_));
  job.Insert(QStringLiteral("ove_cropmatrix"), NodeValue(NodeValue::kMatrix, crop_matrix_));

  renderer()->Blit(blank_shader_, job, device_params, false);
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

void ViewerDisplayWidget::Play(const int64_t &start_timestamp, const int &playback_speed, const rational &timebase, bool start_updating)
{
  playback_timebase_ = timebase;
  playback_speed_ = playback_speed;

  timer_.Start(start_timestamp, playback_speed, timebase.toDouble());

  if (start_updating) {
    connect(this, &ViewerDisplayWidget::frameSwapped, this, &ViewerDisplayWidget::UpdateFromQueue);

    update();
  }
}

void ViewerDisplayWidget::Pause()
{
  disconnect(this, &ViewerDisplayWidget::frameSwapped, this, &ViewerDisplayWidget::UpdateFromQueue);

  queue_.clear();
  queue_starved_ = false;
}

QPointF ViewerDisplayWidget::ScreenToScenePoint(const QPoint &p)
{
  if (gizmo_last_draw_transform_.isIdentity()) {
    GenerateGizmoTransforms();
  }

  return p * gizmo_last_draw_transform_inverted_;
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
  text_edit_ = nullptr;
  text_toolbar_ = nullptr;
  inner_widget()->setMouseTracking(false);
  inner_widget()->setFocusPolicy(Qt::NoFocus);
  UpdateCursor();
  disconnect(qApp, &QApplication::focusChanged, this, &ViewerDisplayWidget::FocusChanged);
}

void ViewerDisplayWidget::SubtitlesChanged(const TimeRange &r)
{
  if (time_ >= r.in() && time_ < r.out()) {
    update();
  }
}

void ViewerDisplayWidget::FocusChanged(QWidget *old, QWidget *now)
{
  if (!now) {
    // Ignore this
    return;
  }

  bool unfocused = true;

  while (now) {
    if (now == text_toolbar_ || now == this) {
      unfocused = false;
      break;
    } else {
      now = now->parentWidget();
    }
  }

  if (unfocused) {
    CloseTextEditor();
  }
}

QRectF ViewerDisplayWidget::UpdateActiveTextGizmoSize()
{
  QRectF text_rect = active_text_gizmo_->GetRect();
  text_edit_pos_ = text_rect.topLeft();
  text_edit_->setGeometry(text_rect.toRect());
  return text_rect;
}

}
