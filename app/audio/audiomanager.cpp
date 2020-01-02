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

  // Refreshing devices can take some time, so we do it in a separate thread
  QThread* thread = new QThread();
  connect(thread, &QThread::finished, thread, &QThread::deleteLater);
  thread->start(QThread::LowPriority);

  AudioRefreshDevicesObject* refresher = new AudioRefreshDevicesObject();
  connect(refresher, &AudioRefreshDevicesObject::ListsReady, this, &AudioManager::RefreshThreadDone);
  refresher->moveToThread(thread);

  QMetaObject::invokeMethod(refresher,
                            "Refresh",
                            Qt::QueuedConnection);
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
  output_manager_.PullFromDevice(device);
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
      abort();
    }

    output_manager_.SetOutputDevice(info, format);
  }

  // Un-comment this to get debug information about what the audio output is doing
  //connect(output_.get(), &QAudioOutput::stateChanged, this, &AudioManager::OutputStateChanged);
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
  input_(nullptr),
  input_file_(nullptr),
  refreshing_devices_(false)
{
  RefreshDevices();

  connect(&output_manager_, &AudioHybridDevice::SentSamples, this, &AudioManager::SentSamples);

  output_manager_.SetEnableSendingSamples(true);
}

void AudioManager::RefreshThreadDone()
{
  AudioRefreshDevicesObject* refresher = static_cast<AudioRefreshDevicesObject*>(sender());

  output_devices_ = refresher->output_devices();
  input_devices_ = refresher->input_devices();

  refreshing_devices_ = false;

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
  refresher->deleteLater();

  emit DeviceListReady();
}

void AudioManager::OutputStateChanged(QAudio::State state)
{
  qDebug() << state;
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

  // Move back to main thread
  moveToThread(QApplication::instance()->thread());

  emit ListsReady();
}
