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

#ifndef MANAGEDDISPLAYOBJECT_H
#define MANAGEDDISPLAYOBJECT_H

#include <QOpenGLWidget>

#include "render/backend/opengl/openglcolorprocessor.h"
#include "render/colormanager.h"
#include "widget/menu/menu.h"

OLIVE_NAMESPACE_ENTER

class ManagedDisplayWidget : public QOpenGLWidget
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

protected:
  /**
   * @brief Provides access to the color processor (nullptr if none is set)
   */
  OpenGLColorProcessorPtr color_service();

  /**
   * @brief Override when setting up OpenGL context
   */
  virtual void initializeGL() override;

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
   * @brief Connected color manager
   */
  ColorManager* color_manager_;

  /**
   * @brief Color management service
   */
  OpenGLColorProcessorPtr color_service_;

  /**
   * @brief Internal color transform storage
   */
  ColorTransform color_transform_;

private slots:
  /**
   * @brief Sets all color settings to the defaults pertaining to this configuration
   */
  void ColorConfigChanged();

  /**
   * @brief Cleans up resources if context is about to be destroyed
   */
  void ContextCleanup();

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

OLIVE_NAMESPACE_EXIT

#endif // MANAGEDDISPLAYOBJECT_H
