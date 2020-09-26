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

#include <OpenImageIO/imagebufalgo.h>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "common/define.h"
#include "config/config.h"
#include "core.h"

OLIVE_NAMESPACE_ENTER

QStringList OIIODecoder::supported_formats_;

OIIODecoder::OIIODecoder() :
  image_(nullptr),
  buffer_(nullptr)
{
}

QString OIIODecoder::id()
{
  return QStringLiteral("oiio");
}

ItemPtr OIIODecoder::Probe(const QString& filename, const QAtomicInt* cancelled) const
{
  if (!FileTypeIsSupported(filename)) {
    return nullptr;
  }

  std::string std_filename = filename.toStdString();

  auto in = OIIO::ImageInput::open(std_filename);

  if (!in) {
    return nullptr;
  }

  if (!strcmp(in->format_name(), "FFmpeg movie")) {
    // If this is FFmpeg via OIIO, fall-through to our native FFmpeg decoder
    return nullptr;
  }

  FootagePtr footage = std::make_shared<Footage>();

  VideoStreamPtr image_stream = std::make_shared<VideoStream>();

  image_stream->set_width(in->spec().width);
  image_stream->set_height(in->spec().height);
  image_stream->set_format(GetFormatFromOIIOBasetype(in->spec()));
  image_stream->set_pixel_aspect_ratio(GetPixelAspectRatioFromOIIO(in->spec()));
  image_stream->set_video_type(VideoStream::kVideoTypeStill);

  // Images will always have just one stream
  image_stream->set_index(0);

  // OIIO automatically premultiplies alpha
  // FIXME: We usually disassociate the alpha for the color management later, for 8-bit images this
  //        likely reduces the fidelity?
  image_stream->set_premultiplied_alpha(true);

  // Get stats for this image and dump them into the Footage file
  footage->add_stream(image_stream);

  // If we're here, we have a successful image open
  in->close();

#if OIIO_VERSION < 10903
  OIIO::ImageInput::destroy(in);
#endif

  return footage;
}

bool OIIODecoder::Open()
{
  Q_ASSERT(stream());

  if (stream()->type() != Stream::kVideo && !OpenImageHandler(stream()->footage()->filename())) {
    return false;
  }

  open_ = true;

  return true;
}

FramePtr OIIODecoder::RetrieveVideo(const rational &timecode, const int& divider)
{
  if (!open_) {
    return nullptr;
  }

  if (stream()->type() == Stream::kVideo) {
    int64_t ts = std::static_pointer_cast<VideoStream>(stream())->get_time_in_timebase_units(timecode);

    if (!OpenImageHandler(TransformImageSequenceFileName(stream()->footage()->filename(), ts))) {
      return nullptr;
    }
  }

  FramePtr frame = Frame::Create();

  frame->set_video_params(VideoParams(buffer_->spec().width,
                                      buffer_->spec().height,
                                      pix_fmt_,
                                      GetPixelAspectRatioFromOIIO(buffer_->spec()),
                                      VideoParams::kInterlaceNone, // FIXME: Does OIIO deinterlace for us?
                                      divider));
  frame->allocate();

  if (divider == 1) {

    BufferToFrame(buffer_, frame);

  } else {

    // Will need to resize the image
    OIIO::ImageBuf dst(OIIO::ImageSpec(frame->width(), frame->height(), buffer_->spec().nchannels, buffer_->spec().format));

    if (!OIIO::ImageBufAlgo::resample(dst, *buffer_)) {
      qWarning() << "OIIO resize failed";
    }

    BufferToFrame(&dst, frame);

  }

  if (stream()->type() == Stream::kVideo) {
    CloseImageHandle();
  }

  return frame;
}

void OIIODecoder::Close()
{
  CloseImageHandle();
}

bool OIIODecoder::SupportsVideo()
{
  return true;
}

