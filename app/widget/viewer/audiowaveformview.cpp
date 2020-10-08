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
#include "timeline/timelinecommon.h"

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

void AudioWaveformView::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  if (!playback_) {
    return;
  }

  const AudioParams& params = playback_->GetParameters();

  if (playback_->GetPCMFilename().isEmpty()
      || !params.is_valid()) {
    return;
  }

  if (cached_size_ != size()
      || cached_scale_ != GetScale()
      || cached_scroll_ != GetScroll()) {

    cached_waveform_ = QPixmap(size());
    cached_waveform_.fill(Qt::transparent);

    QFile fs(playback_->GetPCMFilename());

    if (fs.open(QFile::ReadOnly)) {

      QPainter wave_painter(&cached_waveform_);

      // FIXME: Hardcoded color
      wave_painter.setPen(QColor(64, 255, 160));

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

        QVector<AudioVisualWaveform::SamplePerChannel> samples = AudioVisualWaveform::SumSamples(reinterpret_cast<const float*>(read_buffer.constData()),
                                                                                       samples_len,
                                                                                       params.channel_count());

        for (int i=0;i<params.channel_count();i++) {
          AudioVisualWaveform::DrawSample(&wave_painter, samples, x, 0, height());

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
  p.setPen(PLAYHEAD_COLOR);

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
