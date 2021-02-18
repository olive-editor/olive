/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#ifndef MANAGEDDISPLAYOBJECT_H
#define MANAGEDDISPLAYOBJECT_H

#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOpenGLWidget>

#include "render/colormanager.h"
#include "render/renderer.h"
#include "widget/menu/menu.h"

namespace olive {

class ManagedDisplayWidgetOpenGL : public QOpenGLWidget
{
  Q_OBJECT
public:
  ManagedDisplayWidgetOpenGL(QWidget* parent = nullptr) :
    QOpenGLWidget(parent)
  {
  }

signals:
  void OnInit();

  void OnPaint();

  void OnDestroy();

  void OnMouseMove(QMouseEvent* e);

protected:
  virtual void initializeGL() override
  {
    connect(context(), &QOpenGLContext::aboutToBeDestroyed,
            this, &ManagedDisplayWidgetOpenGL::OnDestroy);

    emit OnInit();
  }

  virtual void paintGL() override
  {
    emit OnPaint();
  }

  virtual void mouseMoveEvent(QMouseEvent* e) override
  {
    emit OnMouseMove(e);

    QOpenGLWidget::mouseMoveEvent(e);
  }

private slots:
  void DestroyListener()
  {
    makeCurrent();

    emit OnDestroy();

    doneCurrent();
  }

};

class ManagedDisplayWidget : public QWidget
{
  Q_OBJECT
public:
  ManagedDisplayWidget(QWidget* parent = nullptr);

  virtual ~ManagedDisplayWidget() override;

  /**
   * @brief Disconnect a ColorManager (equivalent to ConnectColorManager(nullptr))
   */
  void DisconnectColorManager();

  /**
   * @brief Access currently connected ColorManager (nullptr if none)
   */
  ColorManager* color_manager() const;

  /**
   * @brief Get current color transform
   */
  const ColorTransform& GetColorTransform() const;

  /**
   * @brief Get menu that can be used to select the colorspace
   */
  Menu* GetColorSpaceMenu(QMenu* parent, bool auto_connect = true);

  /**
   * @brief Get menu that can be used to select the display transform
   */
  Menu* GetDisplayMenu(QMenu* parent, bool auto_connect = true);

  /**
   * @brief Get menu that can be used to select the view transform
   */
  Menu* GetViewMenu(QMenu* parent, bool auto_connect = true);

  /**
   * @brief Get menu that can be used to select the look transform
   */
  Menu* GetLookMenu(QMenu* parent, bool auto_connect = true);

  /**
   * @brief Passes update signal through to inner widget
   */
  void update();

public slots:
  /**
   * @brief Replaces the color transform with a new one
   */
  void SetColorTransform(const ColorTransform& transform);

  /**
   * @brief Connect a ColorManager (ColorManagers usually belong to the Project)
   */
  void ConnectColorManager(ColorManager* color_manager);

signals:
  /**
   * @brief Emitted when the color processor changes
   */
  void ColorProcessorChanged(ColorProcessorPtr processor);

  /**
   * @brief Emitted when a new color manager is connected
   */
  void ColorManagerChanged(ColorManager* color_manager);

  void frameSwapped();

  void InnerWidgetMouseMove(QMouseEvent* event);

protected:
  /**
   * @brief Provides access to the color processor (nullptr if none is set)
   */
  ColorProcessorPtr color_service();

  /**
   * @brief Enables a context menu that allows simple access to the DVL pipeline
   */
  void EnableDefaultContextMenu();

  /**
   * @brief Function called whenever the processor changes
   *
   * Default functionality is just to call update()
   */
  virtual void ColorProcessorChangedEvent();

  Renderer* renderer() const
  {
    return attached_renderer_;
  }

  void makeCurrent();

  void doneCurrent();

  QWidget* inner_widget() const
  {
    return inner_widget_;
  }

protected slots:
  /**
   * @brief Called whenever the internal rendering context has been created
   */
  virtual void OnInit();

  /**
   * @brief Called while the internal rendering context is being rendered
   */
  virtual void OnPaint() = 0;

  /**
   * @brief Called just before the internal rendering context is destroyed
   */
  virtual void OnDestroy();

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
   * @brief Main drawing surface abstraction
   */
  QWidget* inner_widget_;

  /**
   * @brief Renderer abstraction
   */
  Renderer* attached_renderer_;

  /**
   * @brief Connected color manager
   */
  ColorManager* color_manager_;

  /**
   * @brief Color management service
   */
  ColorProcessorPtr color_service_;

  /**
   * @brief Internal color transform storage
   */
  ColorTransform color_transform_;

private slots:
  /**
   * @brief Sets all color settings to the defaults pertaining to this configuration
   */
  void ColorConfigChanged();

  void ColorManagerValueChanged(const NodeInput &input, const TimeRange &range);

  /**
   * @brief The default context menu shown
   */
  void ShowDefaultContextMenu();

  /**
   * @brief If GetDisplayMenu() is called with `auto_connect` set to true, it will be connected to this
   */
  void MenuDisplaySelect(QAction* action);

  /**
   * @brief If GetViewMenu() is called with `auto_connect` set to true, it will be connected to this
   */
  void MenuViewSelect(QAction* action);

  /**
   * @brief If GetLookMenu() is called with `auto_connect` set to true, it will be connected to this
   */
  void MenuLookSelect(QAction* action);

  /**
   * @brief If GetColorSpaceMenu() is called with `auto_connect` set to true, it will be connected to this
   */
  void MenuColorspaceSelect(QAction* action);

};

}

#endif // MANAGEDDISPLAYOBJECT_H
