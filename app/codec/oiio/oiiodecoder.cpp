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

#include "common/define.h"

OIIODecoder::OIIODecoder() :
  image_(nullptr),
  frame_(nullptr)
{
}

QString OIIODecoder::id()
{
  return "oiio";
}

bool OIIODecoder::Probe(Footage *f)
{
  std::string std_filename = f->filename().toStdString();

  auto in = OIIO::ImageInput::open(std_filename);

  if (!in) {
    return false;
  }

  if (!strcmp(in->format_name(), "FFmpeg movie")) {
    // If this is FFmpeg via OIIO, fall-through to our native FFmpeg decoder
    return false;
  }

  // Get stats for this image and dump them into the Footage file
  const OIIO::ImageSpec& spec = in->spec();

  ImageStreamPtr image_stream = std::make_shared<ImageStream>();
  image_stream->set_width(spec.width);
  image_stream->set_height(spec.height);

  // OIIO automatically premultiplies alpha
  // FIXME: We usually disassociate the alpha for the color management later, for 8-bit images this likely reduces the
  //        fidelity?
  image_stream->set_premultiplied_alpha(true);

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

  // Weirdly, switch statement doesn't work correctly here
  if (spec.format == OIIO::TypeDesc::UINT8) {
    pix_fmt_ = PixelFormat::PIX_FMT_RGBA8;
  } else if (spec.format == OIIO::TypeDesc::UINT16) {
    pix_fmt_ = PixelFormat::PIX_FMT_RGBA16U;
  } else if (spec.format == OIIO::TypeDesc::HALF) {
    pix_fmt_ = PixelFormat::PIX_FMT_RGBA16F;
  } else if (spec.format == OIIO::TypeDesc::FLOAT) {
    pix_fmt_ = PixelFormat::PIX_FMT_RGBA32F;
  } else {
    qWarning() << "Failed to convert OIIO::ImageDesc to native pixel format";
    return false;
  }

  // FIXME: Many OIIO pixel formats are not handled here

  is_rgba_ = (spec.nchannels == kRGBAChannels);

  pix_fmt_info_ = PixelService::GetPixelFormatInfo(static_cast<PixelFormat::Format>(pix_fmt_));

  return true;
}

FramePtr OIIODecoder::RetrieveVideo(const rational &timecode)
{
  if (!open_ && !Open()) {
    return nullptr;
  }

  Q_UNUSED(timecode)

  if (!frame_) {
    frame_ = Frame::Create();

    frame_->set_width(width_);
    frame_->set_height(height_);
    frame_->set_format(pix_fmt_);
    frame_->allocate();

    // Use the native format to determine what format OIIO should return
    // FIXME: Behavior of RGB images as opposed to RGBA?
    image_->read_image(pix_fmt_info_.oiio_desc, frame_->data());

    if (!is_rgba_) {
      PixelService::ConvertRGBtoRGBA(frame_);
    }
  }

  return frame_;
}

void OIIODecoder::Close()
{
  if (image_ != nullptr) {
    image_->close();
#if OIIO_VERSION < 10903
    OIIO::ImageInput::destroy(image_);
#endif
    image_ = nullptr;
  }

  frame_ = nullptr;
}

int64_t OIIODecoder::GetTimestampFromTime(const rational &time)
{
  Q_UNUSED(time)

  // A still image will always return the same frame

  return 0;
}

bool OIIODecoder::SupportsVideo()
{
  return true;
}
