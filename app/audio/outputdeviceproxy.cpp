#include "outputdeviceproxy.h"

#include "bufferaverage.h"

AudioOutputDeviceProxy::AudioOutputDeviceProxy() :
  device_(nullptr),
  send_averages_(false)
{
}

void AudioOutputDeviceProxy::SetDevice(QIODevice *device)
{
  device_ = device;
}

void AudioOutputDeviceProxy::SetSendAverages(bool send)
{
  send_averages_ = send;
}

qint64 AudioOutputDeviceProxy::readData(char *data, qint64 maxlen)
{
  if (device_) {
    qint64 read_count = device_->read(data, maxlen);

    if (send_averages_ && read_count > 0) {
      emit ProcessedAverages(AudioBufferAverage::ProcessAverages(data, static_cast<int>(read_count)));
    }

    return read_count;
  }

  return 0;
}

qint64 AudioOutputDeviceProxy::writeData(const char *data, qint64 maxSize)
{
  return -1;
}
