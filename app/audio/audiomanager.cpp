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

void AudioManager::PushToOutput(const QByteArray &samples)
{
  output_manager_.Push(samples);
}

void AudioManager::StartOutput(QIODevice *device)
{
  if (output_ == nullptr) {
    return;
  }

  output_manager_.ConnectDevice(device);
}

void AudioManager::StopOutput()
{
  output_manager_.Stop();

  if (output_ != nullptr) {
    output_->stop();
  }
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
    case SAMPLE_FMT_U8:
      format.setSampleSize(8);
      format.setSampleType(QAudioFormat::UnSignedInt);
      break;
    case SAMPLE_FMT_S16:
      format.setSampleSize(16);
      format.setSampleType(QAudioFormat::SignedInt);
      break;
    case SAMPLE_FMT_S32:
      format.setSampleSize(32);
      format.setSampleType(QAudioFormat::SignedInt);
      break;
    case SAMPLE_FMT_S64:
      format.setSampleSize(64);
      format.setSampleType(QAudioFormat::SignedInt);
      break;
    case SAMPLE_FMT_FLT:
      format.setSampleSize(32);
      format.setSampleType(QAudioFormat::Float);
      break;
    case SAMPLE_FMT_DBL:
      format.setSampleSize(64);
      format.setSampleType(QAudioFormat::Float);
      break;
    case SAMPLE_FMT_COUNT:
    case SAMPLE_FMT_INVALID:
      Q_ASSERT(false);
    }

    output_ = std::unique_ptr<QAudioOutput>(new QAudioOutput(info, format, this));
    connect(output_.get(), SIGNAL(notify()), this, SLOT(OutputNotified()));
  }

  // Un-comment this to get debug information about what the audio output is doing
  //connect(output_.get(), SIGNAL(stateChanged(QAudio::State)), this, SLOT(OutputStateChanged(QAudio::State)));
}

void AudioManager::SetOutputParams(const AudioRenderingParams &params)
{
  if (output_params_ != params) {
    output_params_ = params;

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

AudioManager::AudioManager() :
  output_(nullptr),
  input_(nullptr),
  input_file_(nullptr),
  refreshing_devices_(false)
{
  connect(&refresh_thread_, SIGNAL(ListsReady()), this, SLOT(RefreshThreadDone()));

  RefreshDevices();

  connect(&output_manager_, SIGNAL(HasSamples()), this, SLOT(OutputManagerHasSamples()));
  connect(&output_manager_, SIGNAL(SentSamples(QVector<double>)), this, SIGNAL(SentSamples(QVector<double>)));

  output_manager_.SetEnableSendingSamples(true);
  output_manager_.open(AudioHybridDevice::ReadOnly);
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

void AudioManager::OutputManagerHasSamples()
{
  if (output_ != nullptr && output_->state() != QAudio::ActiveState) {
    output_->start(&output_manager_);
  }
}

void AudioManager::OutputStateChanged(QAudio::State state)
{
  qDebug() << state;
}

void AudioManager::OutputNotified()
{
  if (output_manager_.IsIdle()) {
    static_cast<QAudioOutput*>(sender())->stop();
  }
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
