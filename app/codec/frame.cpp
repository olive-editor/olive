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

#include "frame.h"

#include <OpenImageIO/imagebuf.h>
#include <QDebug>
#include <QtGlobal>
#include <QtMath>

#include "common/oiioutils.h"

namespace olive {

Frame::Frame() :
  timestamp_(0)
{
}

FramePtr Frame::Create()
{
  return std::make_shared<Frame>();
}

const VideoParams &Frame::video_params() const
{
  return params_;
}

void Frame::set_video_params(const VideoParams &params)
{
  params_ = params;

  linesize_ = generate_linesize_bytes(width(), params_.format(), params_.channel_count());
  linesize_pixels_ = linesize_ / params_.GetBytesPerPixel();
}

int Frame::generate_linesize_bytes(int width, VideoParams::Format format, int channel_count)
{
  // Align to 32 bytes (not sure if this is necessary?)
  return VideoParams::GetBytesPerPixel(format, channel_count) * ((width + 31) & ~31);
}

Color Frame::get_pixel(int x, int y) const
{
  if (!contains_pixel(x, y)) {
    return Color();
  }

  int byte_offset = y * linesize_bytes() + x * video_params().GetBytesPerPixel();

  return Color(data_.data() + byte_offset, video_params().format(), video_params().channel_count());
}

bool Frame::contains_pixel(int x, int y) const
{
  return (is_allocated() && x >= 0 && x < width() && y >= 0 && y < height());
}

void Frame::set_pixel(int x, int y, const Color &c)
{
  if (!contains_pixel(x, y)) {
    return;
  }

  int byte_offset = y * linesize_bytes() + x * video_params().GetBytesPerPixel();

  c.toData(data_.data() + byte_offset, video_params().format(), video_params().channel_count());
}

bool Frame::allocate()
{
  // Assume this frame is intended to be a video frame
  if (!params_.is_valid()) {
    qWarning() << "Tried to allocate a frame with invalid parameters";
    return false;
  }

  data_.resize(VideoParams::GetBufferSize(linesize_, height(), params_.format(), params_.channel_count()));

  return true;
}

FramePtr Frame::convert(VideoParams::Format format) const
{
  // Create new params with destination format
  VideoParams params = params_;
  params.set_format(format);

  // Create new frame
  FramePtr converted = Frame::Create();
  converted->set_video_params(params);
  converted->set_timestamp(timestamp_);
  converted->allocate();

  // Do the conversion through OIIO for convenience
  OIIO::ImageBuf src(OIIO::ImageSpec(width(), height(),
                                     channel_count(),
                                     OIIOUtils::GetOIIOBaseTypeFromFormat(this->format())));

  OIIOUtils::FrameToBuffer(this, &src);

  OIIO::ImageBuf dst(OIIO::ImageSpec(converted->width(), converted->height(),
                                     channel_count(),
                                     OIIOUtils::GetOIIOBaseTypeFromFormat(format)));

  if (dst.copy_pixels(src)) {
    OIIOUtils::BufferToFrame(&dst, converted.get());
    return converted;
  } else {
    return nullptr;
  }
}

}
