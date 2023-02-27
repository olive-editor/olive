/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include "common/define.h"
#include "common/oiioutils.h"
#include "config/config.h"
#include "core.h"
#include "render/renderer.h"

namespace olive {

QStringList OIIODecoder::supported_formats_;

OIIODecoder::OIIODecoder() :
  image_(nullptr)
{
}

QString OIIODecoder::id() const
{
  return QStringLiteral("oiio");
}

FootageDescription OIIODecoder::Probe(const QString &filename, CancelAtom *cancelled) const
{
  Q_UNUSED(cancelled)

  FootageDescription desc(id());

  // Filter out any file extensions that aren't expected to work - sometimes OIIO will crash trying
  // to open a file that it can't if it's given one
  if (!FileTypeIsSupported(filename)) {
    return desc;
  }

  std::string std_filename = filename.toStdString();

  auto in = OIIO::ImageInput::open(std_filename);

  if (!in) {
    return desc;
  }

  // Filter out OIIO detecting an "FFmpeg movie", we have a native FFmpeg decoder that can handle
  // it better
  if (!strcmp(in->format_name(), "FFmpeg movie")) {
    return desc;
  }

  bool stream_enabled = true;

  int i;
  for (i=0; in->seek_subimage(i, 0); i++) {
    OIIO::ImageSpec spec = in->spec();

    VideoParams video_params = GetVideoParamsFromImageSpec(spec);

    video_params.set_stream_index(i);

    if (i > 1) {
      // This is a multilayer image and this image might have an offset
      OIIO::ImageSpec root_spec = in->spec(0);

      float norm_x = spec.x + float(spec.width)*0.5f - float(root_spec.width)*0.5f;
      float norm_y = spec.y + float(spec.height)*0.5f - float(root_spec.height)*0.5f;

      video_params.set_x(norm_x);
      video_params.set_y(norm_y);
    }

    // By default, only enable the first subimage (presumably the combined image). Later we will
    // ask the user if they want to enable the layers instead.
    video_params.set_enabled(stream_enabled);
    stream_enabled = false;

    // OIIO automatically premultiplies alpha
    // FIXME: We usually disassociate the alpha for the color management later, for 8-bit images this
    //        likely reduces the fidelity?
    video_params.set_premultiplied_alpha(true);

    desc.AddVideoStream(video_params);
  }

  desc.SetStreamCount(i);

  // If we're here, we have a successful image open
  in->close();

  return desc;
}

bool OIIODecoder::OpenInternal()
{
  // If we can open the filename provided, assume everything is working
  return OpenImageHandler(stream().filename(), stream().stream());
}

TexturePtr OIIODecoder::RetrieveVideoInternal(const RetrieveVideoParams &p)
{
  VideoParams vp = GetVideoParamsFromImageSpec(image_->spec());
  vp.set_divider(p.divider);

  if (!buffer_.is_allocated()
      || last_params_.divider != p.divider) {
    last_params_ = p;

    buffer_.destroy();
    buffer_.set_video_params(vp);
    buffer_.allocate();

    if (p.divider == 1) {
      // Just upload straight to the buffer
      image_->read_image(oiio_pix_fmt_, buffer_.data(), OIIO::AutoStride, buffer_.linesize_bytes());
    } else {
      OIIO::ImageBuf buf(image_->spec());
      image_->read_image(image_->spec().format, buf.localpixels(), buf.pixel_stride(), buf.scanline_stride(), buf.z_stride());

      // Roughly downsample image for divider (for some reason OIIO::ImageBufAlgo::resample failed here)
      int px_sz = vp.GetBytesPerPixel();
      for (int dst_y=0; dst_y<buffer_.height(); dst_y++) {
        int src_y = dst_y * buf.spec().height / buffer_.height();

        for (int dst_x=0; dst_x<buffer_.width(); dst_x++) {
          int src_x = dst_x * buf.spec().width / buffer_.width();
          memcpy(buffer_.data() + buffer_.linesize_bytes() * dst_y + px_sz * dst_x,
                 static_cast<uint8_t*>(buf.localpixels()) + buf.scanline_stride() * src_y + px_sz * src_x,
                 px_sz);
        }
      }
    }
  }

  return p.renderer->CreateTexture(vp, buffer_.data(), buffer_.linesize_pixels());
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

  if (!supported_formats_.contains(QFileInfo(fn).suffix(), Qt::CaseInsensitive)) {
    return false;
  }

  return true;
}

bool OIIODecoder::OpenImageHandler(const QString &fn, int subimage)
{
  image_ = OIIO::ImageInput::open(fn.toStdString());

  if (!image_) {
    return false;
  }

  if (!image_->seek_subimage(subimage, 0)) {
    return false;
  }

  // Check if we can work with this pixel format
  const OIIO::ImageSpec& spec = image_->spec();

  // We use RGBA frames because that tends to be the native format of GPUs
  pix_fmt_ = OIIOUtils::GetFormatFromOIIOBasetype(static_cast<OIIO::TypeDesc::BASETYPE>(spec.format.basetype));

  if (pix_fmt_ == PixelFormat::INVALID) {
    qWarning() << "Failed to convert OIIO::ImageDesc to native pixel format";
    return false;
  }

  oiio_pix_fmt_ = OIIOUtils::GetOIIOBaseTypeFromFormat(pix_fmt_);

  if (oiio_pix_fmt_ == OIIO::TypeDesc::UNKNOWN) {
    qCritical() << "Failed to determine appropriate OIIO basetype from native format";
    return false;
  }

  return true;
}

void OIIODecoder::CloseImageHandle()
{
  if (image_) {
    image_->close();
    image_ = nullptr;
  }

  buffer_.destroy();
}

VideoParams OIIODecoder::GetVideoParamsFromImageSpec(const OIIO::ImageSpec &spec)
{
  VideoParams video_params;

  video_params.set_width(spec.width);
  video_params.set_height(spec.height);
  video_params.set_format(OIIOUtils::GetFormatFromOIIOBasetype(static_cast<OIIO::TypeDesc::BASETYPE>(spec.format.basetype)));
  video_params.set_channel_count(spec.nchannels);
  video_params.set_pixel_aspect_ratio(OIIOUtils::GetPixelAspectRatioFromOIIO(spec));
  video_params.set_video_type(VideoParams::kVideoTypeStill);

  return video_params;
}

}
