#include "audiomonitor.h"

AudioMonitorPanel::AudioMonitorPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("AudioMonitor");

  audio_monitor_ = new AudioMonitor(this);

  setWidget(audio_monitor_);

  Retranslate();
}

void AudioMonitorPanel::Retranslate()
{
  SetTitle(tr("Audio Monitor"));
}
