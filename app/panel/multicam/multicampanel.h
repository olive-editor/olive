#ifndef MULTICAMPANEL_H
#define MULTICAMPANEL_H

#include "panel/viewer/viewerbase.h"
#include "widget/multicam/multicamwidget.h"

namespace olive {

class MulticamPanel : public TimeBasedPanel
{
  Q_OBJECT
public:
  MulticamPanel(QWidget* parent = nullptr);

  MulticamWidget *GetMulticamWidget() const { return static_cast<MulticamWidget *>(GetTimeBasedWidget()); }

  void SetMulticamNode(MultiCamNode *n)
  {
    GetMulticamWidget()->SetMulticamNode(n);
  }

  void SetClip(ClipBlock* clip)
  {
    GetMulticamWidget()->SetClip(clip);
  }

protected:
  virtual void Retranslate() override;

};

}

#endif // MULTICAMPANEL_H
