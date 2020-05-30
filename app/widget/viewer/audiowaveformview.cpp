/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "audiowaveformview.h"

#include <QFile>
#include <QPainter>
#include <QtMath>

#include "common/clamp.h"
#include "config/config.h"

OLIVE_NAMESPACE_ENTER

AudioWaveformView::AudioWaveformView(QWidget *parent) :
  SeekableWidget(parent),
  playback_(nullptr)
{
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);
}

void AudioWaveformView::SetViewer(AudioPlaybackCache *playback)
{
  if (playback_) {
    disconnect(playback_, &AudioPlaybackCache::Validated, this, &AudioWaveformView::ForceUpdate);
    disconnect(playback_, &AudioPlaybackCache::ParametersChanged, this, &AudioWaveformView::BackendParamsChanged);

    SetTimebase(0);
  }

  playback_ = playback;

  if (playback_) {
    connect(playback_, &AudioPlaybackCache::Validated, this, &AudioWaveformView::ForceUpdate);
    connect(playback_, &AudioPlaybackCache::ParametersChanged, this, &AudioWaveformView::BackendParamsChanged);

    SetTimebase(playback_->GetParameters().time_base());
  }

  ForceUpdate();
}

void AudioWaveformView::DrawWaveform(QPainter *painter, const QRect& rect, const double& scale, const SampleSummer::Sum* samples, int nb_samples, int channels)
{
  int sample_index, next_sample_index = 0;

  QVector<SampleSummer::Sum> summary;
  int summary_index = -1;

  int channel_height = rect.height() / channels;
  int channel_half_height = channel_height / 2;

  const QRect& viewport = painter->viewport();
  QPoint top_left = painter->transform().map(viewport.topLeft());

  int start = qMax(rect.x(), -top_left.x());
  int end = qMin(rect.right(), -top_left.x() + viewport.width());

  QVector<QLine> lines;

  for (int i=start;i<end;i++) {
    sample_index = next_sample_index;

    if (sample_index == nb_samples) {
      break;
    }

    next_sample_index = qMin(nb_samples,
                             qFloor(static_cast<double>(SampleSummer::kSumSampleRate) * static_cast<double>(i - rect.x() + 1) / scale) * channels);

    if (summary_index != sample_index) {
      summary = SampleSummer::ReSumSamples(&samples[sample_index],
                                           qMax(channels, next_sample_index - sample_index),
                                           channels);
      summary_index = sample_index;
    }

    int line_x = i;

    for (int j=0;j<summary.size();j++) {
      if (Config::Current()[QStringLiteral("RectifiedWaveforms")].toBool()) {
        int channel_bottom = rect.y() + channel_height * (j + 1);

        int diff = qRound((summary.at(j).max - summary.at(j).min) * channel_half_height);

        lines.append(QLine(line_x,
                           channel_bottom - diff,
                           line_x,
                           channel_bottom));
      } else{
        int channel_mid = rect.y() + channel_height * j + channel_half_height;

        lines.append(QLine(line_x,
                           channel_mid + clamp(qRound(summary.at(j).min * channel_half_height), -channel_half_height, channel_half_height),
                           line_x,
                           channel_mid + clamp(qRound(summary.at(j).max * channel_half_height), -channel_half_height, channel_half_height)));
      }
    }
  }

  painter->drawLines(lines);
}

void AudioWaveformView::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  const AudioRenderingParams& params = playback_->GetParameters();

  if (!playback_
      || playback_->GetCacheFilename().isEmpty()
      || !params.is_valid()) {
    return;
  }

  if (cached_size_ != size()
      || cached_scale_ != GetScale()
      || cached_scroll_ != GetScroll()) {

    cached_waveform_ = QPixmap(size());
    cached_waveform_.fill(Qt::transparent);

    QFile fs(playback_->GetCacheFilename());

    if (fs.open(QFile::ReadOnly)) {

      QPainter wave_painter(&cached_waveform_);

      // FIXME: Hardcoded color
      wave_painter.setPen(QColor(64, 255, 160));

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
          if (Config::Current()[QStringLiteral("RectifiedWaveforms")].toBool()) {
            int channel_bottom = channel_height * (i + 1);

            int diff = qRound((samples.at(i).max - samples.at(i).min) * channel_half_height);

            wave_painter.drawLine(x,
                                  channel_bottom - diff,
                                  x,
                                  channel_bottom);
          } else {
            int channel_mid = channel_height * i + channel_half_height;

            wave_painter.drawLine(x,
                                  channel_mid + qRound(samples.at(i).min * static_cast<float>(channel_half_height)),
                                  x,
                                  channel_mid + qRound(samples.at(i).max * static_cast<float>(channel_half_height)));
          }

          drew++;
        }
      }

      cached_size_ = size();
      cached_scale_ = GetScale();
      cached_scroll_ = GetScroll();

      fs.close();

    }
  }

  QPainter p(this);

  // Draw in/out points
  DrawTimelinePoints(&p);

  // Draw cached waveform pixmap
  p.drawPixmap(0, 0, cached_waveform_);

  // Draw playhead
  p.setPen(GetPlayheadColor());

  int playhead_x = UnitToScreen(GetTime());
  p.drawLine(playhead_x, 0, playhead_x, height());
}

void AudioWaveformView::BackendParamsChanged()
{
  SetTimebase(playback_->GetParameters().time_base());
}

void AudioWaveformView::ForceUpdate()
{
  cached_size_ = QSize();
  update();
}

OLIVE_NAMESPACE_EXIT