void OIIODecoder::FrameToBuffer(FramePtr frame, OIIO::ImageBuf *buf)
{
#if OIIO_VERSION < 20112
  //
  // Workaround for OIIO bug that ignores destination stride in versions OLDER than 2.1.12
  //
  // See more: https://github.com/OpenImageIO/oiio/pull/2487
  //
  int width_in_bytes = frame->width() * PixelFormat::BytesPerPixel(frame->format());

  for (int i=0;i<buf->spec().height;i++) {
    memcpy(
#if OIIO_VERSION < 10903
          reinterpret_cast<char*>(buf->localpixels()) + i * width_in_bytes,
#else
          reinterpret_cast<char*>(buf->localpixels()) + i * buf->scanline_stride(),
#endif
          frame->data() + i * frame->linesize_bytes(),
          width_in_bytes);
  }
#else
  buf->set_pixels(OIIO::ROI(),
                  buf->spec().format,
                  frame->data(),
                  OIIO::AutoStride,
                  frame->linesize_bytes());
#endif
}

void OIIODecoder::BufferToFrame(OIIO::ImageBuf *buf, FramePtr frame)
{
#if OIIO_VERSION < 20112
  //
  // Workaround for OIIO bug that ignores destination stride in versions OLDER than 2.1.12
  //
  // See more: https://github.com/OpenImageIO/oiio/pull/2487
  //
  int width_in_bytes = frame->width() * PixelFormat::BytesPerPixel(frame->format());

  for (int i=0;i<buf->spec().height;i++) {
    memcpy(frame->data() + i * frame->linesize_bytes(),
#if OIIO_VERSION < 10903
           reinterpret_cast<const char*>(buf->localpixels()) + i * width_in_bytes,
#else
           reinterpret_cast<const char*>(buf->localpixels()) + i * buf->scanline_stride(),
#endif
           width_in_bytes);
  }
#else
  buf->get_pixels(OIIO::ROI(),
                  buf->spec().format,
                  frame->data(),
                  OIIO::AutoStride,
                  frame->linesize_bytes());
#endif
}

PixelFormat::Format OIIODecoder::GetFormatFromOIIOBasetype(const OIIO::ImageSpec& spec)
{
  bool has_alpha = (spec.nchannels == kRGBAChannels);

  if (spec.format == OIIO::TypeDesc::UINT8) {
    return has_alpha ? PixelFormat::PIX_FMT_RGBA8 : PixelFormat::PIX_FMT_RGB8;
  } else if (spec.format == OIIO::TypeDesc::UINT16) {
    return has_alpha ? PixelFormat::PIX_FMT_RGBA16U : PixelFormat::PIX_FMT_RGB16U;
  } else if (spec.format == OIIO::TypeDesc::HALF) {
    return has_alpha ? PixelFormat::PIX_FMT_RGBA16F : PixelFormat::PIX_FMT_RGB16F;
  } else if (spec.format == OIIO::TypeDesc::FLOAT) {
    return has_alpha ? PixelFormat::PIX_FMT_RGBA32F : PixelFormat::PIX_FMT_RGB32F;
  } else {
    return PixelFormat::PIX_FMT_INVALID;
  }
}

rational OIIODecoder::GetPixelAspectRatioFromOIIO(const OIIO::ImageSpec &spec)
{
  return rational::fromDouble(spec.get_float_attribute("PixelAspectRatio", 1));
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

  is_rgba_ = (spec.nchannels == kRGBAChannels);

  pix_fmt_ = GetFormatFromOIIOBasetype(spec);

  if (pix_fmt_ == PixelFormat::PIX_FMT_INVALID) {
    qWarning() << "Failed to convert OIIO::ImageDesc to native pixel format";
    return false;
  }

  // FIXME: Many OIIO pixel formats are not handled here
  OIIO::TypeDesc type = PixelFormat::GetOIIOTypeDesc(pix_fmt_);

#if OIIO_VERSION < 20100
  buffer_ = new OIIO::ImageBuf(OIIO::ImageSpec(spec.width, spec.height, spec.nchannels, type));
#else
  buffer_ = new OIIO::ImageBuf(OIIO::ImageSpec(spec.width, spec.height, spec.nchannels, type), OIIO::InitializePixels::No);
#endif
  image_->read_image(type, buffer_->localpixels());

  return true;
}

void OIIODecoder::CloseImageHandle()
{
  if (image_) {
    image_->close();
#if OIIO_VERSION < 10903
    OIIO::ImageInput::destroy(image_);
#endif
    image_ = nullptr;
  }

  if (buffer_) {
    delete buffer_;
    buffer_ = nullptr;
  }
}

OLIVE_NAMESPACE_EXIT
