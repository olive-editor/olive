#ifndef VIEWERPANELBASE_H
#define VIEWERPANELBASE_H

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

  VideoRenderBackend* video_renderer() const;

};

#endif // VIEWERPANELBASE_H
