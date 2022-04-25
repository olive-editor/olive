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

#ifdef PA_HAS_JACK
#include <pa_jack.h>
#endif

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
  if (output_device_ == paNoDevice) {
    return;
  }

  if (output_params_ != params || output_stream_ == nullptr) {
    output_params_ = params;

    CloseOutputStream();

    PaStreamParameters p;

    p.channelCount = output_params_.channel_count();
    p.device = output_device_;
    p.hostApiSpecificStreamInfo = nullptr;
    p.sampleFormat = GetPortAudioSampleFormat(output_params_.format());
    p.suggestedLatency = Pa_GetDeviceInfo(output_device_)->defaultLowOutputLatency;

    Pa_OpenStream(&output_stream_, nullptr, &p, output_params_.sample_rate(), paFramesPerBufferUnspecified, paNoFlag, OutputCallback, output_buffer_);

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
  if (device == paNoDevice) {
    qInfo() << "No output device found";
  } else {
    qInfo() << "Setting output audio device to" << Pa_GetDeviceInfo(device)->name;
  }

  output_device_ = device;

  CloseOutputStream();
}

void AudioManager::SetInputDevice(PaDeviceIndex device)
{
  if (device == paNoDevice) {
    qInfo() << "No input device found";
  } else {
    qInfo() << "Setting input audio device to" << Pa_GetDeviceInfo(device)->name;
  }

  input_device_ = device;
}

void AudioManager::HardReset()
{
  CloseOutputStream();
  Pa_Terminate();
  Pa_Initialize();
}

PaDeviceIndex AudioManager::FindConfigDeviceByName(bool is_output_device)
{
  QString entry = is_output_device ? QStringLiteral("AudioOutput") : QStringLiteral("AudioInput");

  return FindDeviceByName(Config::Current()[entry].toString(), is_output_device);
}

PaDeviceIndex AudioManager::FindDeviceByName(const QString &s, bool is_output_device)
{
  if (!s.isEmpty()) {
    for (PaDeviceIndex i=0, end=Pa_GetDeviceCount(); i<end; i++) {
      const PaDeviceInfo *device = Pa_GetDeviceInfo(i);

      if (((is_output_device && device->maxOutputChannels) || (!is_output_device && device->maxInputChannels))
          && !s.compare(device->name)) {
        return i;
      }
    }
  }

  return is_output_device ? Pa_GetDefaultOutputDevice() : Pa_GetDefaultInputDevice();
}

AudioManager::AudioManager() :
  output_stream_(nullptr)
{
#ifdef PA_HAS_JACK
  // PortAudio doesn't do a strcpy, so we need a const char that's readily accessible (i.e. not
  // a QString converted to UTF-8)
  PaJack_SetClientName("Olive");
#endif

  Pa_Initialize();

  // Get device from config
  PaDeviceIndex output_device = FindConfigDeviceByName(true);
  PaDeviceIndex input_device = FindConfigDeviceByName(false);

  SetOutputDevice(output_device);
  SetInputDevice(input_device);

  output_buffer_ = new PreviewAudioDevice(this);
  output_buffer_->open(PreviewAudioDevice::ReadWrite);
  connect(output_buffer_, &PreviewAudioDevice::Notify, this, &AudioManager::OutputNotify);
}

AudioManager::~AudioManager()
{
  CloseOutputStream();

  Pa_Terminate();
}

}
