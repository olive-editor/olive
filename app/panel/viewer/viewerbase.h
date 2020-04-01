#ifndef VIEWERPANELBASE_H
#define VIEWERPANELBASE_H

#include "panel/pixelsampler/pixelsamplerpanel.h"
#include "panel/timebased/timebased.h"
#include "widget/viewer/viewer.h"

class ViewerPanelBase : public TimeBasedPanel
{
  Q_OBJECT
public:
  ViewerPanelBase(QWidget* parent = nullptr);

  virtual void PlayPause() override;

  virtual void ShuttleLeft() override;

  virtual void ShuttleStop() override;

  virtual void ShuttleRight() override;

  void ConnectTimeBasedPanel(TimeBasedPanel* panel);

  void DisconnectTimeBasedPanel(TimeBasedPanel* panel);

  VideoRenderBackend* video_renderer() const;

  void ConnectPixelSamplerPanel(PixelSamplerPanel *psp);

};

#endif // VIEWERPANELBASE_H
