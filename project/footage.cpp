/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

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

#include "footage.h"

#include <QDebug>
#include <QtMath>
#include <QPainter>
#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE::v1;

#include "project/previewgenerator.h"
#include "timeline/clip.h"
#include "global/config.h"
#include "global/global.h"

Footage::Footage() :
  ready(false),
  preview_gen(nullptr),
  invalid(false),
  in(0),
  out(0),
  speed(1.0),
  alpha_is_associated(true),
  proxy(false),
  start_number(0)
{
  ready_lock.lock();
}

Footage::~Footage() {
  reset();
}

void Footage::Save(QXmlStreamWriter &stream)
{
  QDir proj_dir = QFileInfo(olive::ActiveProjectFilename).absoluteDir();

  stream.writeStartElement("footage");
  stream.writeAttribute("id", QString::number(media_id));
  stream.writeAttribute("name", name);
  stream.writeAttribute("url", proj_dir.relativeFilePath(url));
  stream.writeAttribute("duration", QString::number(length));
  stream.writeAttribute("using_inout", QString::number(using_inout));
  stream.writeAttribute("in", QString::number(in));
  stream.writeAttribute("out", QString::number(out));
  stream.writeAttribute("speed", QString::number(speed));
  stream.writeAttribute("alphapremul", QString::number(alpha_is_associated));
  stream.writeAttribute("startnumber", QString::number(start_number));
  stream.writeAttribute("colorspace", Colorspace());

  stream.writeAttribute("proxy", QString::number(proxy));
  stream.writeAttribute("proxypath", proxy_path);

  // save video stream metadata
  for (int j=0;j<f->video_tracks.size();j++) {
    const FootageStream& ms = f->video_tracks.at(j);
    stream.writeStartElement("video");
    stream.writeAttribute("id", QString::number(ms.file_index));
    stream.writeAttribute("width", QString::number(ms.video_width));
    stream.writeAttribute("height", QString::number(ms.video_height));
    stream.writeAttribute("framerate", QString::number(ms.video_frame_rate, 'f', 10));
    stream.writeAttribute("infinite", QString::number(ms.infinite_length));
    stream.writeEndElement(); // video
  }

  // save audio stream metadata
  for (int j=0;j<f->audio_tracks.size();j++) {
    const FootageStream& ms = f->audio_tracks.at(j);
    stream.writeStartElement("audio");
    stream.writeAttribute("id", QString::number(ms.file_index));
    stream.writeAttribute("channels", QString::number(ms.audio_channels));
    stream.writeAttribute("layout", QString::number(ms.audio_layout));
    stream.writeAttribute("frequency", QString::number(ms.audio_frequency));
    stream.writeEndElement(); // audio
  }

  // save footage markers
  for (int j=0;j<f->markers.size();j++) {
    f->markers.at(j).Save(stream);
  }

  stream.writeEndElement(); // footage
  media_id++;
}

QString Footage::Colorspace()
{
  if (!colorspace_.isEmpty()) {
    return colorspace_;
  }

  // If this footage has no color space set, try to guess the color space from the filename
  OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
  QString guess_colorspace = config->parseColorSpaceFromString(url.toUtf8());

  if (!guess_colorspace.isEmpty()) {
    return guess_colorspace;
  }

  return olive::CurrentConfig.ocio_default_input_colorspace;
}

void Footage::SetColorspace(const QString &cs)
{
  colorspace_ = cs;
}

void Footage::reset() {
  if (preview_gen != nullptr) {
    preview_gen->cancel();
  }
  video_tracks.clear();
  audio_tracks.clear();
  ready = false;
}

long Footage::get_length_in_frames(double frame_rate) {
  if (length >= 0) {
    return qFloor((double(length) / double(AV_TIME_BASE)) * frame_rate / speed);
  }
  return 0;
}

FootageStream* Footage::get_stream_from_file_index(bool video, int index) {
  if (video) {
    for (int i=0;i<video_tracks.size();i++) {
      if (video_tracks.at(i).file_index == index) {
        return &video_tracks[i];
      }
    }
  } else {
    for (int i=0;i<audio_tracks.size();i++) {
      if (audio_tracks.at(i).file_index == index) {
        return &audio_tracks[i];
      }
    }
  }
  return nullptr;
}
