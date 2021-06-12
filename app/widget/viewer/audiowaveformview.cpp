/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#define super SeekableWidget

AudioWaveformView::AudioWaveformView(QWidget *parent) :
  super(parent),
  playback_(nullptr)
{
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Base);
}

AudioVisualWaveform GenerateWaveform(QIODevice* device, AudioParams params, TimeRange range)
{
  device->open(QFile::ReadOnly);
  device->seek(params.time_to_bytes(range.in()));

  SampleBufferPtr samples = SampleBuffer::CreateFromPackedData(params, device->read(params.time_to_bytes(range.length())));
  AudioVisualWaveform waveform;
  waveform.set_channel_count(params.channel_count());
  waveform.OverwriteSamples(samples, params.sample_rate());
  device->close();
  delete device;
  return waveform;
}

void AudioWaveformView::SetViewer(AudioPlaybackCache *playback)
{
  if (playback_) {
    pool_.clear();
    pool_.waitForDone();

    disconnect(playback_, &AudioPlaybackCache::Validated, this, static_cast<void(AudioWaveformView::*)()>(&AudioWaveformView::update));

    SetTimebase(0);
  }

  playback_ = playback;

  if (playback_) {
    connect(playback_, &AudioPlaybackCache::Validated, this, static_cast<void(AudioWaveformView::*)()>(&AudioWaveformView::update));

    SetTimebase(playback_->GetParameters().sample_rate_as_time_base());
  }
}

void AudioWaveformView::paintEvent(QPaintEvent *event)
{
  super::paintEvent(event);

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

  // Draw waveform
  p.setPen(QColor(64, 255, 160)); // FIXME: Hardcoded color
  AudioVisualWaveform::DrawWaveform(&p, rect(), GetScale(), playback_->visual(), SceneToTime(GetScroll()));

  // Draw playhead
  p.setPen(PLAYHEAD_COLOR);

  int playhead_x = UnitToScreen(GetTime());
  p.drawLine(playhead_x, 0, playhead_x, height());
}

}
