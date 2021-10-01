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
}

bool AudioManager::IsRefreshingOutputs()
{
  return is_refreshing_outputs_;
}

bool AudioManager::IsRefreshingInputs()
{
  return is_refreshing_inputs_;
}

void AudioManager::SetOutputNotifyInterval(int n)
{
  output_buffer_->set_notify_interval(n);
}

int OutputCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
  PreviewAudioDevice *device = static_cast<PreviewAudioDevice*>(userData);

  qint64 max_read = frameCount * device->bytes_per_frame();
  qint64 read_count = device->read(reinterpret_cast<char*>(output), max_read);
  if (read_count < max_read) {
    memset(reinterpret_cast<uint8_t*>(output) + read_count, 0, max_read - read_count);
  }

  return paContinue;
}

void AudioManager::PushToOutput(const AudioParams &params, const QByteArray &samples)
{
  if (output_params_ != params || output_stream_ == nullptr) {
    output_params_ = params;

    CloseOutputStream();

    PaStreamParameters p;

    p.channelCount = output_params_.channel_count();
    p.device = output_device_;
    p.hostApiSpecificStreamInfo = nullptr;
    p.sampleFormat = GetPortAudioSampleFormat(output_params_.format());
    p.suggestedLatency = Pa_GetDeviceInfo(output_device_)->defaultLowOutputLatency;

    Pa_OpenStream(&output_stream_, nullptr, &p, output_params_.sample_rate(), paFramesPerBufferUnspecified, paNoFlag, OutputCallback, output_buffer_.get());

    output_buffer_->set_bytes_per_frame(output_params_.samples_to_bytes(1));
  }

  output_buffer_->write(samples);

  if (!Pa_IsStreamActive(output_stream_)) {
    Pa_StartStream(output_stream_);
  }
}

void AudioManager::ClearBufferedOutput()
{
  output_buffer_->clear();
}

PaSampleFormat AudioManager::GetPortAudioSampleFormat(AudioParams::Format fmt)
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

void AudioManager::CloseOutputStream()
{
  if (output_stream_) {
    if (Pa_IsStreamActive(output_stream_)) {
      StopOutput();
    }
    Pa_CloseStream(output_stream_);
    output_stream_ = nullptr;
  }
}

void AudioManager::StopOutput()
{
  // Abort the stream so playback stops immediately
  if (output_stream_) {
    Pa_AbortStream(output_stream_);
    ClearBufferedOutput();
  }
}

void AudioManager::SetOutputDevice(PaDeviceIndex device)
{
  qInfo() << "Setting output audio device to" << Pa_GetDeviceInfo(device)->name;

  output_device_ = device;

  CloseOutputStream();

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

void AudioManager::SetInputDevice(PaDeviceIndex device)
{
  qInfo() << "Setting input audio device to" << Pa_GetDeviceInfo(device)->name;

  input_device_ = device;
}

AudioManager::AudioManager() :
  is_refreshing_inputs_(false),
  is_refreshing_outputs_(false),
  output_stream_(nullptr)
{
  //RefreshDevices();

  Pa_Initialize();

  SetOutputDevice(Pa_GetDefaultOutputDevice());
  SetInputDevice(Pa_GetDefaultInputDevice());

  output_buffer_ = std::make_unique<PreviewAudioDevice>();
  output_buffer_->open(PreviewAudioDevice::ReadWrite);
  connect(output_buffer_.get(), &PreviewAudioDevice::Notify, this, &AudioManager::OutputNotify);
}

AudioManager::~AudioManager()
{
  CloseOutputStream();

  Pa_Terminate();
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
  /*QFutureWatcher< QList<QAudioDeviceInfo> >* watcher = static_cast<QFutureWatcher< QList<QAudioDeviceInfo> >*>(sender());

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

  emit InputListReady();*/
}

}
