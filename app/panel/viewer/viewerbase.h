#ifndef VIEWERPANELBASE_H
#define VIEWERPANELBASE_H

#include "widget/panel/panel.h"
#include "widget/viewer/viewer.h"

class ViewerPanelBase : public PanelWidget
{
  Q_OBJECT
public:
  ViewerPanelBase(QWidget* parent = nullptr);

  virtual void ZoomIn() override;

  virtual void ZoomOut() override;

  virtual void GoToStart() override;

  virtual void PrevFrame() override;

  virtual void PlayPause() override;

  virtual void NextFrame() override;

  virtual void GoToEnd() override;

  virtual void ShuttleLeft() override;

  virtual void ShuttleStop() override;

  virtual void ShuttleRight() override;

protected:
  ViewerWidget* viewer_;

private:

};

#endif // VIEWERPANELBASE_H
