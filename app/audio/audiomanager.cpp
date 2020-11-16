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
  output_manager_->Push(samples);

  emit OutputPushed(samples);
}

void AudioManager::StartOutput(AudioPlaybackCache *cache, qint64 offset, int playback_speed)
{
  // Create device
  QIODevice* device = cache->CreatePlaybackDevice();

  // Move to output manager's thread
  device->moveToThread(&output_thread_);

  // Queue to output manger in other thread
  QMetaObject::invokeMethod(output_manager_,
                            "PullFromDevice",
                            Qt::QueuedConnection,
                            Q_ARG(QIODevice*, device),
                            Q_ARG(qint64, offset),
                            Q_ARG(int, playback_speed));

  emit OutputDeviceStarted(cache, offset, playback_speed);
}

void AudioManager::StopOutput()
{
  QMetaObject::invokeMethod(output_manager_,
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
  }
}

void AudioManager::SetOutputParams(const AudioParams &params)
{
  if (output_params_ != params) {
    output_params_ = params;

    QMetaObject::invokeMethod(output_manager_,
                              "SetParameters",
                              Qt::QueuedConnection,
                              OLIVE_NS_ARG(AudioParams, params));

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
  is_refreshing_inputs_(false),
  is_refreshing_outputs_(false),
  output_is_set_(false),
  input_(nullptr),
  input_file_(nullptr)
{
  RefreshDevices();

  output_thread_.start(QThread::TimeCriticalPriority);
  output_manager_ = new AudioOutputManager();
  output_manager_->moveToThread(&output_thread_);

  connect(output_manager_, &AudioOutputManager::OutputNotified, this, &AudioManager::OutputNotified);
}

AudioManager::~AudioManager()
{
  QMetaObject::invokeMethod(output_manager_, "deleteLater", Qt::BlockingQueuedConnection);
  output_thread_.quit();
  output_thread_.wait();
}

void AudioManager::OutputDevicesRefreshed()
{
  QFutureWatcher< QList<QAudioDeviceInfo> >* watcher = static_cast<QFutureWatcher< QList<QAudioDeviceInfo> >*>(sender());

  output_devices_ = watcher->result();
  watcher->deleteLater();
  is_refreshing_outputs_ = false;

  QString preferred_audio_output = Config::Current()["AudioOutput"].toString();

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

OLIVE_NAMESPACE_EXIT
