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

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <memory>
#include <QAudioInput>
#include <QAudioOutput>
#include <QtConcurrent/QtConcurrent>
#include <QThread>

#include "common/define.h"
#include "outputmanager.h"
#include "render/audioparams.h"
#include "render/audioplaybackcache.h"

OLIVE_NAMESPACE_ENTER

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

  bool IsRefreshingOutputs();

  bool IsRefreshingInputs();

  void PushToOutput(const QByteArray& samples);

  /**
   * @brief Start playing audio from AudioPlaybackCache
   */
  void StartOutput(AudioPlaybackCache* cache, qint64 offset, int playback_speed);

  /**
   * @brief Stop audio output immediately
   */
  void StopOutput();

  void SetOutputDevice(const QAudioDeviceInfo& info);

  void SetOutputParams(const AudioParams& params);

  void SetInputDevice(const QAudioDeviceInfo& info);

  const QList<QAudioDeviceInfo>& ListInputDevices();
  const QList<QAudioDeviceInfo>& ListOutputDevices();

  static void ReverseBuffer(char* buffer, int size, int resolution);

signals:
  void OutputListReady();

  void InputListReady();

  void OutputNotified();

  void OutputDeviceStarted(AudioPlaybackCache* cache, qint64 offset, int playback_speed);

  void AudioParamsChanged(const AudioParams& params);

  void OutputPushed(const QByteArray& data);

  void Stopped();

private:
  AudioManager();

  virtual ~AudioManager() override;

  QList<QAudioDeviceInfo> input_devices_;
  QList<QAudioDeviceInfo> output_devices_;

  bool is_refreshing_inputs_;
  bool is_refreshing_outputs_;

  static AudioManager* instance_;

  QThread output_thread_;
  AudioOutputManager* output_manager_;
  bool output_is_set_;

  QAudioDeviceInfo output_device_info_;
  AudioParams output_params_;

  std::unique_ptr<QAudioInput> input_;
  QAudioDeviceInfo input_device_info_;
  QIODevice* input_file_;

private slots:
  void OutputDevicesRefreshed();

  void InputDevicesRefreshed();

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOMANAGER_H
