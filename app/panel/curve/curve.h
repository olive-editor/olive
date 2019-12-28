#ifndef CURVEPANEL_H
#define CURVEPANEL_H

#include "widget/curvewidget/curvewidget.h"
#include "widget/panel/panel.h"

class CurvePanel : public PanelWidget
{
  Q_OBJECT
public:
  CurvePanel(QWidget* parent);

public slots:
  void SetInput(NodeInput* input);

  void SetTimebase(const rational& timebase);

  void SetTime(const int64_t& timestamp);

  virtual void ZoomIn() override;

  virtual void ZoomOut() override;

signals:
  void TimeChanged(const int64_t& timestamp);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();

  CurveWidget* widget_;

};

#endif // CURVEPANEL_H
