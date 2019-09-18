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

#include "imagestream.h"

ImageStream::ImageStream()
{
  set_type(kImage);
}

QString ImageStream::description()
{
  return QCoreApplication::translate("Stream", "%1: Image - %2x%3").arg(QString::number(index()),
                                                                        QString::number(width()),
                                                                        QString::number(height()));
}

const int &ImageStream::width()
{
  return width_;
}

void ImageStream::set_width(const int &width)
{
  width_ = width;
}

const int &ImageStream::height()
{
  return height_;
}

void ImageStream::set_height(const int &height)
{
  height_ = height;
}
