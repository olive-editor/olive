/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef PLANARFILEDEVICE_H
#define PLANARFILEDEVICE_H

#include <QFile>
#include <QObject>

namespace olive {

class PlanarFileDevice : public QObject
{
  Q_OBJECT
public:
  PlanarFileDevice(QObject *parent = nullptr);

  virtual ~PlanarFileDevice() override;

  bool isOpen() const
  {
    return !files_.isEmpty();
  }

  bool open(const QVector<QString> &filenames, QIODevice::OpenMode mode);

  qint64 read(char **data, qint64 bytes_per_channel, qint64 offset = 0);

  qint64 write(const char **data, qint64 bytes_per_channel, qint64 offset = 0);

  qint64 size() const;

  bool seek(qint64 pos);

  void close();

private:
  QVector<QFile*> files_;

};

}

#endif // PLANARFILEDEVICE_H
