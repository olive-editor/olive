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

#include "oiiodecoder.h"

#include <OpenImageIO/imagebufalgo.h>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "common/define.h"
#include "common/oiioutils.h"
#include "config/config.h"
#include "core.h"

namespace olive {

QStringList OIIODecoder::supported_formats_;

OIIODecoder::OIIODecoder() :
  image_(nullptr),
  buffer_(nullptr)
{
}

OIIODecoder::~OIIODecoder()
{
  CloseInternal();
}

QString OIIODecoder::id()
{
  return QStringLiteral("oiio");
}

Streams OIIODecoder::Probe(const QString &filename, const QAtomicInt* cancelled) const
{
  Q_UNUSED(cancelled)

  Streams streams;

  // Filter out any file extensions that aren't expected to work - sometimes OIIO will crash trying
  // to open a file that it can't if it's given one
  if (!FileTypeIsSupported(filename)) {
    return streams;
  }

  std::string std_filename = filename.toStdString();

  auto in = OIIO::ImageInput::open(std_filename);

  if (!in) {
    return streams;
  }

  // Filter out OIIO detecting an "FFmpeg movie", we have a native FFmpeg decoder that can handle
  // it better
  if (!strcmp(in->format_name(), "FFmpeg movie")) {
    return streams;
  }

  Stream stream(Stream::kVideo);

  stream.set_width(in->spec().width);
  stream.set_height(in->spec().height);
  stream.set_pixel_format(OIIOUtils::GetFormatFromOIIOBasetype(static_cast<OIIO::TypeDesc::BASETYPE>(in->spec().format.basetype)));
  stream.set_channel_count(in->spec().nchannels);
  stream.set_pixel_aspect_ratio(OIIOUtils::GetPixelAspectRatioFromOIIO(in->spec()));
  stream.set_video_type(Stream::kVideoTypeStill);

  // OIIO automatically premultiplies alpha
  // FIXME: We usually disassociate the alpha for the color management later, for 8-bit images this
  //        likely reduces the fidelity?
  stream.set_premultiplied_alpha(true);

  streams.append(stream);

  // If we're here, we have a successful image open
  in->close();

  return streams;
}

bool OIIODecoder::OpenInternal()
{
  // If we can open the filename provided, assume everything is working
  return OpenImageHandler(stream().filename());
}

FramePtr OIIODecoder::RetrieveVideoInternal(const rational &timecode, const int& divider)
{
  Q_UNUSED(timecode)

  FramePtr frame = Frame::Create();

  frame->set_video_params(VideoParams(buffer_->spec().width,
                                      buffer_->spec().height,
                                      pix_fmt_,
                                      channel_count_,
                                      OIIOUtils::GetPixelAspectRatioFromOIIO(buffer_->spec()),
                                      VideoParams::kInterlaceNone, // FIXME: Does OIIO deinterlace for us?
                                      divider));
  frame->allocate();

  if (divider == 1) {

    OIIOUtils::BufferToFrame(buffer_, frame.get());

  } else {

    // Will need to resize the image
    OIIO::ImageBuf dst(OIIO::ImageSpec(frame->width(), frame->height(), buffer_->spec().nchannels, buffer_->spec().format));

    if (!OIIO::ImageBufAlgo::resample(dst, *buffer_)) {
      qWarning() << "OIIO resize failed";
    }

    OIIOUtils::BufferToFrame(&dst, frame.get());

  }

  return frame;
}

void OIIODecoder::CloseInternal()
{
  CloseImageHandle();
}

bool OIIODecoder::FileTypeIsSupported(const QString& fn)
{
  // We prioritize OIIO over FFmpeg to pick up still images more effectively, but some OIIO decoders (notably OpenJPEG)
  // will segfault entirely if given unexpected data (an MPEG-4 for instance). To workaround this issue, we use OIIO's
  // "extension_list" attribute and match it with the extension of the file.

  // Check if we've created the supported formats list, create it if not
  if (supported_formats_.isEmpty()) {
    QStringList extension_list = QString::fromStdString(OIIO::get_string_attribute("extension_list")).split(';');

    // The format of "extension_list" is "format:ext", we want to separate it into a simple list of extensions
    foreach (const QString& ext, extension_list) {
      QStringList format_and_ext = ext.split(':');

      supported_formats_.append(format_and_ext.at(1).split(','));
    }
  }

  if (!supported_formats_.contains(QFileInfo(fn).completeSuffix(), Qt::CaseInsensitive)) {
    return false;
  }

  return true;
}

bool OIIODecoder::OpenImageHandler(const QString &fn)
{
  image_ = OIIO::ImageInput::open(fn.toStdString());

  if (!image_) {
    return false;
  }

  // Check if we can work with this pixel format
  const OIIO::ImageSpec& spec = image_->spec();

  // Store channel count
  channel_count_ = spec.nchannels;

  // We use RGBA frames because that tends to be the native format of GPUs
  pix_fmt_ = OIIOUtils::GetFormatFromOIIOBasetype(static_cast<OIIO::TypeDesc::BASETYPE>(spec.format.basetype));

  if (pix_fmt_ == VideoParams::kFormatInvalid) {
    qWarning() << "Failed to convert OIIO::ImageDesc to native pixel format";
    return false;
  }

  OIIO::TypeDesc::BASETYPE type = OIIOUtils::GetOIIOBaseTypeFromFormat(pix_fmt_);

  if (type == OIIO::TypeDesc::UNKNOWN) {
    qCritical() << "Failed to determine appropriate OIIO basetype from native format";
    return false;
  }

  buffer_ = new OIIO::ImageBuf(OIIO::ImageSpec(spec.width, spec.height, spec.nchannels, type),
                               OIIO::InitializePixels::No);

  image_->read_image(type, buffer_->localpixels());

  return true;
}

void OIIODecoder::CloseImageHandle()
{
  if (image_) {
    image_->close();
    image_ = nullptr;
  }

  if (buffer_) {
    delete buffer_;
    buffer_ = nullptr;
  }
}

}
