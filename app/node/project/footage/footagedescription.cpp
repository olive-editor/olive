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

#include "footagedescription.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/xmlutils.h"

namespace olive {

bool FootageDescription::Load(const QString &filename)
{
  // Reset self
  *this = FootageDescription();

  QFile file(filename);

  if (file.open(QFile::ReadOnly)) {
    QXmlStreamReader reader(&file);

    while (XMLReadNextStartElement(&reader)) {
      if (reader.name() == QStringLiteral("streamcache")) {
        while (XMLReadNextStartElement(&reader)) {
          if (reader.name() == QStringLiteral("decoder")) {
            decoder_ = reader.readElementText();
          } else if (reader.name() == QStringLiteral("streams")) {
            while (XMLReadNextStartElement(&reader)) {
              if (reader.name() == QStringLiteral("video")) {
                VideoParams vp;
                vp.Load(&reader);
                AddVideoStream(vp);
              } else if (reader.name() == QStringLiteral("audio")) {
                AudioParams ap;
                ap.Load(&reader);
                AddAudioStream(ap);
              } else {
                reader.skipCurrentElement();
              }
            }
          } else {
            reader.skipCurrentElement();
          }
        }
      } else {
        reader.skipCurrentElement();
      }
    }

    file.close();

    return true;
  }

  return false;
}

bool FootageDescription::Save(const QString &filename) const
{
  QFile file(filename);

  if (!file.open(QFile::WriteOnly)) {
    return false;
  }

  QXmlStreamWriter writer(&file);

  writer.writeStartDocument();

  writer.writeStartElement(QStringLiteral("streamcache"));

  writer.writeTextElement(QStringLiteral("decoder"), decoder_);

  writer.writeStartElement(QStringLiteral("streams"));

  foreach (const VideoParams& vp, video_streams_) {
    writer.writeStartElement(QStringLiteral("video"));
    vp.Save(&writer);
    writer.writeEndElement(); // video
  }

  foreach (const AudioParams& ap, audio_streams_) {
    writer.writeStartElement(QStringLiteral("audio"));
    ap.Save(&writer);
    writer.writeEndElement(); // audio
  }

  writer.writeEndElement(); // streams

  writer.writeEndElement(); // streamcache

  writer.writeEndDocument();

  file.close();

  return true;
}

}
