#ifndef PIXELSAMPLERPANEL_H
#define PIXELSAMPLERPANEL_H

#include "widget/panel/panel.h"
#include "widget/viewer/pixelsamplerwidget.h"

class PixelSamplerPanel : public PanelWidget
{
  Q_OBJECT
public:
  PixelSamplerPanel(QWidget* parent = nullptr);

public slots:
  void SetValues(const Color& reference, const Color& display);

private:
  virtual void Retranslate() override;

  ManagedPixelSamplerWidget* sampler_widget_;

};

#endif // PIXELSAMPLERPANEL_H
