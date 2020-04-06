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

#ifndef AUDIOWAVEFORMVIEW_H
#define AUDIOWAVEFORMVIEW_H

#include <QWidget>

#include "audio/sumsamples.h"
#include "render/audioparams.h"
#include "render/backend/audiorenderbackend.h"
#include "widget/timeruler/seekablewidget.h"

OLIVE_NAMESPACE_ENTER

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

  void ForceUpdate();

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOWAVEFORMVIEW_H
