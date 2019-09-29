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

#include "stream.h"

/**
 * @brief A Stream derivative containing video-specific information
 */
class ImageStream : public Stream
{
  Q_OBJECT
public:
  ImageStream();

  virtual QString description() override;

  const int& width();
  void set_width(const int& width);

  const int& height();
  void set_height(const int& height);

  bool premultiplied_alpha();
  void set_premultiplied_alpha(bool e);

  const QString& colorspace();
  void set_colorspace(const QString& color);

signals:
  void ColorSpaceChanged();

private:
  int width_;
  int height_;
  bool premultiplied_alpha_;
  QString colorspace_;

private slots:
  void ColorConfigChangedSlot();
};

using ImageStreamPtr = std::shared_ptr<ImageStream>;

#endif // IMAGESTREAM_H
