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

#ifndef FFMPEGFRAMEPOOL_H
#define FFMPEGFRAMEPOOL_H

#include "common/memorypool.h"
#include "render/videoparams.h"

OLIVE_NAMESPACE_ENTER

class FFmpegFramePool : public MemoryPool<uint8_t>
{
public:
  FFmpegFramePool(int element_count);

  void SetParameters(int width, int height, VideoParams::Format format, int channel_count);

  const int& width() const
  {
    return width_;
  }

  const int& height() const
  {
    return height_;
  }

protected:
  virtual size_t GetElementSize() override;

private:
  int width_;

  int height_;

  VideoParams::Format format_;

  int channel_count_;

};

OLIVE_NAMESPACE_EXIT

#endif // FFMPEGFRAMEPOOL_H
