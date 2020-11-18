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

#include "outputdeviceproxy.h"

#include "audiomanager.h"

namespace olive {

AudioOutputDeviceProxy::AudioOutputDeviceProxy(QObject *parent) :
  QIODevice(parent),
  device_(nullptr)
{
}

void AudioOutputDeviceProxy::SetParameters(const AudioParams &params)
{
  params_ = params;
}

void AudioOutputDeviceProxy::SetDevice(QIODevice* device, qint64 offset, int playback_speed)
{
  if (device_) {
    delete device_;
  }

  device_ = device;
  device_->setParent(this);

  if (!device_->open(QFile::ReadOnly)) {
    qCritical() << "Failed to open IO device for audio playback";
    delete device_;
    device_ = nullptr;
    return;
  }

  device_->seek(offset);

  playback_speed_ = playback_speed;

  if (qAbs(playback_speed_) != 1) {
    tempo_processor_.Open(params_, qAbs(playback_speed_));
  }
}

void AudioOutputDeviceProxy::close()
{
  QIODevice::close();

  delete device_;
  device_ = nullptr;

  if (tempo_processor_.IsOpen()) {
    tempo_processor_.Close();
  }
}

qint64 AudioOutputDeviceProxy::readData(char *data, qint64 maxlen)
{
  if (!device_) {
    return 0;
  }

  qint64 read_count;

  if (tempo_processor_.IsOpen()) {

    while ((read_count = tempo_processor_.Pull(data, static_cast<int>(maxlen))) == 0) {
      int dev_read = static_cast<int>(ReverseAwareRead(data, maxlen));

      if (!dev_read) {
        break;
      }

      tempo_processor_.Push(data, dev_read);
    }

  } else {
    // If we aren't doing any tempo processing, simply passthrough the read signal
    read_count = ReverseAwareRead(data, maxlen);
  }

  return read_count;
}

qint64 AudioOutputDeviceProxy::writeData(const char *data, qint64 maxSize)
{
  Q_UNUSED(data)
  Q_UNUSED(maxSize)

  return 0;
}

qint64 AudioOutputDeviceProxy::ReverseAwareRead(char *data, qint64 maxlen)
{
  qint64 new_pos = -1;

  if (playback_speed_ < 0) {
    // If we're reversing, we'll seek back by maxlen bytes before we read
    new_pos = device_->pos() - maxlen;

    if (new_pos < 0) {
      maxlen = device_->pos();

      new_pos = 0;
    }

    device_->seek(new_pos);
  }

  qint64 read_count = device_->read(data, maxlen);

  if (playback_speed_ < 0) {
    device_->seek(new_pos);

    // Reverse the samples here
    AudioManager::ReverseBuffer(data, static_cast<int>(read_count), params_.samples_to_bytes(1));
  }

  return read_count;
}

}
