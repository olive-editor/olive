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

extern "C" {
#include <libavutil/imgutils.h>
}

OLIVE_NAMESPACE_ENTER

FFmpegFramePool::FFmpegFramePool(int element_count) :
  MemoryPool(element_count),
  width_(0),
  height_(0),
  format_(AV_PIX_FMT_NONE)
{
}

void FFmpegFramePool::SetParameters(int width, int height, AVPixelFormat format)
{
  Clear();

  width_ = width;
  height_ = height;
  format_ = format;
}

size_t FFmpegFramePool::GetElementSize()
{
  int buf_sz = av_image_get_buffer_size(static_cast<AVPixelFormat>(format_),
                                        width_,
                                        height_,
                                        1);

  if (buf_sz < 0) {
    qDebug() << "Failed to find buffer size:" << buf_sz;
    return 0;
  }

  return buf_sz;
}

OLIVE_NAMESPACE_EXIT
