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

  void SetOutputNotifyInterval(int n);

  void PushToOutput(const AudioParams &params, const QByteArray& samples);

  void ClearBufferedOutput();

  void StopOutput();

  PaDeviceIndex GetOutputDevice() const
  {
    return output_device_;
  }

  PaDeviceIndex GetInputDevice() const
  {
    return input_device_;
  }

  void SetOutputDevice(PaDeviceIndex device);

  void SetInputDevice(PaDeviceIndex device);

  void HardReset();

  static PaDeviceIndex FindConfigDeviceByName(bool is_output_device);
  static PaDeviceIndex FindDeviceByName(const QString &s, bool is_output_device);

signals:
  void OutputNotify();

private:
  AudioManager();

  virtual ~AudioManager() override;

  static PaSampleFormat GetPortAudioSampleFormat(AudioParams::Format fmt);

  void CloseOutputStream();

  static AudioManager* instance_;

  PaDeviceIndex output_device_;
  PaStream *output_stream_;
  AudioParams output_params_;
  PreviewAudioDevice *output_buffer_;

  PaDeviceIndex input_device_;

};

}

#endif // AUDIOMANAGER_H
