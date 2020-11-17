/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "outputmanager.h"

#include <QDebug>
#include <QtMath>

#include <QFile>

OLIVE_NAMESPACE_ENTER

AudioOutputManager::AudioOutputManager(QObject *parent) :
  QObject(parent),
  output_(nullptr),
  push_device_(nullptr),
  device_proxy_(this)
{
}

AudioOutputManager::~AudioOutputManager()
{
  Close();
}

void AudioOutputManager::Push(const QByteArray& samples)
{
  // This function is not queued and is intended to be called from the caller's thread
  QMutexLocker lock(&push_sample_lock_);

  // Replace sample buffer with this one
  push_samples_ = samples;
  push_sample_index_ = 0;

  // If we had another device connected, disconnect it now
  QMetaObject::invokeMethod(this, "ResetToPushMode", Qt::QueuedConnection);

  // Start pushing samples to the output
  QMetaObject::invokeMethod(this, "PushMoreSamples", Qt::QueuedConnection);
}

void AudioOutputManager::ResetToPushMode()
{
  // If we have a null push device, then we currently have the output in pull mode. We restore it to push mode here.
  if (output_ && !push_device_) {
    output_->stop();

    device_proxy_.close();

    // Put QAudioOutput back into push mode
    push_device_ = output_->start();
  }
}

void AudioOutputManager::SetParameters(AudioParams params)
{
  device_proxy_.SetParameters(params);
}

void AudioOutputManager::Close()
{
  if (output_) {
    output_->stop();

    if (device_proxy_.isOpen()) {
      device_proxy_.close();
    }

    delete output_;
    output_ = nullptr;
  }
}

void AudioOutputManager::PullFromDevice(QIODevice *device, qint64 offset, int playback_speed)
{
  if (!output_) {
    return;
  }

  // Stop any current output and disable push mode
  output_->stop();
  push_device_ = nullptr;
  push_samples_.clear();

  // Pull from the device
  device_proxy_.SetDevice(device, offset, playback_speed);
  device_proxy_.open(QIODevice::ReadOnly);
  output_->start(&device_proxy_);
}

void AudioOutputManager::PushMoreSamples()
{
  QMutexLocker lock(&push_sample_lock_);

  // Check if we're currently in push mode and if we have samples to push
  if (!push_device_ || push_samples_.isEmpty()) {
    return;
  }

  const char* read_ptr = push_samples_.constData() + push_sample_index_;

  // Push the bytes we have to the audio output
  qint64 write_count = push_device_->write(read_ptr,
                                           push_samples_.size() - push_sample_index_);

  // Increment sample buffer index (faster than shift the bytes up)
  push_sample_index_ += static_cast<int>(write_count);

  // If we've pushed all samples, we can clear this array
  if (push_sample_index_ == push_samples_.size()) {
    push_samples_.clear();
  }
}

void AudioOutputManager::SetOutputDevice(QAudioDeviceInfo info, QAudioFormat format)
{
  // Whatever the output is doing right now, stop it
  Close();

  // Create a new output device and start it in push mode
  output_ = new QAudioOutput(info, format, this);
  output_->setBufferSize(131072);
  output_->setNotifyInterval(1);
  push_device_ = output_->start();
  connect(output_, &QAudioOutput::notify, this, &AudioOutputManager::PushMoreSamples);
  connect(output_, &QAudioOutput::notify, this, &AudioOutputManager::OutputNotified);

  // Un-comment this to get debug information about what the audio output is doing
  //connect(output_, &QAudioOutput::stateChanged, this, &AudioOutputManager::OutputStateChanged);
}

void AudioOutputManager::OutputStateChanged(QAudio::State state)
{
  qDebug() << state << output_->error();
}

OLIVE_NAMESPACE_EXIT
