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

#ifndef VIEWERGLWIDGET_H
#define VIEWERGLWIDGET_H

#include <QOpenGLWidget>
#include <QMatrix4x4>

#include "node/color/colormanager/colormanager.h"
#include "node/node.h"
#include "render/color.h"
#include "tool/tool.h"
#include "viewersafemargininfo.h"
#include "widget/manageddisplay/manageddisplay.h"
#include "widget/timetarget/timetarget.h"

namespace olive {

/**
 * @brief The inner display/rendering widget of a Viewer class.
 *
 * Actual composition occurs elsewhere offscreen and
 * multithreaded, so its main purpose is receiving a finalized OpenGL texture and displaying it.
 *
 * The main entry point is SetTexture() which will receive an OpenGL texture ID, store it, and then call update() to
 * draw it on screen. The drawing function is in paintGL() (called during the update() process by Qt) and is fairly
 * simple OpenGL drawing code standardized around OpenGL ES 3.2 Core.
 *
 * If the texture has been modified and you're 100% sure this widget is using the same texture object, it's possible
 * to call update() directly to trigger a repaint, however this is not recommended. If you are not 100% sure it'll be
 * the same texture object, use SetTexture() since it will nearly always be faster to just set it than to check *and*
 * set it.
 */
class ViewerDisplayWidget : public ManagedDisplayWidget, public TimeTargetObject
{
  Q_OBJECT
public:
  /**
   * @brief ViewerGLWidget Constructor
   *
   * @param parent
   *
   * QWidget parent.
   */
  ViewerDisplayWidget(QWidget* parent = nullptr);

  MANAGEDDISPLAYWIDGET_DEFAULT_DESTRUCTOR(ViewerDisplayWidget)

  const ViewerSafeMarginInfo& GetSafeMargin() const;
  void SetSafeMargins(const ViewerSafeMarginInfo& safe_margin);

  void SetGizmos(Node* node);
  void SetVideoParams(const VideoParams &params);
  void SetTime(const rational& time);

  void SetShowWidgetBackground(bool e)
  {
    show_widget_background_ = e;
    update();
  }

  /**
   * @brief Transform a point from viewer space to the buffer space.
   * Multiplies by the inverted transform matrix to undo the scaling and translation.
   */
  QPoint TransformViewerSpaceToBufferSpace(QPoint pos);

  bool IsDeinterlacing() const
  {
    return deinterlace_;
  }

  void ResetFPSTimer();

  bool GetShowFPS() const
  {
    return show_fps_;
  }

  void IncrementSkippedFrames();

  void IncrementFrameCount()
  {
    fps_timer_update_count_++;
  }

  TexturePtr GetCurrentTexture() const
  {
    return texture_;
  }

public slots:
  /**
   * @brief Set the transformation matrix to draw with
   *
   * Set this if you want the drawing to pass through some sort of transform (most of the time you won't want this).
   */
  void SetMatrixTranslate(const QMatrix4x4& mat);

  /**
  * @brief Set the scale matrix.
  */
  void SetMatrixZoom(const QMatrix4x4& mat);

  void SetMatrixCrop(const QMatrix4x4& mat);

  /**
   * @brief Enables or disables whether this color at the cursor should be emitted
   *
   * Since tracking the mouse every movement, reading pixels, and doing color transforms are processor intensive, we
   * have an option for it. Ideally, this should be connected to a PixelSamplerPanel::visibilityChanged signal so that
   * it can automatically be enabled when the user is pixel sampling and disabled for optimization when they're not.
   */
  void SetSignalCursorColorEnabled(bool e);

  void SetImage(const QVariant &buffer);

  void SetBlank();

  /**
   * @brief Changes the pointer type if the tool is changed to the hand tool. Otherwise resets the pointer to it's
   * normal type.
   */
  void UpdateCursor();

  /**
   * @brief Enables/disables a basic deinterlace on the viewer
   */
  void SetDeinterlacing(bool e);

  void SetShowFPS(bool e);

signals:
  /**
   * @brief Signal emitted when the user starts dragging from the viewer
   */
  void DragStarted();

  /**
   * @brief Signal emitted when a hand drag starts
   */
  void HandDragStarted();

  /**
   * @brief Signal emitted when a hand drag moves
   */
  void HandDragMoved(int x, int y);

