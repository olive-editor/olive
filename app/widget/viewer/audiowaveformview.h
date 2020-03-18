#ifndef WAVEFORMVIEW_H
#define WAVEFORMVIEW_H

#include <QWidget>

#include "audio/sumsamples.h"
#include "render/audioparams.h"
#include "render/backend/audiorenderbackend.h"
#include "widget/timeruler/seekablewidget.h"

class AudioWaveformView : public SeekableWidget
{
  Q_OBJECT
public:
  AudioWaveformView(QWidget* parent = nullptr);

  //void SetData(const QString& file, const AudioRenderingParams& params);

  void SetBackend(AudioRenderBackend* backend);

  static void DrawWaveform(QPainter* painter, const QRect &rect, const double &scale, const SampleSummer::Sum *samples, int nb_samples, int channels);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

private:
  AudioRenderBackend* backend_;

  QPixmap cached_waveform_;
  QSize cached_size_;
  double cached_scale_;
  int cached_scroll_;

private slots:
  void BackendParamsChanged();

};

#endif // WAVEFORMVIEW_H
