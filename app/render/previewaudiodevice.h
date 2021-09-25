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

  virtual qint64 writeData(const char *, qint64) override;

  void Push(const QByteArray &b);

private:
  enum LockMethod {
    kDontLock,
    kTryLock,
    kFullLock
  };

  bool SwapBuffers(LockMethod m);

  QMutex lock_;

  QByteArray internal_buffer_[2];

  QByteArray *using_;
  QByteArray *pushing_;

  QAtomicInt swap_requested_;

};

}

#endif // PREVIEWAUDIODEVICE_H
