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

#ifndef AUDIOWAVEFORMVIEW_H
#define AUDIOWAVEFORMVIEW_H

#include <QWidget>

#include "audio/audiovisualwaveform.h"
#include "render/audioparams.h"
#include "render/audioplaybackcache.h"
#include "widget/timeruler/seekablewidget.h"

namespace olive {

class AudioWaveformView : public SeekableWidget
{
  Q_OBJECT
public:
  AudioWaveformView(QWidget* parent = nullptr);

  //void SetData(const QString& file, const AudioRenderingParams& params);

  void SetViewer(AudioPlaybackCache *playback);

protected:
  virtual void paintEvent(QPaintEvent* event) override;

private:
  AudioPlaybackCache *playback_;

  QPixmap cached_waveform_;
  QSize cached_size_;
  double cached_scale_;
  int cached_scroll_;

private slots:
  void BackendParamsChanged();

  void ForceUpdate();

};

}

#endif // AUDIOWAVEFORMVIEW_H
