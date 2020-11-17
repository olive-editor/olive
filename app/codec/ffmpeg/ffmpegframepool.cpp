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

#include "ffmpegframepool.h"

#include "codec/frame.h"

namespace olive {

FFmpegFramePool::FFmpegFramePool(int element_count) :
  MemoryPool(element_count),
  width_(0),
  height_(0),
  format_(VideoParams::kFormatInvalid),
  channel_count_(0)
{
}

void FFmpegFramePool::SetParameters(int width, int height, VideoParams::Format format, int channel_count)
{
  Clear();

  width_ = width;
  height_ = height;
  format_ = format;
  channel_count_ = channel_count;
}

size_t FFmpegFramePool::GetElementSize()
{
  return Frame::generate_linesize_bytes(width_, format_, channel_count_) * height_;
}

}
