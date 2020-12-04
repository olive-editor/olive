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

#ifndef AUDIOHYBRIDDEVICE_H
#define AUDIOHYBRIDDEVICE_H

#include <memory>
#include <QAudioOutput>
#include <QBuffer>
#include <QIODevice>
#include <QMutex>
#include <QThread>

#include "outputdeviceproxy.h"

namespace olive {

class AudioOutputManager : public QObject
{
  Q_OBJECT
public:
  AudioOutputManager(QObject* parent = nullptr);

  virtual ~AudioOutputManager() override;

  // Thread-safe
  void Push(const QByteArray &samples);

public slots:
  // Queued
  void SetOutputDevice(QAudioDeviceInfo info, QAudioFormat format);

  /**
   * @brief Connect a QIODevice (e.g. QFile) to start sending to the audio output
   *
   * This will clear any pushed samples or QIODevices currently being read and will start reading from this next time
   * the audio output requests data.
   */
  void PullFromDevice(QIODevice* device, qint64 offset, int playback_speed);

  // Queued
  void ResetToPushMode();

  // Queued
  void SetParameters(olive::AudioParams params);

  // Queued
  void Close();

signals:
  void OutputNotified();

private:
  QAudioOutput* output_;
  QIODevice* push_device_;

  QMutex push_sample_lock_;
  QByteArray push_samples_;
  int push_sample_index_;

  AudioOutputDeviceProxy device_proxy_;

private slots:
  void PushMoreSamples();

  void OutputStateChanged(QAudio::State state);

};

}

#endif // AUDIOHYBRIDDEVICE_H
