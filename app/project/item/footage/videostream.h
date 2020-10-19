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

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include "render/pixelformat.h"
#include "render/videoparams.h"
#include "stream.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A Stream derivative containing video-specific information
 */
class VideoStream : public Stream
{
  Q_OBJECT
public:
  VideoStream();

  enum VideoType {
    kVideoTypeVideo,
    kVideoTypeStill,
    kVideoTypeImageSequence
  };

  virtual QString description() const override;

  VideoType video_type() const
  {
    return video_type_;
  }

  void set_video_type(VideoType t)
  {
    video_type_ = t;
  }

  const int& width() const
  {
    return width_;
  }

  void set_width(const int& width)
  {
    width_ = width;
  }

  const int& height() const
  {
    return height_;
  }

  void set_height(const int& height)
  {
    height_ = height;
  }

  const PixelFormat::Format& format() const
  {
    return format_;
  }

  void set_format(const PixelFormat::Format& format)
  {
    format_ = format;
  }

  bool premultiplied_alpha() const;
  void set_premultiplied_alpha(bool e);

  const QString& colorspace(bool default_if_empty = true) const;
  void set_colorspace(const QString& color);

  QString get_colorspace_match_string() const;

  VideoParams::Interlacing interlacing() const
  {
    return interlacing_;
  }

  void set_interlacing(VideoParams::Interlacing i)
  {
    interlacing_ = i;

    emit ParametersChanged();
  }

  const rational& pixel_aspect_ratio() const
  {
    return pixel_aspect_ratio_;
  }

  void set_pixel_aspect_ratio(const rational& r)
  {
    // Auto-correct null aspect ratio to 1:1
    if (r.isNull()) {
      pixel_aspect_ratio_ = 1;
    } else {
      pixel_aspect_ratio_ = r;
    }

    emit ParametersChanged();
  }

  /**
   * @brief Get this video stream's frame rate
   *
   * Used purely for metadata, rendering uses the timebase instead.
   */
  const rational& frame_rate() const;
  void set_frame_rate(const rational& frame_rate);

  const int64_t& start_time() const;
  void set_start_time(const int64_t& start_time);

  int64_t get_time_in_timebase_units(const rational& time) const;

  virtual QIcon icon() const override;

public slots:
  void ColorConfigChanged();

  void DefaultColorSpaceChanged();

protected:
  virtual void LoadCustomParameters(QXmlStreamReader *reader) override;

  virtual void SaveCustomParameters(QXmlStreamWriter* writer) const override;

private:
  int width_;
  int height_;
  bool premultiplied_alpha_;
  QString colorspace_;
  VideoParams::Interlacing interlacing_;

  VideoType video_type_;

  PixelFormat::Format format_;

  rational pixel_aspect_ratio_;

  rational frame_rate_;

  int64_t start_time_;

};

using VideoStreamPtr = std::shared_ptr<VideoStream>;

OLIVE_NAMESPACE_EXIT

#endif // VIDEOSTREAM_H
