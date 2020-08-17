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

#include "videoparams.h"

#include <QtMath>

#include "core.h"

OLIVE_NAMESPACE_ENTER

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
  format_(PixelFormat::PIX_FMT_INVALID),
  interlacing_(Interlacing::kInterlaceNone)
{
}

VideoParams::VideoParams(const int &width, const int &height, const PixelFormat::Format &format, const rational& pixel_aspect_ratio, const Interlacing &interlacing, const int& divider) :
  width_(width),
  height_(height),
  format_(format),
  pixel_aspect_ratio_(pixel_aspect_ratio),
  interlacing_(interlacing),
  divider_(divider)
{
  calculate_effective_size();
  validate_pixel_aspect_ratio();
}

VideoParams::VideoParams(const int &width, const int &height, const rational &time_base, const PixelFormat::Format &format, const rational& pixel_aspect_ratio, const Interlacing &interlacing, const int &divider) :
  width_(width),
  height_(height),
  time_base_(time_base),
  format_(format),
  pixel_aspect_ratio_(pixel_aspect_ratio),
  interlacing_(interlacing),
  divider_(divider)
{
  calculate_effective_size();
  validate_pixel_aspect_ratio();
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

void VideoParams::calculate_effective_size()
{
  effective_width_ = GetScaledDimension(width(), divider_);
  effective_height_ = GetScaledDimension(height(), divider_);
}

void VideoParams::validate_pixel_aspect_ratio()
{
  if (pixel_aspect_ratio_.isNull()) {
    pixel_aspect_ratio_ = 1;
  }
}

bool VideoParams::is_valid() const
{
  return (width() > 0
          && height() > 0
          && !pixel_aspect_ratio_.isNull()
          && format_ != PixelFormat::PIX_FMT_INVALID
          && format_ != PixelFormat::PIX_FMT_COUNT);
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
  return qCeil(dim / divider * 0.5) * 2;
}

OLIVE_NAMESPACE_EXIT
