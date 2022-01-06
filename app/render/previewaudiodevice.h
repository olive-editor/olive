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

#ifndef PREVIEWAUDIODEVICE_H
#define PREVIEWAUDIODEVICE_H

#include "previewautocacher.h"

namespace olive {

class PreviewAudioDevice : public QIODevice
{
  Q_OBJECT
public:
  PreviewAudioDevice(QObject *parent = nullptr);

  virtual ~PreviewAudioDevice() override;

  void StartQueuing();

  virtual bool isSequential() const override;

  virtual qint64 readData(char *data, qint64 maxSize) override;

  virtual qint64 writeData(const char *data, qint64 length) override;

  int bytes_per_frame() const
  {
    return bytes_per_frame_;
  }

  void set_bytes_per_frame(int b)
  {
    bytes_per_frame_ = b;
  }

  void set_notify_interval(qint64 i)
  {
    notify_interval_ = i;
  }

  void clear();

signals:
  void Notify();

private:
  QMutex lock_;

  QByteArray buffer_;

  int bytes_per_frame_;

  qint64 notify_interval_;

  qint64 bytes_read_;

};

}

#endif // PREVIEWAUDIODEVICE_H
