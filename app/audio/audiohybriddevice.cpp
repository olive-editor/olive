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
