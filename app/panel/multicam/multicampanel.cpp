#include "multicampanel.h"

namespace olive {

#define super ViewerPanelBase

MulticamPanel::MulticamPanel(QWidget *parent) :
  super(QStringLiteral("MultiCamPanel"), parent)
{
  widget_ = new MulticamWidget();
  SetViewerWidget(widget_);

  Retranslate();
}

void MulticamPanel::Retranslate()
{
  super::Retranslate();

  SetTitle(tr("Multi-Cam"));
}

}
