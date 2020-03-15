#include "waveformview.h"

#include <QFile>
#include <QPainter>
#include <QtMath>

#include "common/clamp.h"

WaveformView::WaveformView(QWidget *parent) :
  SeekableWidget(parent),
  backend_(nullptr)
{
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);
}

void WaveformView::SetBackend(AudioRenderBackend *backend)
{
  if (backend_) {
    disconnect(backend_, &AudioRenderBackend::QueueComplete, this, static_cast<void (WaveformView::*)()>(&WaveformView::update));
    disconnect(backend_, &AudioRenderBackend::ParamsChanged, this, &WaveformView::BackendParamsChanged);

    SetTimebase(0);
  }

  backend_ = backend;

  if (backend_) {
    connect(backend_, &AudioRenderBackend::QueueComplete, this, static_cast<void (WaveformView::*)()>(&WaveformView::update));
    connect(backend_, &AudioRenderBackend::ParamsChanged, this, &WaveformView::BackendParamsChanged);

    SetTimebase(backend_->params().time_base());
  }

  update();
}

void WaveformView::DrawWaveform(QPainter *painter, const QRect& rect, const double& scale, const SampleSummer::Sum* samples, int nb_samples, int channels)
{
  int sample_index, next_sample_index = 0;

  QVector<SampleSummer::Sum> summary;
  int summary_index = -1;

  int channel_height = rect.height() / channels;
  int channel_half_height = channel_height / 2;

  for (int i=0;i<rect.width();i++) {
    sample_index = next_sample_index;

    if (sample_index == nb_samples) {
      break;
    }

    next_sample_index = qMin(nb_samples,
                             qFloor(static_cast<double>(SampleSummer::kSumSampleRate) * static_cast<double>(i+1) / scale) * channels);

    if (summary_index != sample_index) {
      summary = SampleSummer::ReSumSamples(&samples[sample_index],
                                           qMax(channels, next_sample_index - sample_index),
                                           channels);
      summary_index = sample_index;
    }

    int line_x = i + rect.x();

    for (int j=0;j<summary.size();j++) {
      int channel_mid = rect.y() + channel_height * j + channel_half_height;

      painter->drawLine(line_x,
                        channel_mid + clamp(qRound(summary.at(j).min * channel_half_height), -channel_half_height, channel_half_height),
                        line_x,
                        channel_mid + clamp(qRound(summary.at(j).max * channel_half_height), -channel_half_height, channel_half_height));
    }
  }
}

void WaveformView::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  if (!backend_ || backend_->CachePathName().isEmpty() || !backend_->params().is_valid()) {
    return;
  }

  const AudioRenderingParams& params = backend_->params();

  QFile fs(backend_->CachePathName());

  if (fs.open(QFile::ReadOnly)) {

    QPainter p(this);

    DrawTimelinePoints(&p);

    // FIXME: Hardcoded color
    p.setPen(Qt::green);

    int channel_height = height() / params.channel_count();
    int channel_half_height = channel_height / 2;

    int drew = 0;

    fs.seek(params.samples_to_bytes(ScreenToUnitRounded(0)));

    for (int x=0; x<width() && !fs.atEnd(); x++) {

      int samples_len = ScreenToUnitRounded(x+1) - ScreenToUnitRounded(x);
      int max_read_size = params.samples_to_bytes(samples_len);

      QByteArray read_buffer = fs.read(max_read_size);

      // Detect whether we've reached EOF and recalculate sample count if so
      if (read_buffer.size() < max_read_size) {
        samples_len = params.bytes_to_samples(read_buffer.size());
      }

      QVector<SampleSummer::Sum> samples = SampleSummer::SumSamples(reinterpret_cast<const float*>(read_buffer.constData()),
                                                                    samples_len,
                                                                    params.channel_count());

      for (int i=0;i<params.channel_count();i++) {
        int channel_mid = channel_height * i + channel_half_height;

        p.drawLine(x,
                   channel_mid + samples.at(i).min * static_cast<float>(channel_half_height),
                   x,
                   channel_mid + samples.at(i).max * static_cast<float>(channel_half_height));

        drew++;
      }
    }

    fs.close();

    // Draw playhead
    p.setPen(GetPlayheadColor());

    int playhead_x = UnitToScreen(GetTime());
    p.drawLine(playhead_x, 0, playhead_x, height());

  }
}

void WaveformView::BackendParamsChanged()
{
  SetTimebase(rational(1, backend_->params().sample_rate()));
}
