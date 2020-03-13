#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include "widget/timebased/timebased.h"

class WaveformView : public TimeBasedWidget
{
public:
  WaveformView(QWidget* parent = nullptr);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

};

#endif // WAVEFORMVIEW_H
