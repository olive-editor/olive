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

#ifndef AUDIOMONITORPANEL_H
#define AUDIOMONITORPANEL_H

#include "widget/audiomonitor/audiomonitor.h"
#include "widget/panel/panel.h"

namespace olive {

/**
 * @brief PanelWidget wrapper around an AudioMonitor
 */
class AudioMonitorPanel : public PanelWidget
{
  Q_OBJECT
public:
  AudioMonitorPanel(QWidget* parent = nullptr);

  bool IsPlaying() const
  {
    return audio_monitor_->IsPlaying();
  }

  void SetParams(const AudioParams& params)
  {
    audio_monitor_->SetParams(params);
  }

private:
  virtual void Retranslate() override;

  AudioMonitor* audio_monitor_;

};

}

#endif // AUDIOMONITORPANEL_H
