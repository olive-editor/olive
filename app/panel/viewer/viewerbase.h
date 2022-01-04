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

#ifndef VIEWERPANELBASE_H
#define VIEWERPANELBASE_H

#include "panel/pixelsampler/pixelsamplerpanel.h"
#include "panel/timebased/timebased.h"
#include "widget/viewer/viewer.h"

namespace olive {

class ViewerPanelBase : public TimeBasedPanel
{
  Q_OBJECT
public:
  ViewerPanelBase(const QString& object_name, QWidget* parent = nullptr);

  virtual void PlayPause() override;

  virtual void PlayInToOut() override;

  virtual void ShuttleLeft() override;

  virtual void ShuttleStop() override;

  virtual void ShuttleRight() override;

  void ConnectTimeBasedPanel(TimeBasedPanel* panel);

  void DisconnectTimeBasedPanel(TimeBasedPanel* panel);

  /**
   * @brief Wrapper for ViewerWidget::SetFullScreen()
   */
  void SetFullScreen(QScreen* screen = nullptr);

  ColorManager *GetColorManager()
  {
    return static_cast<ViewerWidget*>(GetTimeBasedWidget())->color_manager();
  }

  void UpdateTextureFromNode()
  {
    static_cast<ViewerWidget*>(GetTimeBasedWidget())->UpdateTextureFromNode();
  }

public slots:
  void SetGizmos(Node* node);

  void CacheEntireSequence();

  void CacheSequenceInOut();

signals:
  /**
   * @brief Signal emitted when a new frame is loaded
   */
  void TextureChanged(TexturePtr t);

  /**
   * @brief Wrapper for ViewerGLWidget::ColorProcessorChanged()
   */
  void ColorProcessorChanged(ColorProcessorPtr processor);

  /**
   * @brief Wrapper for ViewerGLWidget::ColorManagerChanged()
   */
  void ColorManagerChanged(ColorManager* color_manager);

protected:
  void SetViewerWidget(ViewerWidget *vw);

};

}

#endif // VIEWERPANELBASE_H
