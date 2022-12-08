#include "multicampanel.h"

namespace olive {

#define super TimeBasedPanel

MulticamPanel::MulticamPanel(QWidget *parent) :
  super(QStringLiteral("MultiCamPanel"), parent)
{
  SetTimeBasedWidget(new MulticamWidget());

  Retranslate();
}

void MulticamPanel::Retranslate()
{
  super::Retranslate();

  SetTitle(tr("Multi-Cam"));
}

}
