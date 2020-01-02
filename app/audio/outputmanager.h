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

#ifndef AUDIOHYBRIDDEVICE_H
#define AUDIOHYBRIDDEVICE_H

#include <QAudioOutput>
#include <QBuffer>
#include <QIODevice>
#include <QMutex>

#include "outputdeviceproxy.h"

class AudioOutputManager : public QObject
{
  Q_OBJECT
public:
  AudioOutputManager(QObject* parent = nullptr);

  bool OutputIsSet();

  /**
   * @brief If enabled, this will emit the SentSamples() signal whenever samples are sent to the output device
   */
  void SetEnableSendingSamples(bool e);

  void SetOutputDevice(QAudioDeviceInfo info, QAudioFormat format);

  void Push(const QByteArray &samples);

  /**
   * @brief Connect a QIODevice (e.g. QFile) to start sending to the audio output
   *
   * This will clear any pushed samples or QIODevices currently being read and will start reading from this next time
   * the audio output requests data.
   */
  void PullFromDevice(QIODevice* device);

  void ResetToPushMode();

signals:
  /**
   * @brief Signal emitted when samples are sent to the output device
   *
   * This sends an array of values corresponding to the channel count. Each value is an average of all the samples just
   * sent to that channel. Useful for connecting to AudioMonitor.
   *
   * Can be disabled with SetEnableSendingSamples() to save CPU usage.
   */
  void SentSamples(QVector<double> averages);

private:
  void ProcessAverages(const char* data, int length);

  std::unique_ptr<QAudioOutput> output_;
  QIODevice* push_device_;

  QByteArray pushed_samples_;
  int pushed_sample_index_;

  bool enable_sending_samples_;

  AudioOutputDeviceProxy device_proxy_;

private slots:
  void OutputNotified();
};

#endif // AUDIOHYBRIDDEVICE_H