  /**
   * @brief Signal emitted when a hand drag ends
   */
  void HandDragEnded();

  /**
   * @brief Signal emitted when cursor color is enabled and the user's mouse position changes
   */
  void CursorColor(const Color& reference, const Color& display);

  void DragEntered(QDragEnterEvent* event);

  void DragLeft(QDragLeaveEvent* event);

  void Dropped(QDropEvent* event);

  void VisibilityChanged(bool visible);

  void TextureChanged(TexturePtr texture);

protected:
  /**
   * @brief Override the mouse press event for the DragStarted() signal and gizmos
   */
  virtual void mousePressEvent(QMouseEvent* event) override;

  /**
   * @brief Override mouse move to signal for the pixel sampler and gizmos
   */
  virtual void mouseMoveEvent(QMouseEvent* event) override;

  /**
   * @brief Override mouse release event for gizmos
   */
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  virtual void dragEnterEvent(QDragEnterEvent* event) override;

  virtual void dragLeaveEvent(QDragLeaveEvent* event) override;

  virtual void dropEvent(QDropEvent* event) override;

  virtual void showEvent(QShowEvent* event) override;

  virtual void hideEvent(QHideEvent* event) override;

protected slots:
  /**
   * @brief Paint function to display the texture (received in SetTexture()) on screen.
   *
   * Simple OpenGL drawing function for painting the texture on screen. Standardized around OpenGL ES 3.2 Core.
   */
  virtual void OnPaint() override;

  virtual void OnDestroy() override;

private:
  QPointF GetTexturePosition(const QPoint& screen_pos);
  QPointF GetTexturePosition(const QSize& size);
  QPointF GetTexturePosition(const double& x, const double& y);

  static void DrawTextWithCrudeShadow(QPainter* painter, const QRect& rect, const QString& text);

  rational GetGizmoTime();

  bool IsHandDrag(QMouseEvent* event) const;

  void UpdateMatrix();

  QTransform GenerateWorldTransform();

  QTransform GenerateGizmoTransform();

  /**
   * @brief Internal reference to the OpenGL texture to draw. Set in SetTexture() and used in paintGL().
   */
  TexturePtr texture_;

  /**
   * @brief Internal texture to deinterlace to
   */
  TexturePtr deinterlace_texture_;

  /**
   * @brief Deinterlace shader
   */
  QVariant deinterlace_shader_;

  /**
   * @brief Blank shader
   */
  QVariant blank_shader_;

  /**
   * @brief Translation only matrix (defaults to identity).
   */
  QMatrix4x4 translate_matrix_;

  /**
   * @breif Scale only matrix.
   */
  QMatrix4x4 scale_matrix_;

  /**
   * @brief Crop only matrix
   */
  QMatrix4x4 crop_matrix_;

  /**
   * @brief Cached result of translate_matrix_ and scale_matrix_ multiplied
   */
  QMatrix4x4 combined_matrix_;
  QMatrix4x4 combined_matrix_flipped_;

  bool signal_cursor_color_;

  ViewerSafeMarginInfo safe_margin_;

  Node* gizmos_;
  NodeValueDatabase gizmo_db_;
  rational gizmo_drag_time_;
  VideoParams gizmo_params_;
  QPoint gizmo_start_drag_;
  bool gizmo_click_;

  rational time_;

  /**
   * @brief Position of mouse to calculate delta from.
   */
  QPoint hand_last_drag_pos_;
  bool hand_dragging_;

  bool deinterlace_;

  qint64 fps_timer_start_;
  int fps_timer_update_count_;

  bool show_fps_;
  int frames_skipped_;

  QVector<double> frame_rate_averages_;
  int frame_rate_average_count_;

  bool show_widget_background_;

  QVariant load_frame_;

  enum PushMode {
    /// New frame to push to internal texture
    kPushFrame,

    /// Internal texture reference is up to date, keep showing it
    kPushUnnecessary,

    /// Draw blank/black screen
    kPushBlank,

    /// Draw nothing (not even a black frame)
    kPushNull,
  };

  PushMode push_mode_;

private slots:
  void EmitColorAtCursor(QMouseEvent* e);

};

}

#endif // VIEWERGLWIDGET_H
