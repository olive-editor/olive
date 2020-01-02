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

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <memory>
#include <QAudioInput>
#include <QAudioOutput>
#include <QThread>

#include "audiohybriddevice.h"
#include "render/audioparams.h"

/**
 * @brief A thread for refreshing the total list of devices on the system
 *
 * Refreshing devices causes a noticeable pause in execution. Doing it another thread is intended to avoid this.
 */
class AudioRefreshDevicesObject : public QObject {
  Q_OBJECT
public:
  AudioRefreshDevicesObject();

  const QList<QAudioDeviceInfo>& input_devices();
  const QList<QAudioDeviceInfo>& output_devices();

public slots:
  void Refresh();

signals:
  void ListsReady();

private:
  QList<QAudioDeviceInfo> input_devices_;
  QList<QAudioDeviceInfo> output_devices_;
};

/**
 * @brief Audio input and output management class
 *
 * Wraps around a QAudioOutput and AudioHybridDevice, connecting them together and exposing audio functionality to
 * the rest of the system.
 */
class AudioManager : public QObject
{
  Q_OBJECT
public:
  static void CreateInstance();
  static void DestroyInstance();

  static AudioManager* instance();

  void RefreshDevices();

  bool IsRefreshing();

  void PushToOutput(const QByteArray& samples);

  /**
   * @brief Start playing audio from QIODevice
   *
   * This takes ownership of the QIODevice and will delete it when StopOutput() is called
   */
  void StartOutput(QIODevice* device);

  /**
   * @brief Stop audio output immediately
   */
  void StopOutput();

  void SetOutputDevice(const QAudioDeviceInfo& info);

  void SetOutputParams(const AudioRenderingParams& params);

  void SetInputDevice(const QAudioDeviceInfo& info);

  const QList<QAudioDeviceInfo>& ListInputDevices();
  const QList<QAudioDeviceInfo>& ListOutputDevices();

signals:
  void DeviceListReady();

  void SentSamples(QVector<double> averages);

private:
  AudioManager();

  QList<QAudioDeviceInfo> input_devices_;

  QList<QAudioDeviceInfo> output_devices_;

  static AudioManager* instance_;

  AudioHybridDevice output_manager_;

  QAudioDeviceInfo output_device_info_;
  AudioRenderingParams output_params_;

  std::unique_ptr<QAudioInput> input_;
  QAudioDeviceInfo input_device_info_;
  QIODevice* input_file_;

  bool refreshing_devices_;

private slots:
  void RefreshThreadDone();

  void OutputStateChanged(QAudio::State state);

};

#endif // AUDIOMANAGER_H
