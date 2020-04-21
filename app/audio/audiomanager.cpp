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

#include "audiomanager.h"

#include <QApplication>

#include "config/config.h"

OLIVE_NAMESPACE_ENTER

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
  if (refresh_thread_) {
    return;
  }

  input_devices_.clear();
  output_devices_.clear();

  // Refreshing devices can take some time, so we do it in a separate thread
  refresh_thread_ = new QThread(this);
  connect(refresh_thread_, &QThread::finished, refresh_thread_, &QThread::deleteLater);

  refresh_thread_->start(QThread::IdlePriority);

  AudioRefreshDevicesObject* refresher = new AudioRefreshDevicesObject();
  connect(refresher, &AudioRefreshDevicesObject::ListsReady, this, &AudioManager::RefreshThreadDone, Qt::QueuedConnection);
  refresher->moveToThread(refresh_thread_);

  QMetaObject::invokeMethod(refresher,
                            "Refresh",
                            Qt::QueuedConnection);
}

bool AudioManager::IsRefreshing()
{
  return refresh_thread_;
}

void AudioManager::PushToOutput(const QByteArray &samples)
{
  output_manager_.Push(samples);
}

void AudioManager::StartOutput(const QString &filename, qint64 offset, int playback_speed)
{
  output_manager_.PullFromDevice(filename, offset, playback_speed);
}

void AudioManager::StopOutput()
{
  output_manager_.ResetToPushMode();
}

void AudioManager::SetOutputDevice(const QAudioDeviceInfo &info)
{
  qInfo() << "Setting output audio device to" << info.deviceName();

  StopOutput();

  output_device_info_ = info;

  if (output_params_.is_valid()) {
    QAudioFormat format;
    format.setSampleRate(output_params_.sample_rate());
    format.setChannelCount(output_params_.channel_count());
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);

    switch (output_params_.format()) {
    case SampleFormat::SAMPLE_FMT_U8:
      format.setSampleSize(8);
      format.setSampleType(QAudioFormat::UnSignedInt);
      break;
    case SampleFormat::SAMPLE_FMT_S16:
      format.setSampleSize(16);
      format.setSampleType(QAudioFormat::SignedInt);
      break;
    case SampleFormat::SAMPLE_FMT_S32:
      format.setSampleSize(32);
      format.setSampleType(QAudioFormat::SignedInt);
      break;
    case SampleFormat::SAMPLE_FMT_S64:
      format.setSampleSize(64);
      format.setSampleType(QAudioFormat::SignedInt);
      break;
    case SampleFormat::SAMPLE_FMT_FLT:
      format.setSampleSize(32);
      format.setSampleType(QAudioFormat::Float);
      break;
    case SampleFormat::SAMPLE_FMT_DBL:
      format.setSampleSize(64);
      format.setSampleType(QAudioFormat::Float);
      break;
    case SampleFormat::SAMPLE_FMT_COUNT:
    case SampleFormat::SAMPLE_FMT_INVALID:
      abort();
    }

    if (info.isFormatSupported(format)) {
      output_manager_.SetOutputDevice(info, format);
    } else {
      qWarning() << "Output format not supported by device";
    }
  }
}

void AudioManager::SetOutputParams(const AudioRenderingParams &params)
{
  if (output_params_ != params) {
    output_params_ = params;

    output_manager_.SetParameters(params);

    // Refresh output device
    SetOutputDevice(output_device_info_);
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

void AudioManager::ReverseBuffer(char *buffer, int buffer_size, int sample_size)
{
  int half_buffer_sz = buffer_size / 2;
  char* temp_buffer = new char[sample_size];

  for (int src_index=0;src_index<half_buffer_sz;src_index+=sample_size) {
    char* src_ptr = buffer + src_index;
    char* dst_ptr = buffer + buffer_size - sample_size - src_index;

    // Simple swap
    memcpy(temp_buffer, src_ptr, static_cast<size_t>(sample_size));
    memcpy(src_ptr, dst_ptr, static_cast<size_t>(sample_size));
    memcpy(dst_ptr, temp_buffer, static_cast<size_t>(sample_size));
  }

  delete [] temp_buffer;
}

AudioManager::AudioManager() :
  input_(nullptr),
  input_file_(nullptr),
  refresh_thread_(nullptr)
{
  RefreshDevices();

  connect(&output_manager_, &AudioOutputManager::OutputNotified, this, &AudioManager::OutputNotified);
}

AudioManager::~AudioManager()
{
  if (refresh_thread_) {
    refresh_thread_->quit();
    refresh_thread_->wait();
  }
}

void AudioManager::RefreshThreadDone()
{
  AudioRefreshDevicesObject* refresher = static_cast<AudioRefreshDevicesObject*>(sender());

  output_devices_ = refresher->output_devices();
  input_devices_ = refresher->input_devices();



  QString preferred_audio_output = Config::Current()["PreferredAudioOutput"].toString();

  if (!output_manager_.OutputIsSet()
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

  QString preferred_audio_input = Config::Current()["PreferredAudioInput"].toString();

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

  // Clean up refresher object
  refresh_thread_->quit();
  refresh_thread_ = nullptr;
  refresher->deleteLater();

  emit DeviceListReady();
}

AudioRefreshDevicesObject::AudioRefreshDevicesObject()
{
}

const QList<QAudioDeviceInfo>& AudioRefreshDevicesObject::input_devices()
{
  return input_devices_;
}

const QList<QAudioDeviceInfo>& AudioRefreshDevicesObject::output_devices()
{
  return output_devices_;
}

void AudioRefreshDevicesObject::Refresh()
{
  output_devices_ = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
  input_devices_ = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

  emit ListsReady();
}

OLIVE_NAMESPACE_EXIT
