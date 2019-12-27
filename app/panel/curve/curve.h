#ifndef CURVEPANEL_H
#define CURVEPANEL_H

#include "widget/panel/panel.h"

class CurvePanel : public PanelWidget
{
public:
  CurvePanel(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();

};

#endif // CURVEPANEL_H
