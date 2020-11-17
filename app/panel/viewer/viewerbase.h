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

#ifndef VIEWERPANELBASE_H
#define VIEWERPANELBASE_H

#include "panel/pixelsampler/pixelsamplerpanel.h"
#include "panel/timebased/timebased.h"
#include "widget/viewer/viewer.h"

OLIVE_NAMESPACE_ENTER

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

  void ConnectPixelSamplerPanel(PixelSamplerPanel *psp);

  /**
   * @brief Wrapper for ViewerWidget::SetFullScreen()
   */
  void SetFullScreen(QScreen* screen = nullptr);

public slots:
  void SetGizmos(Node* node);

  void CacheEntireSequence();

  void CacheSequenceInOut();

protected:
  void CreateScopePanel(ScopePanel::Type type);

  virtual void closeEvent(QCloseEvent* e) override;

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERPANELBASE_H
