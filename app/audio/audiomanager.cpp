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

int InputCallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData)
{
  FFmpegEncoder *f = static_cast<FFmpegEncoder*>(userData);

  AudioParams our_params = f->params().audio_params();
  our_params.set_format(AudioParams::GetPackedEquivalent(f->params().audio_params().format()));

  f->WriteAudioData(our_params, reinterpret_cast<const uint8_t**>(&input), frameCount);

  return paContinue;
}

bool AudioManager::PushToOutput(const AudioParams &params, const QByteArray &samples, QString *error)
{
  if (output_device_ == paNoDevice) {
    if (error) *error = tr("No output device is set");
    return false;
  }

  if (output_params_ != params || output_stream_ == nullptr) {
    output_params_ = params;

    CloseOutputStream();

    PaStreamParameters p = GetPortAudioParams(params, output_device_);

    PaError r = Pa_OpenStream(&output_stream_, nullptr, &p, output_params_.sample_rate(), paFramesPerBufferUnspecified, paNoFlag, OutputCallback, output_buffer_);
    if (r != paNoError) {
      // Unhandled error
      //qCritical() << "Failed to open output stream:" << Pa_GetErrorText(r);
      if (error) *error = Pa_GetErrorText(r);
      return false;
    }

    output_buffer_->set_bytes_per_frame(output_params_.samples_to_bytes(1));
  }

  output_buffer_->write(samples);

  if (!Pa_IsStreamActive(output_stream_)) {
    Pa_StartStream(output_stream_);
  }

  return true;
}

void AudioManager::ClearBufferedOutput()
{
  output_buffer_->clear();
}

PaSampleFormat AudioManager::GetPortAudioSampleFormat(AudioParams::Format fmt)
{
  switch (fmt) {
  case AudioParams::kFormatUnsigned8Packed:
  case AudioParams::kFormatUnsigned8Planar:
    return paUInt8;
  case AudioParams::kFormatSigned16Packed:
  case AudioParams::kFormatSigned16Planar:
    return paInt16;
  case AudioParams::kFormatSigned32Packed:
  case AudioParams::kFormatSigned32Planar:
    return paInt32;
  case AudioParams::kFormatFloat32Packed:
  case AudioParams::kFormatFloat32Planar:
    return paFloat32;
  case AudioParams::kFormatSigned64Packed:
  case AudioParams::kFormatSigned64Planar:
  case AudioParams::kFormatFloat64Packed:
  case AudioParams::kFormatFloat64Planar:
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

bool AudioManager::StartRecording(const EncodingParams &params, QString *error_str)
{
  if (input_device_ == paNoDevice) {
    return false;
  }

  input_encoder_ = new FFmpegEncoder(params);
  if (!input_encoder_->Open()) {
    qCritical() << "Failed to open encoder for recording";
    return false;
  }

  PaStreamParameters p = GetPortAudioParams(params.audio_params(), input_device_);

  PaError r = Pa_OpenStream(&input_stream_, &p, nullptr, params.audio_params().sample_rate(), paFramesPerBufferUnspecified, paNoFlag, InputCallback, input_encoder_);
  if (r == paNoError) {
    //const PaStreamInfo* info = Pa_GetStreamInfo(input_stream_);
    r = Pa_StartStream(input_stream_);
    if (r == paNoError) {
      return true;
    }
  }

  if (error_str) {
    *error_str = Pa_GetErrorText(r);
  }

  StopRecording();
  return false;
}

void AudioManager::StopRecording()
{
  if (input_stream_) {
    if (Pa_IsStreamActive(input_stream_)) {
      Pa_StopStream(input_stream_);
    }
    Pa_CloseStream(input_stream_);

    input_stream_ = nullptr;
  }

  if (input_encoder_) {
    input_encoder_->Close();
    delete input_encoder_;
    input_encoder_ = nullptr;
  }
}

PaDeviceIndex AudioManager::FindConfigDeviceByName(bool is_output_device)
{
  QString entry = is_output_device ? QStringLiteral("AudioOutput") : QStringLiteral("AudioInput");

  return FindDeviceByName(OLIVE_CONFIG_STR(entry).toString(), is_output_device);
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

PaStreamParameters AudioManager::GetPortAudioParams(const AudioParams &params, PaDeviceIndex device)
{
  PaStreamParameters p;

  p.channelCount = params.channel_count();
  p.device = device;
  p.hostApiSpecificStreamInfo = nullptr;
  p.sampleFormat = GetPortAudioSampleFormat(params.format());
  p.suggestedLatency = Pa_GetDeviceInfo(device)->defaultLowOutputLatency;

  return p;
}

AudioManager::AudioManager() :
  output_stream_(nullptr),
  input_stream_(nullptr),
  input_encoder_(nullptr)
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
