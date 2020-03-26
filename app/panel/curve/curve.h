#ifndef CURVEPANEL_H
#define CURVEPANEL_H

#include "panel/timebased/timebased.h"
#include "widget/curvewidget/curvewidget.h"

class CurvePanel : public TimeBasedPanel
{
  Q_OBJECT
public:
  CurvePanel(QWidget* parent);

public slots:
  void SetInput(NodeInput* input);

  void SetTimeTarget(Node* target);

  virtual void IncreaseTrackHeight() override;

  virtual void DecreaseTrackHeight() override;

protected:
  virtual void Retranslate() override;

};

#endif // CURVEPANEL_H
