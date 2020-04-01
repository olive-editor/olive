#include "pixelsamplerpanel.h"

PixelSamplerPanel::PixelSamplerPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // FIXME: This won't work if there's ever more than one of this panel
  setObjectName("ProjectPanel");

  sampler_widget_ = new ManagedPixelSamplerWidget();
  setWidget(sampler_widget_);

  Retranslate();
}

void PixelSamplerPanel::SetValues(const Color &reference, const Color &display)
{
  sampler_widget_->SetValues(reference, display);
}

void PixelSamplerPanel::Retranslate()
{
  SetTitle(tr("Pixel Sampler"));
}
