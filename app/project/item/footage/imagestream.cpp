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

#include "footage.h"
#include "project/project.h"
#include "render/colormanager.h"

ImageStream::ImageStream() :
  premultiplied_alpha_(false)
{
  set_type(kImage);

  // For some reason this connection fails if we don't explicitly specify DirectConnection
  connect(ColorManager::instance(), SIGNAL(ConfigChanged()), this, SLOT(ColorConfigChangedSlot()), Qt::DirectConnection);
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

bool ImageStream::premultiplied_alpha()
{
  return premultiplied_alpha_;
}

void ImageStream::set_premultiplied_alpha(bool e)
{
  premultiplied_alpha_ = e;
}

const QString &ImageStream::colorspace()
{
  if (colorspace_.isEmpty()) {
    return footage()->project()->default_input_colorspace();
  } else {
    return colorspace_;
  }
}

void ImageStream::set_colorspace(const QString &color)
{
  colorspace_ = color;

  emit ColorSpaceChanged();
}

void ImageStream::ColorConfigChangedSlot()
{
  // Check if this colorspace is in the new config
  if (!colorspace_.isEmpty()) {
    QStringList colorspaces = ColorManager::ListAvailableInputColorspaces(OCIO::GetCurrentConfig());
    if (!colorspaces.contains(colorspace_)) {
      // Set to empty if not
      colorspace_.clear();
    }
  }

  // Either way, the color calculation has likely changed so we signal here
  emit ColorSpaceChanged();
}
