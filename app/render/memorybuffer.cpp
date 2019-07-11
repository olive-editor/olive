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

#include "memorybuffer.h"

MemoryBuffer::MemoryBuffer()
{
}

void MemoryBuffer::Create(int width, int height, const olive::PixelFormat &format)
{
  width_ = width;
  height_ = height;
  format_ = format;

  data_.resize(PixelService::GetBufferSize(format, width, height));
}

const int &MemoryBuffer::width() const
{
  return width_;
}

const int &MemoryBuffer::height() const
{
  return height_;
}

const olive::PixelFormat &MemoryBuffer::format() const
{
  return format_;
}

uint8_t *MemoryBuffer::data()
{
  return data_.data();
}

const uint8_t *MemoryBuffer::const_data() const
{
  return data_.constData();
}
