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

protected:
  virtual void Retranslate() override;

};

}

#endif // MULTICAMPANEL_H
