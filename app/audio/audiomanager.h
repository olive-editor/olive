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

#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <memory>
#include <QtConcurrent/QtConcurrent>
#include <QThread>
#include <portaudio.h>

#include "audiovisualwaveform.h"
#include "common/define.h"
#include "render/audioparams.h"
#include "render/audioplaybackcache.h"
#include "render/previewaudiodevice.h"

namespace olive {

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

  void SetOutputNotifyInterval(int n);

  void PushToOutput(const AudioParams &params, const QByteArray& samples);

  void ClearBufferedOutput();

  void StopOutput();

  void SetOutputDevice(PaDeviceIndex device);

  void SetInputDevice(PaDeviceIndex device);

signals:
  void OutputListReady();

  void OutputNotify();

  void InputListReady();

private:
  AudioManager();

  virtual ~AudioManager() override;

  static PaSampleFormat GetPortAudioSampleFormat(AudioParams::Format fmt);

  void CloseOutputStream();

  bool is_refreshing_inputs_;
  bool is_refreshing_outputs_;

  static AudioManager* instance_;

  PaDeviceIndex output_device_;
  PaStream *output_stream_;
  AudioParams output_params_;
  std::unique_ptr<PreviewAudioDevice> output_buffer_;

  PaDeviceIndex input_device_;

private slots:
  void OutputDevicesRefreshed();

  void InputDevicesRefreshed();

};

}

#endif // AUDIOMANAGER_H
