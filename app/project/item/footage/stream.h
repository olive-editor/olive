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

#ifndef STREAM_H
#define STREAM_H

#include <QVector>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/rational.h"
#include "render/audioparams.h"
#include "render/videoparams.h"

namespace olive {

class Stream {
public:
  enum Type {
    kUnknown = -1,
    kVideo,
    kAudio,
    kData,
    kSubtitle,
    kAttachment
  };

  enum VideoType {
    kVideoTypeVideo,
    kVideoTypeStill,
    kVideoTypeImageSequence
  };

  Stream(Type type = kUnknown) :
    type_(type)
  {
    Init();
  }

  bool IsValid() const
  {
    return type_ != kUnknown;
  }

  Type type() const
  {
    return type_;
  }

  const rational& timebase() const
  {
    return timebase_;
  }

  void set_timebase(const rational& timebase)
  {
    timebase_ = timebase;
  }

  int64_t duration() const
  {
    return duration_;
  }

  void set_duration(int64_t duration)
  {
    duration_ = duration;
  }

  int channel_count() const
  {
    return channel_count_;
  }

  void set_channel_count(int c)
  {
    channel_count_ = c;
  }

  bool enabled() const
  {
    return enabled_;
  }

  void set_enabled(bool e)
  {
    enabled_ = e;
  }

  int width() const
  {
    return width_;
  }

  void set_width(int w)
  {
    width_ = w;
  }

  int height() const
  {
    return height_;
  }

  void set_height(int h)
  {
    height_ = h;
  }

  const rational& pixel_aspect_ratio() const
  {
    return pixel_aspect_ratio_;
  }

  void set_pixel_aspect_ratio(const rational& pixel_aspect_ratio)
  {
    pixel_aspect_ratio_ = pixel_aspect_ratio;
  }

  VideoType video_type() const
  {
    return video_type_;
  }

  void set_video_type(VideoType t)
  {
    video_type_ = t;
  }

  VideoParams::Interlacing interlacing() const
  {
    return interlacing_;
  }

  void set_interlacing(VideoParams::Interlacing interlacing)
  {
    interlacing_ = interlacing;
  }

  VideoParams::Format pixel_format() const
  {
    return pixel_format_;
  }

  void set_pixel_format(VideoParams::Format pixel_format)
  {
    pixel_format_ = pixel_format;
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

  int sample_rate() const
  {
    return sample_rate_;
  }

  void set_sample_rate(int sample_rate)
  {
    sample_rate_ = sample_rate;
  }

  uint64_t channel_layout() const
  {
    return channel_layout_;
  }

  void set_channel_layout(uint64_t channel_layout)
  {
    channel_layout_ = channel_layout;
  }

  VideoParams video_params() const
  {
    if (type_ == kVideo) {
      return VideoParams(width_, height_, timebase_,
                         pixel_format_, channel_count_, pixel_aspect_ratio_,
                         interlacing_);
    } else {
      return VideoParams();
    }
  }

  AudioParams audio_params() const
  {
    if (type_ == kAudio) {
      return AudioParams(sample_rate_, channel_layout_, AudioParams::kInternalFormat);
    } else {
      return AudioParams();
    }
  }

  void Load(QXmlStreamReader* reader);

  void Save(QXmlStreamWriter* writer) const;

  QByteArray toBytes() const
  {
    QByteArray arr;

    arr.append(reinterpret_cast<const char*>(&type_), sizeof(type_));
    arr.append(reinterpret_cast<const char*>(&timebase_), sizeof(timebase_));
    arr.append(reinterpret_cast<const char*>(&duration_), sizeof(duration_));
    arr.append(reinterpret_cast<const char*>(&channel_count_), sizeof(channel_count_));
    arr.append(reinterpret_cast<const char*>(&enabled_), sizeof(enabled_));
    arr.append(reinterpret_cast<const char*>(&width_), sizeof(width_));
    arr.append(reinterpret_cast<const char*>(&height_), sizeof(height_));
    arr.append(reinterpret_cast<const char*>(&pixel_aspect_ratio_), sizeof(pixel_aspect_ratio_));
    arr.append(reinterpret_cast<const char*>(&video_type_), sizeof(video_type_));
    arr.append(reinterpret_cast<const char*>(&interlacing_), sizeof(interlacing_));
    arr.append(reinterpret_cast<const char*>(&pixel_format_), sizeof(pixel_format_));
    arr.append(reinterpret_cast<const char*>(&frame_rate_), sizeof(frame_rate_));
    arr.append(reinterpret_cast<const char*>(&start_time_), sizeof(start_time_));
    arr.append(reinterpret_cast<const char*>(&premultiplied_alpha_), sizeof(premultiplied_alpha_));
    arr.append(colorspace_.toUtf8());
    arr.append(reinterpret_cast<const char*>(&sample_rate_), sizeof(sample_rate_));
    arr.append(reinterpret_cast<const char*>(&channel_layout_), sizeof(channel_layout_));

    return arr;
  }

private:
  void Init()
  {
    duration_ = AV_NOPTS_VALUE;
    channel_count_ = 0;
    enabled_ = true;
    width_ = 0;
    height_ = 0;
    video_type_ = VideoType::kVideoTypeVideo;
    interlacing_ = VideoParams::kInterlaceNone;
    pixel_format_ = VideoParams::kFormatInvalid;
    start_time_ = 0;
    premultiplied_alpha_ = false;
    sample_rate_ = 0;
    channel_layout_ = 0;
  }

  // Global members
  Type type_;
  rational timebase_;
  int64_t duration_;
  int channel_count_;
  bool enabled_;

  // Video members
  int width_;
  int height_;
  rational pixel_aspect_ratio_;
  VideoType video_type_;
  VideoParams::Interlacing interlacing_;
  VideoParams::Format pixel_format_;
  rational frame_rate_;
  int64_t start_time_;
  bool premultiplied_alpha_;
  QString colorspace_;

  // Audio members
  int sample_rate_;
  uint64_t channel_layout_;

};

using Streams = QVector<Stream>;

}

Q_DECLARE_METATYPE(olive::Stream)

#endif // STREAM_H
