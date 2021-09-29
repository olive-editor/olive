/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

void AudioOutputDeviceProxy::SetDevice(std::shared_ptr<QIODevice> device)
{
  device_ = device;

  if (!device_->open(QFile::ReadOnly)) {
    qCritical() << "Failed to open IO device for audio playback";
    device_ = nullptr;
    return;
  }
}

void AudioOutputDeviceProxy::close()
{
  QIODevice::close();

  device_ = nullptr;
}

qint64 AudioOutputDeviceProxy::readData(char *data, qint64 maxlen)
{
  if (!device_) {
    return 0;
  }

  return device_->read(data, maxlen);
}

qint64 AudioOutputDeviceProxy::writeData(const char *data, qint64 maxSize)
{
  Q_UNUSED(data)
  Q_UNUSED(maxSize)

  return -1;
}

}
