/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/rational.h"
#include "rendermodes.h"

namespace olive {

class VideoParams {
public:
  enum Format {
    /// Invalid or no format
    kFormatInvalid = -1,

    /// 8-bit unsigned integer
    kFormatUnsigned8,

    /// 16-bit unsigned integer
    kFormatUnsigned16,

    /// 16-bit half float
    kFormatFloat16,

    /// 32-bit full float
    kFormatFloat32,

    /// 64-bit double float - disabled since very, very few libs support 64-bit buffers
    //kFormatFloat64,

    /// Total format count
    kFormatCount
  };

  enum Interlacing {
    kInterlaceNone,
    kInterlacedTopFirst,
    kInterlacedBottomFirst
  };

  enum Type {
    kVideoTypeVideo,
    kVideoTypeStill,
    kVideoTypeImageSequence
  };

  VideoParams();
  VideoParams(int width, int height, Format format, int nb_channels,
              const rational& pixel_aspect_ratio = 1,
              Interlacing interlacing = kInterlaceNone, int divider = 1);
  VideoParams(int width, int height, int depth,
              Format format, int nb_channels,
              const rational& pixel_aspect_ratio = 1,
              Interlacing interlacing = kInterlaceNone, int divider = 1);
  VideoParams(int width, int height, const rational& time_base,
              Format format, int nb_channels,
              const rational& pixel_aspect_ratio = 1,
              Interlacing interlacing = kInterlaceNone, int divider = 1);

  int width() const
  {
    return width_;
  }

  void set_width(int width)
  {
    width_ = width;
    calculate_effective_size();
  }

  /**
   * @brief Returns width multiplied by pixel aspect ratio where applicable
   */
  int square_pixel_width() const
  {
    return par_width_;
  }

  int height() const
  {
    return height_;
  }

  void set_height(int height)
  {
    height_ = height;
    calculate_effective_size();
  }

  int depth() const
  {
    return depth_;
  }

  void set_depth(int depth)
  {
    depth_ = depth;
    calculate_effective_size();
  }

  const rational& time_base() const
  {
    return time_base_;
  }

  void set_time_base(const rational& r)
  {
    time_base_ = r;
  }

  rational frame_rate_as_time_base() const
  {
    return frame_rate_.flipped();
  }

  int divider() const
  {
    return divider_;
  }

  void set_divider(int d)
  {
    divider_ = d;
    calculate_effective_size();
  }

  int effective_width() const
  {
    return effective_width_;
  }

  int effective_height() const
  {
    return effective_height_;
  }

  int effective_depth() const
  {
    return effective_depth_;
  }

  Format format() const
  {
    return format_;
  }

  void set_format(Format f)
  {
    format_ = f;
  }

  int channel_count() const
  {
    return channel_count_;
  }

  void set_channel_count(int c)
  {
    channel_count_ = c;
  }

  const rational& pixel_aspect_ratio() const
  {
    return pixel_aspect_ratio_;
  }

  void set_pixel_aspect_ratio(const rational& r)
  {
    pixel_aspect_ratio_ = r;
    validate_pixel_aspect_ratio();
  }

  Interlacing interlacing() const
  {
    return interlacing_;
  }

  void set_interlacing(Interlacing i)
  {
    interlacing_ = i;
  }

  static int generate_auto_divider(qint64 width, qint64 height);

  bool is_valid() const;

  bool operator==(const VideoParams& rhs) const;
  bool operator!=(const VideoParams& rhs) const;

  static int GetBytesPerChannel(Format format);
  int GetBytesPerChannel() const
  {
    return GetBytesPerChannel(format_);
  }

  static int GetBytesPerPixel(Format format, int channels);
  int GetBytesPerPixel() const
  {
    return GetBytesPerPixel(format_, channel_count_);
  }

  static int GetBufferSize(int width, int height, Format format, int channels)
  {
    return width * height * GetBytesPerPixel(format, channels);
  }
  int GetBufferSize() const
  {
    return GetBufferSize(width_, height_, format_, channel_count_);
  }

  static bool FormatIsFloat(Format format);

  static QString GetFormatName(Format format);

  static const int kInternalChannelCount;

  static const rational kPixelAspectSquare;
  static const rational kPixelAspectNTSCStandard;
  static const rational kPixelAspectNTSCWidescreen;
  static const rational kPixelAspectPALStandard;
  static const rational kPixelAspectPALWidescreen;
  static const rational kPixelAspect1080Anamorphic;

  static const QVector<rational> kSupportedFrameRates;
  static const QVector<rational> kStandardPixelAspects;
  static const QVector<int> kSupportedDividers;

  static const int kHSVChannelCount = 3;
  static const int kRGBChannelCount = 3;
  static const int kRGBAChannelCount = 4;

  /**
   * @brief Convert rational frame rate (i.e. flipped timebase) to a user-friendly string
   */
  static QString FrameRateToString(const rational& frame_rate);

  static QStringList GetStandardPixelAspectRatioNames();
  static QString FormatPixelAspectRatioString(const QString& format, const rational& ratio);

  static int GetScaledDimension(int dim, int divider);

  QByteArray toBytes() const;

  bool enabled() const
  {
    return enabled_;
  }

  void set_enabled(bool e)
  {
    enabled_ = e;
  }

  int stream_index() const
  {
    return stream_index_;
  }

  void set_stream_index(int s)
  {
    stream_index_ = s;
  }

  Type video_type() const
  {
    return video_type_;
  }

  void set_video_type(Type t)
  {
    video_type_ = t;
  }

  const rational& frame_rate() const
  {
    return frame_rate_;
  }

  void set_frame_rate(const rational& frame_rate)
  {
    frame_rate_ = frame_rate;
  }

  int64_t start_time() const
  {
    return start_time_;
  }

  void set_start_time(int64_t start_time)
  {
    start_time_ = start_time;
  }

  int64_t duration() const
  {
    return duration_;
  }

  void set_duration(int64_t duration)
  {
    duration_ = duration;
  }

  bool premultiplied_alpha() const
  {
    return premultiplied_alpha_;
  }

  void set_premultiplied_alpha(bool premultiplied_alpha)
  {
    premultiplied_alpha_ = premultiplied_alpha;
  }

  const QString& colorspace() const
  {
    return colorspace_;
  }

  void set_colorspace(const QString& c)
  {
    colorspace_ = c;
  }

  int64_t get_time_in_timebase_units(const rational& time) const;

  void Load(QXmlStreamReader* reader);

  void Save(QXmlStreamWriter* writer) const;

private:
  void calculate_effective_size();

  void validate_pixel_aspect_ratio();

  void set_defaults_for_footage();

  void calculate_square_pixel_width();

  int width_;
  int height_;
  int depth_;
  rational time_base_;

  Format format_;

  int channel_count_;

  rational pixel_aspect_ratio_;

  Interlacing interlacing_;

  int divider_;

  // Cached values
  int effective_width_;
  int effective_height_;
  int effective_depth_;
  int par_width_;

  bool enabled_;
  int stream_index_;
  Type video_type_;
  rational frame_rate_;
  int64_t start_time_;
  int64_t duration_;
  bool premultiplied_alpha_;
  QString colorspace_;

};

}

Q_DECLARE_METATYPE(olive::VideoParams)
Q_DECLARE_METATYPE(olive::VideoParams::Interlacing)

#endif // VIDEOPARAMS_H
