#ifndef MULTICAMPANEL_H
#define MULTICAMPANEL_H

#include "panel/viewer/viewerbase.h"
#include "widget/multicam/multicamwidget.h"
#include "widget/panel/panel.h"

namespace olive {

class MulticamPanel : public PanelWidget
{
  Q_OBJECT
public:
  MulticamPanel(ViewerPanelBase *sibling, QWidget* parent = nullptr);

  void SetNode(MultiCamNode *n)
  {
    widget_->SetNode(n);
  }

public slots:
  void SetTime(const rational &t)
  {
    widget_->SetTime(t);
  }

protected:
  virtual void Retranslate() override;

private:
  MulticamWidget *widget_;

};

}

#endif // MULTICAMPANEL_H
