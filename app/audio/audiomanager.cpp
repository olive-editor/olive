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

#include "audiomanager.h"

#include <QApplication>

#include "config/config.h"

namespace olive {

AudioManager* AudioManager::instance_ = nullptr;

QString AudioManager::GetAudioBackendName(AudioManager::Backend b)
{
  switch (b) {
  case kAudioBackendQt:
    return tr("Qt");
  case kAudioBackendCount:
    break;
  }

  return tr("Unknown");
}

void AudioManager::CreateInstance()
{
  if (instance_ == nullptr) {
    instance_ = new AudioManager();
  }
}

void AudioManager::DestroyInstance()
{
  delete instance_;
  instance_ = nullptr;
}

AudioManager *AudioManager::instance()
{
  return instance_;
}

void AudioManager::RefreshDevices()
{
  if (!is_refreshing_outputs_) {
    QFutureWatcher< QList<QAudioDeviceInfo> >* output_watcher = new QFutureWatcher< QList<QAudioDeviceInfo> >();
    connect(output_watcher, &QFutureWatcher< QList<QAudioDeviceInfo> >::finished, this, &AudioManager::OutputDevicesRefreshed);
    output_watcher->setFuture(QtConcurrent::run(QAudioDeviceInfo::availableDevices, QAudio::AudioOutput));

    is_refreshing_outputs_ = true;
  }

  if (!is_refreshing_inputs_) {
    QFutureWatcher< QList<QAudioDeviceInfo> >* input_watcher = new QFutureWatcher< QList<QAudioDeviceInfo> >();
    connect(input_watcher, &QFutureWatcher< QList<QAudioDeviceInfo> >::finished, this, &AudioManager::InputDevicesRefreshed);
    input_watcher->setFuture(QtConcurrent::run(QAudioDeviceInfo::availableDevices, QAudio::AudioInput));

    is_refreshing_inputs_ = true;
  }
}

bool AudioManager::IsRefreshingOutputs()
{
  return is_refreshing_outputs_;
}

bool AudioManager::IsRefreshingInputs()
{
  return is_refreshing_inputs_;
}

void AudioManager::PushToOutput(const QByteArray &samples)
{
  if (!output_stream_) {
    // Start output with no callback so it'll be in "push" mode
    StartOutputStream();
  }

  Pa_WriteStream(output_stream_, samples.constData(), output_params_.bytes_to_samples(samples.size()));
}

static PaSampleFormat GetPortAudioSampleFormat(AudioParams::Format fmt)
{
  switch (fmt) {
  case AudioParams::kFormatUnsigned8:
    return paUInt8;
  case AudioParams::kFormatSigned16:
    return paInt16;
  case AudioParams::kFormatSigned32:
    return paInt32;
  case AudioParams::kFormatFloat32:
    return paFloat32;
  case AudioParams::kFormatSigned64:
  case AudioParams::kFormatFloat64:
  case AudioParams::kFormatInvalid:
  case AudioParams::kFormatCount:
    break;
  }

  return 0;
}

int OutputCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
  PreviewAudioDevice *device = static_cast<PreviewAudioDevice*>(userData);

  device->read(reinterpret_cast<char*>(output), frameCount * device->bytes_per_frame());

  return paContinue;
}

void AudioManager::StartOutput(std::shared_ptr<PreviewAudioDevice> device)
{
  // First stop any current device
  StopOutputStream();

  // Store device
  output_device_ = device;

  // Start output stream that pulls from this device
  StartOutputStream(OutputCallback);
}

void AudioManager::StopOutput()
{
  // Stop playback first so the callback won't get called again
  StopOutputStream();

  // Then clear our reference to the output devicec
  output_device_ = nullptr;
}

void AudioManager::SetOutputDevice(const QAudioDeviceInfo &info)
{
  /*qInfo() << "Setting output audio device to" << info.deviceName();

  StopOutput();

  output_device_info_ = info;

  if (output_params_.is_valid()) {
    QAudioFormat format;
    format.setSampleRate(output_params_.sample_rate());
    format.setChannelCount(output_params_.channel_count());
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleSize(output_params_.bits_per_sample());
    format.setSampleType(AudioParams::GetQtSampleType(output_params_.format()));

    if (info.isFormatSupported(format)) {
      QMetaObject::invokeMethod(output_manager_,
                                "SetOutputDevice",
                                Qt::QueuedConnection,
                                Q_ARG(const QAudioDeviceInfo&, info),
                                Q_ARG(const QAudioFormat&, format));
      output_is_set_ = true;
    } else {
      qWarning() << "Output format not supported by device";
    }
  }*/
}

