#ifndef AUDIOMONITORPANEL_H
#define AUDIOMONITORPANEL_H

#include "widget/audiomonitor/audiomonitor.h"
#include "widget/panel/panel.h"

/**
 * @brief PanelWidget wrapper around an AudioMonitor
 */
class AudioMonitorPanel : public PanelWidget
{
public:
  AudioMonitorPanel(QWidget* parent = nullptr);

private:
  virtual void Retranslate() override;

  AudioMonitor* audio_monitor_;

};

#endif // AUDIOMONITORPANEL_H
