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

#include "audiomonitor.h"

#include "panel/panelmanager.h"

namespace olive {

#define super PanelWidget

AudioMonitorPanel::AudioMonitorPanel(QWidget *parent) :
  super(QStringLiteral("AudioMonitor"), parent)
{
  audio_monitor_ = new AudioMonitor();

  audio_monitor_->installEventFilter(this);

  setWidget(QWidget::createWindowContainer(audio_monitor_));

  Retranslate();
}

bool AudioMonitorPanel::eventFilter(QObject *o, QEvent *e)
{
  if (o == audio_monitor_ && e->type() == QEvent::FocusIn) {
    // HACK: QWindow focus isn't accounted for in QApplication::focusChanged, so we handle it
    //       manually here.
    PanelManager::instance()->FocusChanged(nullptr, this);
  }

  return super::eventFilter(o, e);
}

void AudioMonitorPanel::Retranslate()
{
  SetTitle(tr("Audio Monitor"));
}

}
