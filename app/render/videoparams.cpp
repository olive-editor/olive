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

#include "videoparams.h"

#include <QtMath>

#include "core.h"

namespace olive {

const int VideoParams::kInternalChannelCount = kRGBAChannelCount;

const rational VideoParams::kPixelAspectSquare(1);
const rational VideoParams::kPixelAspectNTSCStandard(8, 9);
const rational VideoParams::kPixelAspectNTSCWidescreen(32, 27);
const rational VideoParams::kPixelAspectPALStandard(16, 15);
const rational VideoParams::kPixelAspectPALWidescreen(64, 45);
const rational VideoParams::kPixelAspect1080Anamorphic(4, 3);

const QVector<rational> VideoParams::kSupportedFrameRates = {
  rational(10, 1),             // 10 FPS
  rational(15, 1),             // 15 FPS
  rational(24000, 1001),       // 23.976 FPS
  rational(24, 1),             // 24 FPS
  rational(25, 1),             // 25 FPS
  rational(30000, 1001),       // 29.97 FPS
  rational(30, 1),             // 30 FPS
  rational(48000, 1001),       // 47.952 FPS
  rational(48, 1),             // 48 FPS
  rational(50, 1),             // 50 FPS
  rational(60000, 1001),       // 59.94 FPS
  rational(60, 1)              // 60 FPS
};

const QVector<int> VideoParams::kSupportedDividers = {1, 2, 3, 4, 6, 8, 12, 16};

const QVector<rational> VideoParams::kStandardPixelAspects = {
  VideoParams::kPixelAspectSquare,
  VideoParams::kPixelAspectNTSCStandard,
  VideoParams::kPixelAspectNTSCWidescreen,
  VideoParams::kPixelAspectPALStandard,
  VideoParams::kPixelAspectPALWidescreen,
  VideoParams::kPixelAspect1080Anamorphic
};

VideoParams::VideoParams() :
  width_(0),
  height_(0),
  depth_(0),
  format_(kFormatInvalid),
  channel_count_(0),
  interlacing_(Interlacing::kInterlaceNone),
  divider_(1)
{
  set_defaults_for_footage();
}

VideoParams::VideoParams(int width, int height, Format format, int nb_channels, const rational& pixel_aspect_ratio, Interlacing interlacing, int divider) :
  width_(width),
  height_(height),
  depth_(1),
  format_(format),
  channel_count_(nb_channels),
  pixel_aspect_ratio_(pixel_aspect_ratio),
  interlacing_(interlacing),
  divider_(divider)
{
  calculate_effective_size();
  validate_pixel_aspect_ratio();
  set_defaults_for_footage();
}

VideoParams::VideoParams(int width, int height, int depth, Format format, int nb_channels, const rational &pixel_aspect_ratio, VideoParams::Interlacing interlacing, int divider) :
  width_(width),
  height_(height),
  depth_(depth),
  format_(format),
  channel_count_(nb_channels),
  pixel_aspect_ratio_(pixel_aspect_ratio),
  interlacing_(interlacing),
  divider_(divider)
{
  calculate_effective_size();
  validate_pixel_aspect_ratio();
  set_defaults_for_footage();
}

VideoParams::VideoParams(int width, int height, const rational &time_base, Format format, int nb_channels, const rational& pixel_aspect_ratio, Interlacing interlacing, int divider) :
  width_(width),
  height_(height),
  depth_(1),
  time_base_(time_base),
  format_(format),
  channel_count_(nb_channels),
  pixel_aspect_ratio_(pixel_aspect_ratio),
  interlacing_(interlacing),
  divider_(divider)
{
  calculate_effective_size();
  validate_pixel_aspect_ratio();
  set_defaults_for_footage();
}

int VideoParams::generate_auto_divider(qint64 width, qint64 height)
{
  // Arbitrary pixel count (from 640x360)
  const int target_res = 230400;

  qint64 megapixels = width * height;

  double squared_divider = double(megapixels) / double(target_res);
  double divider = qSqrt(squared_divider);

  if (divider <= kSupportedDividers.first()) {
    return kSupportedDividers.first();
  } else if (divider >= kSupportedDividers.last()) {
    return kSupportedDividers.last();
  } else {
    for (int i=1; i<kSupportedDividers.size(); i++) {
      int prev_divider = kSupportedDividers.at(i-1);
      int next_divider = kSupportedDividers.at(i);

      if (divider >= prev_divider && divider <= next_divider) {
        double prev_diff = qAbs(prev_divider - divider);
        double next_diff = qAbs(next_divider - divider);

        if (prev_diff < next_diff) {
          return prev_divider;
        } else {
          return next_divider;
        }
      }
    }

    // "Safe" fallback
    return 2;
  }
}

bool VideoParams::operator==(const VideoParams &rhs) const
{
  return width() == rhs.width()
      && height() == rhs.height()
      && time_base() == rhs.time_base()
      && format() == rhs.format()
      && pixel_aspect_ratio() == rhs.pixel_aspect_ratio()
      && divider() == rhs.divider();
}

bool VideoParams::operator!=(const VideoParams &rhs) const
{
  return !(*this == rhs);
}

int VideoParams::GetBytesPerChannel(VideoParams::Format format)
{
  switch (format) {
  case kFormatInvalid:
  case kFormatCount:
    break;
  case kFormatUnsigned8:
    return 1;
  case kFormatUnsigned16:
  case kFormatFloat16:
    return 2;
  case kFormatFloat32:
    return 4;
  }

  return 0;
}

int VideoParams::GetBytesPerPixel(VideoParams::Format format, int channels)
{
  return GetBytesPerChannel(format) * channels;
}

bool VideoParams::FormatIsFloat(VideoParams::Format format)
{
  switch (format) {
  case kFormatFloat16:
  case kFormatFloat32:
    return true;
  case kFormatUnsigned8:
  case kFormatUnsigned16:
  case kFormatInvalid:
  case kFormatCount:
    break;
  }

  return false;
}

QString VideoParams::GetFormatName(VideoParams::Format format)
{
  switch (format) {
  case kFormatUnsigned8:
    return QCoreApplication::translate("VideoParams", "8-bit");
  case kFormatUnsigned16:
    return QCoreApplication::translate("VideoParams", "16-bit Integer");
  case kFormatFloat16:
    return QCoreApplication::translate("VideoParams", "Half-Float (16-bit)");
  case kFormatFloat32:
    return QCoreApplication::translate("VideoParams", "Full-Float (32-bit)");
  case kFormatInvalid:
  case kFormatCount:
    break;
  }

  return QCoreApplication::translate("VideoParams", "Unknown (0x%1)").arg(format, 0, 16);
}

void VideoParams::calculate_effective_size()
{
  effective_width_ = GetScaledDimension(width(), divider_);
  effective_height_ = GetScaledDimension(height(), divider_);
  effective_depth_ = GetScaledDimension(depth(), divider_);
}

void VideoParams::validate_pixel_aspect_ratio()
{
  if (pixel_aspect_ratio_.isNull()) {
    pixel_aspect_ratio_ = 1;
  }
}

void VideoParams::set_defaults_for_footage()
{
  enabled_ = true;
  stream_index_ = 0;
  video_type_ = kVideoTypeVideo;
  start_time_ = 0;
  duration_ = 0;
  premultiplied_alpha_ = false;
}

bool VideoParams::is_valid() const
{
  return (width() > 0
          && height() > 0
          && !pixel_aspect_ratio_.isNull()
          && format_ > kFormatInvalid && format_ < kFormatCount
          && channel_count_ > 0);
}

QString VideoParams::FrameRateToString(const rational &frame_rate)
{
  return QCoreApplication::translate("VideoParams", "%1 FPS").arg(frame_rate.toDouble());
}

QStringList VideoParams::GetStandardPixelAspectRatioNames()
{
  QStringList strings = {
    QCoreApplication::translate("VideoParams", "Square Pixels (%1)"),
    QCoreApplication::translate("VideoParams", "NTSC Standard (%1)"),
    QCoreApplication::translate("VideoParams", "NTSC Widescreen (%1)"),
    QCoreApplication::translate("VideoParams", "PAL Standard (%1)"),
    QCoreApplication::translate("VideoParams", "PAL Widescreen (%1)"),
    QCoreApplication::translate("VideoParams", "HD Anamorphic 1080 (%1)")
  };

  // Format each
  for (int i=0; i<strings.size(); i++) {
    strings.replace(i, FormatPixelAspectRatioString(strings.at(i), kStandardPixelAspects.at(i)));
  }

  return strings;
}

QString VideoParams::FormatPixelAspectRatioString(const QString &format, const rational &ratio)
{
  return format.arg(QString::number(ratio.toDouble(), 'f', 4));
}

int VideoParams::GetScaledDimension(int dim, int divider)
{
  return dim / divider;
}

QByteArray VideoParams::toBytes() const
{
  QCryptographicHash hasher(QCryptographicHash::Sha1);

  hasher.addData(reinterpret_cast<const char*>(&width_), sizeof(width_));
  hasher.addData(reinterpret_cast<const char*>(&height_), sizeof(height_));
  hasher.addData(reinterpret_cast<const char*>(&depth_), sizeof(depth_));
  hasher.addData(reinterpret_cast<const char*>(&time_base_), sizeof(time_base_));
  hasher.addData(reinterpret_cast<const char*>(&format_), sizeof(format_));
  hasher.addData(reinterpret_cast<const char*>(&channel_count_), sizeof(channel_count_));
  hasher.addData(reinterpret_cast<const char*>(&pixel_aspect_ratio_), sizeof(pixel_aspect_ratio_));
  hasher.addData(reinterpret_cast<const char*>(&interlacing_), sizeof(interlacing_));
  hasher.addData(reinterpret_cast<const char*>(&divider_), sizeof(divider_));
  hasher.addData(reinterpret_cast<const char*>(&enabled_), sizeof(enabled_));
  hasher.addData(reinterpret_cast<const char*>(&stream_index_), sizeof(stream_index_));
  hasher.addData(reinterpret_cast<const char*>(&video_type_), sizeof(video_type_));
  hasher.addData(reinterpret_cast<const char*>(&frame_rate_), sizeof(frame_rate_));
  hasher.addData(reinterpret_cast<const char*>(&start_time_), sizeof(start_time_));
  hasher.addData(reinterpret_cast<const char*>(&duration_), sizeof(duration_));
  hasher.addData(reinterpret_cast<const char*>(&premultiplied_alpha_), sizeof(premultiplied_alpha_));
  hasher.addData(colorspace_.toUtf8());

  return hasher.result();
}

int64_t VideoParams::get_time_in_timebase_units(const rational &time) const
{
  if (time_base_.isNull()) {
    return AV_NOPTS_VALUE;
  }

  return Timecode::time_to_timestamp(time, time_base_) + start_time_;
}

void VideoParams::Load(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("width")) {
      set_width(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("height")) {
      set_height(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("depth")) {
      set_depth(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("timebase")) {
      set_time_base(rational::fromString(reader->readElementText()));
    } else if (reader->name() == QStringLiteral("format")) {
      set_format(static_cast<VideoParams::Format>(reader->readElementText().toInt()));
    } else if (reader->name() == QStringLiteral("channelcount")) {
      set_channel_count(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("pixelaspectratio")) {
      set_pixel_aspect_ratio(rational::fromString(reader->readElementText()));
    } else if (reader->name() == QStringLiteral("interlacing")) {
      set_interlacing(static_cast<VideoParams::Interlacing>(reader->readElementText().toInt()));
    } else if (reader->name() == QStringLiteral("divider")) {
      set_divider(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("enabled")) {
      set_enabled(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("streamindex")) {
      set_stream_index(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("videotype")) {
      set_video_type(static_cast<VideoParams::Type>(reader->readElementText().toInt()));
    } else if (reader->name() == QStringLiteral("framerate")) {
      set_frame_rate(rational::fromString(reader->readElementText()));
    } else if (reader->name() == QStringLiteral("starttime")) {
      set_start_time(reader->readElementText().toLongLong());
    } else if (reader->name() == QStringLiteral("duration")) {
      set_duration(reader->readElementText().toLongLong());
    } else if (reader->name() == QStringLiteral("premultipliedalpha")) {
      set_premultiplied_alpha(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("colorspace")) {
      set_colorspace(reader->readElementText());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void VideoParams::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("width"), QString::number(width_));
  writer->writeTextElement(QStringLiteral("height"), QString::number(height_));
  writer->writeTextElement(QStringLiteral("depth"), QString::number(depth_));
  writer->writeTextElement(QStringLiteral("timebase"), time_base_.toString());
  writer->writeTextElement(QStringLiteral("format"), QString::number(format_));
  writer->writeTextElement(QStringLiteral("channelcount"), QString::number(channel_count_));
  writer->writeTextElement(QStringLiteral("pixelaspectratio"), pixel_aspect_ratio_.toString());
  writer->writeTextElement(QStringLiteral("interlacing"), QString::number(interlacing_));
  writer->writeTextElement(QStringLiteral("divider"), QString::number(divider_));
  writer->writeTextElement(QStringLiteral("enabled"), QString::number(enabled_));
  writer->writeTextElement(QStringLiteral("streamindex"), QString::number(stream_index_));
  writer->writeTextElement(QStringLiteral("videotype"), QString::number(video_type_));
  writer->writeTextElement(QStringLiteral("framerate"), frame_rate_.toString());
  writer->writeTextElement(QStringLiteral("starttime"), QString::number(start_time_));
  writer->writeTextElement(QStringLiteral("duration"), QString::number(duration_));
  writer->writeTextElement(QStringLiteral("premultipliedalpha"), QString::number(premultiplied_alpha_));
  writer->writeTextElement(QStringLiteral("colorspace"), colorspace_);
}

}
