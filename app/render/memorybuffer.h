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

#ifndef MEMORYBUFFER_H
#define MEMORYBUFFER_H

#include <QVector>

#include "pixelformat.h"

class MemoryBuffer
{
public:
  MemoryBuffer();

  void Create(int width, int height, const olive::PixelFormat &format);

  const int& width() const;
  const int& height() const;
  const olive::PixelFormat& format() const;
  uint8_t* data();
  const uint8_t* const_data() const;

private:
  QVector<uint8_t> data_;
  int width_;
  int height_;
  olive::PixelFormat format_;
};

#endif // MEMORYBUFFER_H
