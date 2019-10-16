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

AudioHybridDevice::AudioHybridDevice(QObject *parent) :
  QIODevice(parent),
  device_(nullptr),
  sample_index_(0)
{

}

void AudioHybridDevice::Push(const QByteArray& samples)
{
  Stop();

  pushed_samples_ = samples;
  sample_index_ = 0;

  if (!pushed_samples_.isEmpty()) {
    emit HasSamples();
  }
}

void AudioHybridDevice::Stop()
{
  // Whatever is happening, stop it
  pushed_samples_.clear();
  device_ = nullptr;
}

void AudioHybridDevice::ConnectDevice(QIODevice *device)
{
  // Clear any previous device or pushed sample
  Stop();

  device_ = device;

  if (device_ != nullptr) {
    emit HasSamples();
  }
}

bool AudioHybridDevice::IsIdle()
{
  return device_ == nullptr && pushed_samples_.isEmpty();
}

qint64 AudioHybridDevice::readData(char *data, qint64 maxSize)
{
  // If a device is connected, passthrough to it
  if (device_ != nullptr) {
    qint64 read_count = device_->read(data, maxSize);

    // Stop reading this device
    if (device_->atEnd()) {
      device_ = nullptr;
    }

    return read_count;
  }

  // If there are samples to push, push those
  if (!pushed_samples_.isEmpty()) {
    qint64 read_count = qMin(maxSize, pushed_samples_.size() - sample_index_);

    memcpy(data, pushed_samples_.data()+sample_index_, static_cast<size_t>(read_count));

    sample_index_ += read_count;

    if (sample_index_ == pushed_samples_.size()) {
      pushed_samples_.clear();
    }

    if (read_count < maxSize) {
      memset(data + read_count, 0, static_cast<size_t>(maxSize - read_count));
    }

    return maxSize;
  }

  memset(data, 0, static_cast<size_t>(maxSize));
  return maxSize;
}

qint64 AudioHybridDevice::writeData(const char *data, qint64 maxSize)
{
  Q_UNUSED(data)
  Q_UNUSED(maxSize)

  // This device doesn't support writing

  return -1;
}
