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

#include "common/xmlutils.h"
#include "footage.h"
#include "project/project.h"
#include "render/colormanager.h"

OLIVE_NAMESPACE_ENTER

ImageStream::ImageStream() :
  premultiplied_alpha_(false)
{
  set_type(kImage);
}

void ImageStream::FootageSetEvent(Footage *f)
{
  // For some reason this connection fails if we don't explicitly specify DirectConnection
  connect(f->project()->color_manager(),
          &ColorManager::ConfigChanged,
          this,
          &ImageStream::ColorConfigChanged,
          Qt::DirectConnection);

  connect(f->project()->color_manager(),
          &ColorManager::DefaultInputColorSpaceChanged,
          this,
          &ImageStream::DefaultColorSpaceChanged,
          Qt::DirectConnection);
}

void ImageStream::LoadCustomParameters(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("colorspace")) {
      set_colorspace(reader->readElementText());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void ImageStream::SaveCustomParameters(QXmlStreamWriter *writer) const
{
  writer->writeTextElement("colorspace", colorspace_);
}

QString ImageStream::description() const
{
  return QCoreApplication::translate("Stream", "%1: Image - %2x%3").arg(QString::number(index()),
                                                                        QString::number(width()),
                                                                        QString::number(height()));
}

bool ImageStream::premultiplied_alpha() const
{
  return premultiplied_alpha_;
}

void ImageStream::set_premultiplied_alpha(bool e)
{
  premultiplied_alpha_ = e;

  emit ParametersChanged();
}

const QString &ImageStream::colorspace(bool default_if_empty) const
{
  if (colorspace_.isEmpty() && default_if_empty) {
    return footage()->project()->color_manager()->GetDefaultInputColorSpace();
  } else {
    return colorspace_;
  }
}

void ImageStream::set_colorspace(const QString &color)
{
  colorspace_ = color;

  emit ParametersChanged();
}

QString ImageStream::get_colorspace_match_string() const
{
  return QStringLiteral("%1:%2").arg(footage()->project()->color_manager()->GetConfigFilename(),
                                     colorspace());
}

void ImageStream::ColorConfigChanged()
{
  ColorManager* color_manager = footage()->project()->color_manager();

  // Check if this colorspace is in the new config
  if (!colorspace_.isEmpty()) {
    QStringList colorspaces = color_manager->ListAvailableColorspaces();
    if (!colorspaces.contains(colorspace_)) {
      // Set to empty if not
      colorspace_.clear();
    }
  }

  // Either way, the color calculation has likely changed so we signal here
  emit ParametersChanged();
}

void ImageStream::DefaultColorSpaceChanged()
{
  // If no colorspace is set, this stream uses the default color space and it's just changed
  if (colorspace_.isEmpty()) {
    emit ParametersChanged();
  }
}

OLIVE_NAMESPACE_EXIT
