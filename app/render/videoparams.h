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

#ifndef VIDEOPARAMS_H
#define VIDEOPARAMS_H

#include "common/rational.h"
#include "pixelformat.h"
#include "rendermodes.h"

OLIVE_NAMESPACE_ENTER

class VideoParams {
public:
  enum Interlacing {
    kInterlaceNone,
    kInterlacedTopFirst,
    kInterlacedBottomFirst
  };

  VideoParams();
  VideoParams(const int& width, const int& height, const PixelFormat::Format& format,
              const rational& pixel_aspect_ratio = 1,
              const Interlacing& interlacing = kInterlaceNone, const int& divider = 1);
  VideoParams(const int& width, const int& height, const rational& time_base,
              const PixelFormat::Format& format, const rational& pixel_aspect_ratio = 1,
              const Interlacing& interlacing = kInterlaceNone, const int& divider = 1);

  const int& width() const
  {
    return width_;
  }

  const int& height() const
  {
    return height_;
  }

  const rational& time_base() const
  {
    return time_base_;
  }

  const int& divider() const
  {
    return divider_;
  }

  const int& effective_width() const
  {
    return effective_width_;
  }

  const int& effective_height() const
  {
    return effective_height_;
  }

  const PixelFormat::Format& format() const
  {
    return format_;
  }

  const rational& pixel_aspect_ratio() const
  {
    return pixel_aspect_ratio_;
  }

  Interlacing interlacing() const
  {
    return interlacing_;
  }

  static int generate_auto_divider(qint64 width, qint64 height);

  bool is_valid() const;

  bool operator==(const VideoParams& rhs) const;
  bool operator!=(const VideoParams& rhs) const;

  static const rational kPixelAspectSquare;
  static const rational kPixelAspectNTSCStandard;
  static const rational kPixelAspectNTSCWidescreen;
  static const rational kPixelAspectPALStandard;
  static const rational kPixelAspectPALWidescreen;
  static const rational kPixelAspect1080Anamorphic;

  static const QVector<rational> kSupportedFrameRates;
  static const QVector<rational> kStandardPixelAspects;
  static const QVector<int> kSupportedDividers;

  /**
   * @brief Convert rational frame rate (i.e. flipped timebase) to a user-friendly string
   */
  static QString FrameRateToString(const rational& frame_rate);

  static QStringList GetStandardPixelAspectRatioNames();
  static QString FormatPixelAspectRatioString(const QString& format, const rational& ratio);

private:
  void calculate_effective_size();

  void validate_pixel_aspect_ratio();

  int width_;
  int height_;
  rational time_base_;

  PixelFormat::Format format_;

  rational pixel_aspect_ratio_;

  Interlacing interlacing_;

  int divider_;
  int effective_width_;
  int effective_height_;
};

OLIVE_NAMESPACE_EXIT

Q_DECLARE_METATYPE(OLIVE_NAMESPACE::VideoParams)
Q_DECLARE_METATYPE(OLIVE_NAMESPACE::VideoParams::Interlacing)

#endif // VIDEOPARAMS_H
