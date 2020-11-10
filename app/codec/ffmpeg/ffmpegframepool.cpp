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

#include "ffmpegframepool.h"

#include "codec/frame.h"

OLIVE_NAMESPACE_ENTER

FFmpegFramePool::FFmpegFramePool(int element_count) :
  MemoryPool(element_count),
  width_(0),
  height_(0),
  format_(PixelFormat::PIX_FMT_INVALID)
{
}

void FFmpegFramePool::SetParameters(int width, int height, PixelFormat::Format format)
{
  Clear();

  width_ = width;
  height_ = height;
  format_ = format;
}

size_t FFmpegFramePool::GetElementSize()
{
  return Frame::generate_linesize_bytes(width_, format_) * height_;
}

OLIVE_NAMESPACE_EXIT
