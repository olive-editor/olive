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

#include "audiohybriddevice.h"

#include <QDebug>
#include <QtMath>

#include "audiobufferaverage.h"

AudioHybridDevice::AudioHybridDevice(QObject *parent) :
  QObject(parent),
  output_(nullptr),
  push_device_(nullptr),
  enable_sending_samples_(false)
{
  connect(&device_proxy_, &AudioOutputDeviceProxy::ProcessedAverages, this, &AudioHybridDevice::SentSamples);
}

bool AudioHybridDevice::OutputIsSet()
{
  return (output_.get());
}

void AudioHybridDevice::Push(const QByteArray& samples)
{
  // If no output device, nothing to be done
  if (!output_) {
    return;
  }

  // Replace sample buffer with this one
  pushed_samples_ = samples;
  pushed_sample_index_ = 0;

  // If we had another device connected, disconnect it now
  ResetToPushMode();

  // Start pushing samples to the output
  OutputNotified();
}

void AudioHybridDevice::ResetToPushMode()
{
  // If we have a null push device, then we currently have the output in pull mode. We restore it to push mode here.
  if (output_ && !push_device_) {
    output_->stop();

    device_proxy_.close();

    // Put QAudioOutput back into push mode
    push_device_ = output_->start();
  }
}

void AudioHybridDevice::PullFromDevice(QIODevice *device)
{
  if (!output_ || !device) {
    return;
  }

  // Stop any current output and disable push mode
  output_->stop();
  push_device_ = nullptr;
  pushed_samples_.clear();

  // Pull from the device
  device_proxy_.SetDevice(device);
  device_proxy_.open(QIODevice::ReadOnly);
  output_->start(&device_proxy_);
}

void AudioHybridDevice::OutputNotified()
{
  // Check if we're currently in push mode and if we have samples to push
  if (!push_device_ || pushed_samples_.isEmpty()) {
    return;
  }

  const char* read_ptr = pushed_samples_.constData() + pushed_sample_index_;

  // Push the bytes we have to the audio output
  qint64 write_count = push_device_->write(read_ptr,
                                           pushed_samples_.size() - pushed_sample_index_);

  // Emit the samples we just sent
  ProcessAverages(read_ptr, static_cast<int>(write_count));

  // Increment sample buffer index (faster than shift the bytes up)
  pushed_sample_index_ += static_cast<int>(write_count);

  // If we've pushed all samples, we can clear this array
  if (pushed_sample_index_ == pushed_samples_.size()) {
    pushed_samples_.clear();
  }
}

void AudioHybridDevice::SetEnableSendingSamples(bool e)
{
  enable_sending_samples_ = e;

  device_proxy_.SetSendAverages(e);
}

void AudioHybridDevice::SetOutputDevice(QAudioDeviceInfo info, QAudioFormat format)
{
  // Whatever the output is doing right now, stop it
  if (output_) {
    output_->stop();

    if (device_proxy_.isOpen()) {
      device_proxy_.close();
    }
  }

  // Create a new output device and start it in push mode
  output_ = std::unique_ptr<QAudioOutput>(new QAudioOutput(info, format, this));
  output_->setNotifyInterval(1);
  push_device_ = output_->start();
  connect(output_.get(), &QAudioOutput::notify, this, &AudioHybridDevice::OutputNotified);
}

void AudioHybridDevice::ProcessAverages(const char *data, int length)
{
  if (!enable_sending_samples_ || length == 0) {
    return;
  }

  emit SentSamples(AudioBufferAverage::ProcessAverages(data, length));
}
