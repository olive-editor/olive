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

#include "planarfiledevice.h"

namespace olive {

PlanarFileDevice::PlanarFileDevice(QObject *parent) :
  QObject(parent)
{

}

PlanarFileDevice::~PlanarFileDevice()
{
  close();
}

bool PlanarFileDevice::open(const QVector<QString> &filenames, QIODevice::OpenMode mode)
{
  if (isOpen()) {
    // Already open
    return false;
  }

  files_.resize(filenames.size());
  files_.fill(nullptr);

  for (int i=0; i<files_.size(); i++) {
    files_[i] = new QFile(filenames.at(i));
    if (!files_[i]->open(mode)) {
      close();
      return false;
    }
  }

  return true;
}

qint64 PlanarFileDevice::read(char **data, qint64 bytes_per_channel, qint64 offset)
{
  qint64 ret = -1;

  if (isOpen()) {
    for (int i=0; i<files_.size(); i++) {
      // Kind of clunky but should be largely fine
      ret = files_[i]->read(data[i] + offset, bytes_per_channel);
    }
  }

  return ret;
}

qint64 PlanarFileDevice::write(const char **data, qint64 bytes_per_channel, qint64 offset)
{
  qint64 ret = -1;

  if (isOpen()) {
    for (int i=0; i<files_.size(); i++) {
      // Kind of clunky but should be largely fine
      ret = files_[i]->write(data[i] + offset, bytes_per_channel);
    }
  }

  return ret;
}

qint64 PlanarFileDevice::size() const
{
  if (isOpen()) {
    return files_.first()->size();
  } else {
    return 0;
  }
}

bool PlanarFileDevice::seek(qint64 pos)
{
  bool ret = true;

  for (int i=0; i<files_.size(); i++) {
    ret = files_[i]->seek(pos) & ret;
  }

  return ret;
}

void PlanarFileDevice::close()
{
  for (int i=0; i<files_.size(); i++) {
    QFile *f = files_.at(i);
    if (f) {
      if (f->isOpen()) {
        f->close();
      }
      delete f;
    }
  }
  files_.clear();
}

}
