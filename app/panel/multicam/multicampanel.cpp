#include "multicampanel.h"

namespace olive {

#define super TimeBasedPanel

MulticamPanel::MulticamPanel() :
  super(QStringLiteral("MultiCamPanel"))
{
  SetTimeBasedWidget(new MulticamWidget(this));

  Retranslate();
}

void MulticamPanel::Retranslate()
{
  super::Retranslate();

  SetTitle(tr("Multi-Cam"));
}

}