void AudioManager::SetOutputParams(const AudioParams &params)
{
  if (output_params_ != params) {
    // If an output stream is running, stop it now
    StopOutputStream();

    // Update parameters
    output_params_ = params;
  }
}

void AudioManager::SetInputDevice(const QAudioDeviceInfo &info)
{
  input_ = std::unique_ptr<QAudioInput>(new QAudioInput(info, QAudioFormat(), this));
  input_device_info_ = info;
}

const QList<QAudioDeviceInfo> &AudioManager::ListInputDevices()
{
  return input_devices_;
}

const QList<QAudioDeviceInfo> &AudioManager::ListOutputDevices()
{
  return output_devices_;
}

AudioManager::AudioManager() :
  is_refreshing_inputs_(false),
  is_refreshing_outputs_(false),
  output_stream_(nullptr),
  input_(nullptr),
  input_file_(nullptr)
{
  //RefreshDevices();

  Pa_Initialize();

  output_ = Pa_GetDefaultOutputDevice();
}

AudioManager::~AudioManager()
{
  StopOutputStream();

  Pa_Terminate();
}

void AudioManager::StartOutputStream(PaStreamCallback *streamCallback)
{
  if (output_stream_) {
    StopOutputStream();
  }

  PaStreamParameters p;

  p.channelCount = output_params_.channel_count();
  p.device = output_;
  p.hostApiSpecificStreamInfo = nullptr;
  p.sampleFormat = GetPortAudioSampleFormat(output_params_.format());
  p.suggestedLatency = Pa_GetDeviceInfo(output_)->defaultLowOutputLatency;

  Pa_OpenStream(&output_stream_, nullptr, &p, output_params_.sample_rate(), paFramesPerBufferUnspecified, paNoFlag, streamCallback, output_device_.get());
  Pa_StartStream(output_stream_);
}

void AudioManager::StopOutputStream()
{
  if (output_stream_) {
    Pa_AbortStream(output_stream_);
    Pa_CloseStream(output_stream_);
    output_stream_ = nullptr;
  }
}

void AudioManager::OutputDevicesRefreshed()
{
  /*QFutureWatcher< QList<QAudioDeviceInfo> >* watcher = static_cast<QFutureWatcher< QList<QAudioDeviceInfo> >*>(sender());

  output_devices_ = watcher->result();
  watcher->deleteLater();
  is_refreshing_outputs_ = false;

  QString preferred_audio_output = Config::Current()["AudioOutput"].toString();

  if (output_ == paNoDevice
      || (!preferred_audio_output.isEmpty() && output_device_info_.deviceName() != preferred_audio_output)) {
    if (preferred_audio_output.isEmpty()) {
      SetOutputDevice(QAudioDeviceInfo::defaultOutputDevice());
    } else {
      foreach (const QAudioDeviceInfo& info, output_devices_) {
        if (info.deviceName() == preferred_audio_output) {
          SetOutputDevice(info);
          break;
        }
      }
    }
  }

  emit OutputListReady();*/
}

void AudioManager::InputDevicesRefreshed()
{
  QFutureWatcher< QList<QAudioDeviceInfo> >* watcher = static_cast<QFutureWatcher< QList<QAudioDeviceInfo> >*>(sender());

  input_devices_ = watcher->result();
  watcher->deleteLater();
  is_refreshing_inputs_ = false;

  QString preferred_audio_input = Config::Current()["AudioInput"].toString();

  if (input_ == nullptr
      || (!preferred_audio_input.isEmpty() && input_device_info_.deviceName() != preferred_audio_input)) {
    if (preferred_audio_input.isEmpty()) {
      SetInputDevice(QAudioDeviceInfo::defaultInputDevice());
    } else {
      foreach (const QAudioDeviceInfo& info, input_devices_) {
        if (info.deviceName() == preferred_audio_input) {
          SetInputDevice(info);
          break;
        }
      }
    }
  }

  emit InputListReady();
}

}
