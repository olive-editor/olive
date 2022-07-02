/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef AUDIOMONITORWIDGET_H
#define AUDIOMONITORWIDGET_H

#include <QFile>
#include <QOpenGLWindow>
#include <QTimer>

#include "audio/audiovisualwaveform.h"
#include "common/define.h"
#include "render/audioparams.h"
#include "render/audiowaveformcache.h"

namespace olive {

class AudioMonitor : public QOpenGLWindow
{
  Q_OBJECT
public:
  AudioMonitor();

  virtual ~AudioMonitor() override;

  bool IsPlaying() const
  {
    return waveform_;
  }

  static void StartWaveformOnAll(const AudioWaveformCache *waveform, const rational& start, int playback_speed)
  {
    foreach (AudioMonitor *m, instances_) {
      m->StartWaveform(waveform, start, playback_speed);
    }
  }

  static void StopOnAll()
  {
    foreach (AudioMonitor *m, instances_) {
      m->Stop();
    }
  }

  static void PushSampleBufferOnAll(const SampleBuffer &d)
  {
    foreach (AudioMonitor *m, instances_) {
      m->PushSampleBuffer(d);
    }
  }

public slots:
  void SetParams(const AudioParams& params);

  void Stop();

  void PushSampleBuffer(const SampleBuffer &samples);

  void StartWaveform(const AudioWaveformCache *waveform, const rational& start, int playback_speed);

protected:
  virtual void paintGL() override;

  virtual void mousePressEvent(QMouseEvent* event) override;

private:
  void SetUpdateLoop(bool e);

  void UpdateValuesFromWaveform(QVector<double> &v, qint64 delta_time);

  void AudioVisualWaveformSampleToInternalValues(const AudioVisualWaveform::Sample &in, QVector<double> &out);

  void PushValue(const QVector<double>& v);

  void BytesToSampleSummary(const QByteArray& bytes, QVector<double>& v);

  QVector<double> GetAverages() const;

  AudioParams params_;

  qint64 last_time_;

  const AudioWaveformCache* waveform_;
  rational waveform_time_;
  rational waveform_length_;

  int playback_speed_;

  QVector< QVector<double> > values_;
  QVector<bool> peaked_;

  QPixmap cached_background_;
  int cached_channels_;

  static QVector<AudioMonitor*> instances_;

};

}

#endif // AUDIOMONITORWIDGET_H
