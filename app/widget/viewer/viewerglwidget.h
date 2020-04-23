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

#ifndef VIEWERGLWIDGET_H
#define VIEWERGLWIDGET_H

#include <QOpenGLWidget>

#include "render/backend/opengl/openglcolorprocessor.h"
#include "render/backend/opengl/openglframebuffer.h"
#include "render/backend/opengl/openglshader.h"
#include "render/backend/opengl/opengltexture.h"
#include "render/color.h"
#include "render/colormanager.h"
#include "viewersafemargininfo.h"
#include "widget/manageddisplay/manageddisplay.h"

OLIVE_NAMESPACE_ENTER

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
class ViewerGLWidget : public ManagedDisplayWidget
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
  ViewerGLWidget(QWidget* parent = nullptr);

  virtual ~ViewerGLWidget() override;

  /**
   * @brief Set an image to load and display on screen
   */
  void SetImage(const QString& fn);

  const QMatrix4x4& GetMatrix();

  void ConnectSibling(ViewerGLWidget* sibling);

  const ViewerSafeMarginInfo& GetSafeMargin() const;
  void SetSafeMargins(const ViewerSafeMarginInfo& safe_margin);

public slots:
  /**
   * @brief Set the transformation matrix to draw with
   *
   * Set this if you want the drawing to pass through some sort of transform (most of the time you won't want this).
   */
  void SetMatrix(const QMatrix4x4& mat);

  /**
   * @brief Enables or disables whether this color at the cursor should be emitted
   *
   * Since tracking the mouse every movement, reading pixels, and doing color transforms are processor intensive, we
   * have an option for it. Ideally, this should be connected to a PixelSamplerPanel::visibilityChanged signal so that
   * it can automatically be enabled when the user is pixel sampling and disabled for optimization when they're not.
   */
  void SetSignalCursorColorEnabled(bool e);

  /**
   * @brief Overrides the image with the load buffer of another ViewerGLWidget
   *
   * If there are multiple ViewerGLWidgets showing the same thing, this is faster than decoding the image from file
   * each time.
   */
  void SetImageFromLoadBuffer(Frame* in_buffer);

  /**
   * @brief Enables or disables DrewManagedTexture()
   *
   * To emit a display referred texture, it needs to be copied after the color transform is complete. This naturally
   * adds extra GPU cycles that are wasted if there's nothing receiving the signal. Therefore, the signal is disabled
   * by default.
   */
  void SetEmitDrewManagedTextureEnabled(bool e);

signals:
  /**
   * @brief Signal emitted when the user starts dragging from the viewer
   */
  void DragStarted();

  /**
   * @brief Signal emitted when cursor color is enabled and the user's mouse position changes
   */
  void CursorColor(const Color& reference, const Color& display);

  /**
   * @brief Signal emitted when a buffer is loaded from file into memory
   *
   * This buffer will be the direct output of the renderer in reference space in CPU memory.
   *
   * Connect this to the SetImageFromLoadBuffer() slot of another ViewerGLWidget to show the same thing
   */
  void LoadedBuffer(Frame* load_buffer);

  /**
   * @brief Signal emitted when a buffer is loaded into a texture
   *
   * This texture will be the direct output of the renderer in reference space in GPU VRAM.
   */
  void LoadedTexture(OpenGLTexture* texture);

  /**
   * @brief Emitted when the a texture has been transformed to display
   */
  void DrewManagedTexture(OpenGLTexture* texture);

protected:
  /**
   * @brief Override the mouse press event simply to emit the DragStarted() signal
   */
  virtual void mousePressEvent(QMouseEvent* event) override;

  /**
   * @brief Override mouse move to provide functionality for
   */
  virtual void mouseMoveEvent(QMouseEvent* event) override;

  /**
   * @brief Initialize function to set up the OpenGL context upon its construction
   *
   * Currently primarily used to regenerate the pipeline shader used for drawing.
   */
  virtual void initializeGL() override;

  /**
   * @brief Paint function to display the texture (received in SetTexture()) on screen.
   *
   * Simple OpenGL drawing function for painting the texture on screen. Standardized around OpenGL ES 3.2 Core.
   */
  virtual void paintGL() override;

private:
  /**
   * @brief Internal reference to the OpenGL texture to draw. Set in SetTexture() and used in paintGL().
   */
  OpenGLTexture texture_;

  /**
   * @brief Internal framebuffer used to draw to managed_texture_
   */
  OpenGLFramebuffer framebuffer_;

  /**
   * @brief Internal referenceto the OpenGL texture that's been managed
   *
   * Kept so that scopes can use the display-referred buffer without having to transform again.
   */
  OpenGLTexture managed_texture_;

  /**
   * @brief Pipeline used to draw to managed_texture_
   */
  OpenGLShaderPtr managed_copy_pipeline_;

  /**
   * @brief Drawing matrix (defaults to identity)
   */
  QMatrix4x4 matrix_;

  /**
   * @brief Buffer to load images into RAM before sending them to the display
   */
  Frame load_buffer_;

#ifdef Q_OS_LINUX
  static bool nouveau_check_done_;
#endif

  bool has_image_;

  bool signal_cursor_color_;

  ViewerSafeMarginInfo safe_margin_;

  bool enable_display_referred_signal_;

private slots:
  /**
   * @brief Slot to connect just before the OpenGL context is destroyed to clean up resources
   */
  void ContextCleanup();

#ifdef Q_OS_LINUX
  /**
   * @brief Shows warning messagebox if Nouveau is detected
   */
  void ShowNouveauWarning();
#endif

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERGLWIDGET_H
