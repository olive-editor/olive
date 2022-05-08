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
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  // NOTE: At some point it might make sense for this to be AlignCenter since the waveform
  //       originates from the center. But we're leaving it top/left for now since it was just
  //       ported from a QWidget's paintEvent.
  setAlignment(Qt::AlignLeft | Qt::AlignTop);
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

void AudioWaveformView::drawForeground(QPainter *p, const QRectF &rect)
{
  super::drawForeground(p, rect);

  if (!playback_) {
    return;
  }

  const AudioParams& params = playback_->GetParameters();

  if (!params.is_valid()) {
    return;
  }

  // Draw in/out points
  DrawTimelinePoints(p);

  // Draw waveform
  p->setPen(QColor(64, 255, 160)); // FIXME: Hardcoded color
  AudioVisualWaveform::DrawWaveform(p, rect.toRect(), GetScale(), playback_->visual(), SceneToTime(GetScroll()));

  // Draw playhead
  p->setPen(PLAYHEAD_COLOR);

  int playhead_x = TimeToScene(GetTime());
  p->drawLine(playhead_x, 0, playhead_x, height());
}

}
