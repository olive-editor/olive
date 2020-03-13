#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include <QWidget>

#include "audio/sumsamples.h"
#include "render/audioparams.h"
#include "render/backend/audiorenderbackend.h"
#include "widget/timelinewidget/timelinescaledobject.h"

class WaveformView : public TimelineScaledWidget
{
  Q_OBJECT
public:
  WaveformView(QWidget* parent = nullptr);

  //void SetData(const QString& file, const AudioRenderingParams& params);

  void SetBackend(AudioRenderBackend* backend);

  static void DrawWaveform(QPainter* painter, const QRect &rect, const double &scale, const SampleSummer::Sum *samples, int nb_samples, int channels);

public slots:
  void SetScroll(int scroll);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

  virtual void ScaleChangedEvent(const double& s) override;

private:
  int GetSampleIndexFromPixel(int x) const;

  AudioRenderBackend* backend_;

  int scroll_;

};

#endif // WAVEFORMVIEW_H
