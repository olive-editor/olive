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

#include "config/config.h"

AudioManager* AudioManager::audio_ = nullptr;

void AudioManager::CreateInstance()
{
  if (audio_ == nullptr) {
    audio_ = new AudioManager();
  }
}

void AudioManager::DestroyInstance()
{
  delete audio_;
  audio_ = nullptr;
}

AudioManager *AudioManager::instance()
{
  return audio_;
}

void AudioManager::RefreshDevices()
{
  if (refreshing_devices_) {
    return;
  }

  input_devices_.clear();
  output_devices_.clear();

  refreshing_devices_ = true;

  refresh_thread_.start(QThread::LowPriority);
}

bool AudioManager::IsRefreshing()
{
  return refreshing_devices_;
}

void AudioManager::StartOutput(QIODevice *device)
{
  if (output_ == nullptr) {
    return;
  }

  output_file_ = device;

  output_->start(output_file_);
}

void AudioManager::StopOutput()
{
  if (output_ == nullptr || output_file_ == nullptr) {
    return;
  }

  output_->stop();

  output_file_->close();
  delete output_file_;
  output_file_ = nullptr;
}

void AudioManager::SetOutputDevice(const QAudioDeviceInfo &info)
{
  QAudioFormat format;
  format.setSampleRate(48000);
  format.setChannelCount(2);
  format.setSampleSize(32);
  format.setCodec("audio/pcm");
  format.setByteOrder(QAudioFormat::LittleEndian);
  format.setSampleType(QAudioFormat::Float);

  output_ = std::unique_ptr<QAudioOutput>(new QAudioOutput(info, format, this));
  output_device_info_ = info;
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
  output_(nullptr),
  output_file_(nullptr),
  input_(nullptr),
  input_file_(nullptr),
  refreshing_devices_(false)
{
  RefreshDevices();

  connect(&refresh_thread_, SIGNAL(ListsReady()), this, SLOT(RefreshThreadDone()));
}

void AudioManager::RefreshThreadDone()
{
  output_devices_ = refresh_thread_.output_devices();
  input_devices_ = refresh_thread_.input_devices();

  refreshing_devices_ = false;

  QString preferred_audio_output = Config::Current()["PreferredAudioOutput"].toString();

  if (output_ == nullptr
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

  emit DeviceListReady();
}

AudioRefreshDevicesThread::AudioRefreshDevicesThread()
{
}

void AudioRefreshDevicesThread::run()
{
  output_devices_ = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
  input_devices_ = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

  emit ListsReady();
}

const QList<QAudioDeviceInfo>& AudioRefreshDevicesThread::input_devices()
{
  return input_devices_;
}

const QList<QAudioDeviceInfo>& AudioRefreshDevicesThread::output_devices()
{
  return output_devices_;
}
