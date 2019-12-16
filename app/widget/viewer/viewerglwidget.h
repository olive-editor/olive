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

#include "render/colormanager.h"
#include "render/backend/opengl/openglshader.h"

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
class ViewerGLWidget : public QOpenGLWidget
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
   * @brief Deleted copy constructor
   */
  ViewerGLWidget(const ViewerGLWidget& other) = delete;

  /**
   * @brief Deleted move constructor
   */
  ViewerGLWidget(ViewerGLWidget&& other) = delete;

  /**
   * @brief Deleted copy assignment
   */
  ViewerGLWidget& operator=(const ViewerGLWidget& other) = delete;

  /**
   * @brief Deleted move assignment
   */
  ViewerGLWidget& operator=(ViewerGLWidget&& other) = delete;

  /**
   * @brief Enable or disable the color management menu
   *
   * While the Viewer is _always_ color managed, In some contexts, the color management may be controlled from an
   * external UI making the menu unnecessary.
   */
  void SetColorMenuEnabled(bool enabled);

public slots:
  /**
   * @brief Set the texture to draw and draw it
   *
   * Use this function to update the viewer.
   *
   * @param tex
   */
  void SetTexture(GLuint tex);

  void SetOCIOParameters(const QString& display, const QString& view, const QString& look);

  /**
   * @brief Externally set the OCIO display to use
   *
   * This value must be a valid display in the current OCIO configuration.
   */
  void SetOCIODisplay(const QString& display);

  /**
   * @brief Externally set the OCIO view to use
   *
   * This value must be a valid display in the current OCIO configuration.
   */
  void SetOCIOView(const QString& view);

  /**
   * @brief Externally set the OCIO look to use (use empty string if none)
   *
   * This value must be a valid display in the current OCIO configuration.
   */
  void SetOCIOLook(const QString& look);

protected:
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
   * @brief Call this if this user has selected a different display/view/look to recreate the processor
   */
  void SetupColorProcessor();

  /**
   * @brief Cleanup function
   */
  void ClearOCIOLutTexture();

  /**
   * @brief Internal variable to set color space to
   */
  QString ocio_display_;

  /**
   * @brief Internal variable to set color space to
   */
  QString ocio_view_;

  /**
   * @brief Internal variable to set color space to
   */
  QString ocio_look_;

  /**
   * @brief Internal reference to the OpenGL texture to draw. Set in SetTexture() and used in paintGL().
   */
  GLuint texture_;

  /**
   * @brief Internal shader object to use as the pipeline shader
   *
   * Retrieved every initializeGL() in order to stay up to date when new contexts are generated.
   */
  OpenGLShaderPtr pipeline_;

  /**
   * @brief OCIO LUT texture used for conversions
   */
  GLuint ocio_lut_;

  /**
   * @brief Color management service
   */
  ColorProcessorPtr color_service_;

  bool color_menu_enabled_;

private slots:
  /**
   * @brief Slot to connect just before the OpenGL context is destroyed to clean up resources
   */
  void ContextCleanup();

  /**
   * @brief Show context menu
   */
  void ShowContextMenu(const QPoint& pos);

  /**
   * @brief Sets all color settings to the defaults pertaining to this configuration
   */
  void RefreshColorPipeline();

  /**
   * @brief Slot called whenever this viewer's OCIO display setting has changed
   */
  void ColorDisplayChanged(QAction* action);

  /**
   * @brief Slot called whenever this viewer's OCIO view setting has changed
   */
  void ColorViewChanged(QAction* action);

  /**
   * @brief Slot called whenever this viewer's OCIO look setting has changed
   */
  void ColorLookChanged(QAction* action);

};

#endif // VIEWERGLWIDGET_H
