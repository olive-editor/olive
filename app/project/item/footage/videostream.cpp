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

#include "videostream.h"

#include <QFile>

#include "common/timecodefunctions.h"
#include "common/xmlutils.h"
#include "footage.h"
#include "project/project.h"
#include "render/colormanager.h"

namespace olive {

VideoStream::VideoStream() :
  premultiplied_alpha_(false),
  interlacing_(VideoParams::kInterlaceNone),
  video_type_(VideoStream::kVideoTypeVideo),
  pixel_aspect_ratio_(1),
  start_time_(0)
{
  set_type(Stream::kVideo);
}

QString VideoStream::description() const
{
  if (video_type_ == VideoStream::kVideoTypeStill) {
    return QCoreApplication::translate("Stream", "%1: Image - %2x%3").arg(QString::number(index()),
                                                                          QString::number(width()),
                                                                          QString::number(height()));
  } else {
    return QCoreApplication::translate("Stream", "%1: Video - %2x%3").arg(QString::number(index()),
                                                                          QString::number(width()),
                                                                          QString::number(height()));
  }
}

const rational &VideoStream::frame_rate() const
{
  return frame_rate_;
}

void VideoStream::set_frame_rate(const rational &frame_rate)
{
  frame_rate_ = frame_rate;
}

const int64_t &VideoStream::start_time() const
{
  return start_time_;
}

void VideoStream::set_start_time(const int64_t &start_time)
{
  start_time_ = start_time;
  emit ParametersChanged();
}

int64_t VideoStream::get_time_in_timebase_units(const rational &time) const
{
  return Timecode::time_to_timestamp(time, timebase()) + start_time();
}

QIcon VideoStream::icon() const
{
  if (video_type_ == kVideoTypeStill) {
    return icon::Image;
  } else {
    return icon::Video;
  }
}

void VideoStream::LoadCustomParameters(QXmlStreamReader *reader)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("width")) {
      set_width(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("height")) {
      set_height(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("premultiplied")) {
      set_premultiplied_alpha(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("colorspace")) {
      set_colorspace(reader->readElementText());
    } else if (reader->name() == QStringLiteral("interlacing")) {
      set_interlacing(static_cast<VideoParams::Interlacing>(reader->readElementText().toInt()));
    } else if (reader->name() == QStringLiteral("type")) {
      set_video_type(static_cast<VideoType>(reader->readElementText().toInt()));
    } else if (reader->name() == QStringLiteral("format")) {
      set_format(static_cast<VideoParams::Format>(reader->readElementText().toInt()));
    } else if (reader->name() == QStringLiteral("channels")) {
      set_channel_count(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("pixelaspect")) {
      set_pixel_aspect_ratio(rational::fromString(reader->readElementText()));
    } else if (reader->name() == QStringLiteral("framerate")) {
      set_frame_rate(rational::fromString(reader->readElementText()));
    } else if (reader->name() == QStringLiteral("starttime")) {
      set_start_time(reader->readElementText().toLongLong());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void VideoStream::SaveCustomParameters(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("width"), QString::number(width_));
  writer->writeTextElement(QStringLiteral("height"), QString::number(height_));
  writer->writeTextElement(QStringLiteral("premultiplied"), QString::number(premultiplied_alpha_));
  writer->writeTextElement(QStringLiteral("colorspace"), colorspace_);
  writer->writeTextElement(QStringLiteral("interlacing"), QString::number(interlacing_));
  writer->writeTextElement(QStringLiteral("type"), QString::number(video_type_));
  writer->writeTextElement(QStringLiteral("format"), QString::number(format_));
  writer->writeTextElement(QStringLiteral("channels"), QString::number(channel_count_));
  writer->writeTextElement(QStringLiteral("pixelaspect"), pixel_aspect_ratio_.toString());
  writer->writeTextElement(QStringLiteral("framerate"), frame_rate_.toString());
  writer->writeTextElement(QStringLiteral("starttime"), QString::number(start_time_));
}

bool VideoStream::premultiplied_alpha() const
{
  return premultiplied_alpha_;
}

void VideoStream::set_premultiplied_alpha(bool e)
{
  premultiplied_alpha_ = e;

  emit ParametersChanged();
}

const QString &VideoStream::colorspace(bool default_if_empty) const
{
  if (colorspace_.isEmpty() && default_if_empty) {
    return footage()->project()->color_manager()->GetDefaultInputColorSpace();
  } else {
    return colorspace_;
  }
}

void VideoStream::set_colorspace(const QString &color)
{
  colorspace_ = color;

  emit ParametersChanged();
}

void VideoStream::ColorConfigChanged()
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

void VideoStream::DefaultColorSpaceChanged()
{
  // If no colorspace is set, this stream uses the default color space and it's just changed
  if (colorspace_.isEmpty()) {
    emit ParametersChanged();
  }
}

}
