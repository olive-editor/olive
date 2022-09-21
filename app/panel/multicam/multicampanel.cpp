#include "multicampanel.h"

namespace olive {

#define super PanelWidget

MulticamPanel::MulticamPanel(ViewerPanelBase *sibling, QWidget *parent) :
  super(QStringLiteral("MultiCamPanel"), parent)
{
  widget_ = new MulticamWidget();
  SetWidgetWithPadding(widget_);

  connect(sibling, &ViewerPanelBase::ColorManagerChanged, widget_, &MulticamWidget::ConnectColorManager);
  widget_->ConnectCacher(static_cast<ViewerWidget*>(sibling->GetTimeBasedWidget())->GetCacher());

  Retranslate();
}

void MulticamPanel::Retranslate()
{
  super::Retranslate();

  SetTitle(tr("Multi-Cam"));
}

}
