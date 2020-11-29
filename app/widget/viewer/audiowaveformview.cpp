/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

namespace olive {

AudioWaveformView::AudioWaveformView(QWidget *parent) :
  SeekableWidget(parent),
  playback_(nullptr)
{
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);

  cached_waveform_.resize(QThread::idealThreadCount());
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

  if (!params.is_valid()) {
    return;
  }

  QPainter p(this);

  // Draw in/out points
  DrawTimelinePoints(&p);

  CachedWaveformInfo wanted_info = {size(), GetScale(), GetScroll(), params};

  for (int i=0; i<cached_waveform_.size(); i++) {
    ActiveCache& cache = cached_waveform_[i];

    int slice_start = width()/cached_waveform_.size() * i;
    int slice_end = width()/cached_waveform_.size() * (i+1);

    if (cache.info == wanted_info) {

      // Draw pixmap
      p.drawPixmap(slice_start, 0, cache.pixmap);

    } else if (cache.caching_info != wanted_info) {

      // Pixmap is obsolete, will need to draw again

      // Delete any existing watcher so we don't receive the signal
      delete cache.watcher;

      // Queue a new background cache operation
      cache.caching_info = wanted_info;
      cache.watcher = new QFutureWatcher<QPixmap>();
      connect(cache.watcher, &QFutureWatcher<QPixmap>::finished, this, &AudioWaveformView::BackgroundCacheFinished);
      cache.watcher->setFuture(QtConcurrent::run(this,
                                                 &AudioWaveformView::DrawWaveform,
                                                 playback_->CreatePlaybackDevice(),
                                                 wanted_info,
                                                 slice_start,
                                                 slice_end));

    }
  }

  // Draw playhead
  p.setPen(PLAYHEAD_COLOR);

  int playhead_x = UnitToScreen(GetTime());
  p.drawLine(playhead_x, 0, playhead_x, height());
}

QPixmap AudioWaveformView::DrawWaveform(QIODevice* fs, CachedWaveformInfo info, int slice_start, int slice_end) const
{
  QPixmap pixmap(slice_end - slice_start, info.size.height());
  pixmap.fill(Qt::transparent);

  if (fs->open(QFile::ReadOnly)) {

    QPainter wave_painter(&pixmap);

    // FIXME: Hardcoded color
    wave_painter.setPen(QColor(64, 255, 160));

    int drew = 0;

    fs->seek(info.params.samples_to_bytes(ScreenToUnitRounded(slice_start)));

    for (int x=slice_start; x<slice_end && !fs->atEnd(); x++) {
      int samples_len = ScreenToUnitRounded(x+1) - ScreenToUnitRounded(x);
      int max_read_size = info.params.samples_to_bytes(samples_len);

      QByteArray read_buffer = fs->read(max_read_size);

      // Detect whether we've reached EOF and recalculate sample count if so
      if (read_buffer.size() < max_read_size) {
        samples_len = info.params.bytes_to_samples(read_buffer.size());
      }

      QVector<AudioVisualWaveform::SamplePerChannel> samples = AudioVisualWaveform::SumSamples(reinterpret_cast<const float*>(read_buffer.constData()),
                                                                                               samples_len,
                                                                                               info.params.channel_count());

      for (int i=0;i<info.params.channel_count();i++) {
        AudioVisualWaveform::DrawSample(&wave_painter, samples, x - slice_start, 0, info.size.height());

        drew++;
      }
    }

    fs->close();

  }

  delete fs;

  return pixmap;
}

void AudioWaveformView::BackendParamsChanged()
{
  SetTimebase(playback_->GetParameters().time_base());
}

void AudioWaveformView::ForceUpdate()
{
  // Forces the cache to invalidate
  for (int i=0; i<cached_waveform_.size(); i++) {
    cached_waveform_[i].info.size = QSize();
  }

  update();
}

void AudioWaveformView::BackgroundCacheFinished()
{
  // Retrieve sender
  QFutureWatcher<QPixmap>* watcher = static_cast<QFutureWatcher<QPixmap>*>(sender());

  // Determine index
  int index = -1;
  for (int i=0; i<cached_waveform_.size(); i++) {
    if (cached_waveform_.at(i).watcher == watcher) {
      index = i;
      break;
    }
  }

  if (index > -1) {
    // Store generated pixmap
    cached_waveform_[index].info = cached_waveform_[index].caching_info;
    cached_waveform_[index].pixmap = watcher->result();
    cached_waveform_[index].watcher = nullptr;

    // Update with new pixmap
    update();
  }

  // Clean up
  delete watcher;
}

}
