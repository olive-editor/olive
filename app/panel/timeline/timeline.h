#ifndef TIMELINE_H
#define TIMELINE_H

#include "widget/panel/panel.h"

class TimelinePanel : public PanelWidget
{
  Q_OBJECT
public:
  TimelinePanel(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();
};

#endif // TIMELINE_H
