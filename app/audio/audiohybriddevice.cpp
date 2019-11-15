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
  sample_index_(0),
  enable_sending_samples_(false)
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

  if (device_ != nullptr) {
    device_->close();
    device_ = nullptr;
  }
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

void AudioHybridDevice::SetEnableSendingSamples(bool e)
{
  enable_sending_samples_ = e;
}

qint64 AudioHybridDevice::readData(char *data, qint64 maxSize)
{
  qint64 read_size = read_internal(data, maxSize);

  if (enable_sending_samples_ && read_size > 0) {
    // FIXME: Assumes float and stereo
    float* samples = reinterpret_cast<float*>(data);
    int sample_count = static_cast<int>(read_size / static_cast<int>(sizeof(float)));
    int channels = 2;

    // Create array of samples to send
    QVector<double> averages(channels);
    averages.fill(0);

    // Add all samples together
    for (int i=0;i<sample_count;i++) {
      averages[i%channels] = qMax(averages[i%channels], static_cast<double>(qAbs(samples[i])));
    }

    emit SentSamples(averages);
  }

  return read_size;
}

qint64 AudioHybridDevice::writeData(const char *data, qint64 maxSize)
{
  Q_UNUSED(data)
  Q_UNUSED(maxSize)

  // This device doesn't support writing

  return -1;
}

qint64 AudioHybridDevice::read_internal(char *data, qint64 maxSize)
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
