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
  output_watcher_.setFuture(QtConcurrent::run(QAudioDeviceInfo::availableDevices, QAudio::AudioOutput));
  input_watcher_.setFuture(QtConcurrent::run(QAudioDeviceInfo::availableDevices, QAudio::AudioInput));
}

bool AudioManager::IsRefreshingOutputs()
{
  return output_watcher_.isRunning();
}

bool AudioManager::IsRefreshingInputs()
{
  return input_watcher_.isRunning();
}

void AudioManager::PushToOutput(const QByteArray &samples)
{
  output_manager_.Push(samples);

  emit OutputPushed(samples);
}

void AudioManager::StartOutput(const QString &filename, qint64 offset, int playback_speed)
{
  QMetaObject::invokeMethod(&output_manager_,
                            "PullFromDevice",
                            Qt::QueuedConnection,
                            Q_ARG(const QString&, filename),
                            Q_ARG(qint64, offset),
                            Q_ARG(int, playback_speed));

  emit OutputDeviceStarted(filename, offset, playback_speed);
}

void AudioManager::StopOutput()
{
  QMetaObject::invokeMethod(&output_manager_,
                            "ResetToPushMode",
                            Qt::QueuedConnection);

  emit Stopped();
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
      QMetaObject::invokeMethod(&output_manager_,
                                "SetOutputDevice",
                                Qt::QueuedConnection,
                                Q_ARG(const QAudioDeviceInfo&, info),
                                Q_ARG(const QAudioFormat&, format));
      output_is_set_ = true;
    } else {
      qWarning() << "Output format not supported by device";
    }
  }
}

void AudioManager::SetOutputParams(const AudioRenderingParams &params)
{
  if (output_params_ != params) {
    output_params_ = params;

    QMetaObject::invokeMethod(&output_manager_,
                              "SetParameters",
                              Qt::QueuedConnection,
                              OLIVE_NS_ARG(AudioRenderingParams, params));

    // Refresh output device
    SetOutputDevice(output_device_info_);

    emit AudioParamsChanged(output_params_);
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
  output_is_set_(false),
  input_(nullptr),
  input_file_(nullptr)
{
  RefreshDevices();

  output_thread_.start(QThread::TimeCriticalPriority);
  output_manager_.moveToThread(&output_thread_);

  connect(&output_manager_, &AudioOutputManager::OutputNotified, this, &AudioManager::OutputNotified);

  connect(&output_watcher_, &QFutureWatcher< QList<QAudioDeviceInfo> >::finished, this, &AudioManager::OutputDevicesRefreshed);
  connect(&input_watcher_, &QFutureWatcher< QList<QAudioDeviceInfo> >::finished, this, &AudioManager::InputDevicesRefreshed);
}

AudioManager::~AudioManager()
{
  QMetaObject::invokeMethod(&output_manager_, "Close", Qt::QueuedConnection);
  output_thread_.quit();
  output_thread_.wait();
}

void AudioManager::OutputDevicesRefreshed()
{
  output_devices_ = output_watcher_.result();

  QString preferred_audio_output = Config::Current()["PreferredAudioOutput"].toString();

  if (!output_is_set_
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

  emit OutputListReady();
}

void AudioManager::InputDevicesRefreshed()
{
  input_devices_ = input_watcher_.result();

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

  emit InputListReady();
}

OLIVE_NAMESPACE_EXIT
