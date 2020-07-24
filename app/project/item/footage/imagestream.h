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

#ifndef IMAGESTREAM_H
#define IMAGESTREAM_H

#include "render/pixelformat.h"
#include "stream.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A Stream derivative containing video-specific information
 */
class ImageStream : public Stream
{
  Q_OBJECT
public:
  ImageStream();

  virtual QString description() const override;

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

  enum Interlacing {
    kInterlaceNone,
    kInterlacedTopFirst,
    kInterlacedBottomFirst
  };

  Interlacing interlacing() const
  {
    return interlacing_;
  }

  void set_interlacing(Interlacing i)
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

protected:
  virtual void FootageSetEvent(Footage*) override;

  virtual void LoadCustomParameters(QXmlStreamReader *reader) override;

  virtual void SaveCustomParameters(QXmlStreamWriter* writer) const override;

private:
  int width_;
  int height_;
  bool premultiplied_alpha_;
  QString colorspace_;
  Interlacing interlacing_;

  PixelFormat::Format format_;

  rational pixel_aspect_ratio_;

private slots:
  void ColorConfigChanged();

  void DefaultColorSpaceChanged();

};

using ImageStreamPtr = std::shared_ptr<ImageStream>;

OLIVE_NAMESPACE_EXIT

#endif // IMAGESTREAM_H
