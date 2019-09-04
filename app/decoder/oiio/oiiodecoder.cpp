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

#include "oiiodecoder.h"

#include <QDebug>

OIIODecoder::OIIODecoder()
{
}

QString OIIODecoder::id()
{
  return "oiio";
}

bool OIIODecoder::Probe(Footage *f)
{
  auto in = OIIO::ImageInput::open(f->filename().toStdString());

  if (!in) {
    return false;
  }

  // Get stats for this image and dump them into the Footage file
  const OIIO::ImageSpec& spec = in->spec();

  ImageStreamPtr image_stream = std::make_shared<ImageStream>();
  image_stream->set_width(spec.width);
  image_stream->set_height(spec.height);
  f->add_stream(image_stream);

  // If we're here, we have a successful image open
  in->close();

  return true;
}

bool OIIODecoder::Open()
{
  image_ = OIIO::ImageInput::open(stream()->footage()->filename().toStdString());

  if (!image_) {
    return false;
  }

  // Check if we can work with this pixel format
  const OIIO::ImageSpec& spec = image_->spec();

  width_ = spec.width;
  height_ = spec.height;

  // Weirdly, compiler complains this is a boolean value without casting to int
  switch (static_cast<int>(spec.format)) {
  case OIIO::TypeDesc::UINT8:
    pix_fmt_ = olive::PIX_FMT_RGBA8;
    break;
  case OIIO::TypeDesc::UINT16:
    pix_fmt_ = olive::PIX_FMT_RGBA16;
    break;
  case OIIO::TypeDesc::HALF:
    pix_fmt_ = olive::PIX_FMT_RGBA16F;
    break;
  case OIIO::TypeDesc::FLOAT:
    pix_fmt_ = olive::PIX_FMT_RGBA32F;
    break;
  default:
    qWarning() << "Failed to convert OIIO::ImageDesc to native pixel format";
    return false;
  }

  pix_fmt_info_ = PixelService::GetPixelFormatInfo(static_cast<olive::PixelFormat>(pix_fmt_));

  return true;
}

FramePtr OIIODecoder::Retrieve(const rational &timecode, const rational &length)
{
  Q_UNUSED(timecode)
  Q_UNUSED(length)

  FramePtr f = Frame::Create();

  f->set_width(width_);
  f->set_height(height_);
  f->set_format(pix_fmt_);
  f->allocate();

  // Use the native format to determine what format OIIO should return
  // FIXME: Behavior of RGB images as opposed to RGBA?
  image_->read_image(pix_fmt_info_.oiio_desc, f->data());

  return f;
}

void OIIODecoder::Close()
{
  image_->close();
  image_ = nullptr;
}

int64_t OIIODecoder::GetTimestampFromTime(const rational &time)
{
  Q_UNUSED(time)

  // A still image will always return the same frame

  return 0;
}
