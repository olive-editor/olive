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

#include "otiodecoder.h"

#include <opentimelineio/timeline.h>

OLIVE_NAMESPACE_ENTER

OTIODecoder::OTIODecoder()
{

}

QString OTIODecoder::id()
{
  return QStringLiteral("otio");
}

ItemPtr OTIODecoder::Probe(const QString& filename, const QAtomicInt* cancelled) const
{
  if (filename.endsWith(QStringLiteral(".otio"), Qt::CaseInsensitive)) {
    opentimelineio::v1_0::ErrorStatus es;

    auto timeline = static_cast<opentimelineio::v1_0::Timeline*>(opentimelineio::v1_0::SerializableObjectWithMetadata::from_json_file(filename.toStdString(), &es));

    if (es != opentimelineio::v1_0::ErrorStatus::OK) {
      return nullptr;
    }

    qDebug() << "Found" << timeline->video_tracks().size() << "video tracks";
  }

  return nullptr;
}

OLIVE_NAMESPACE_EXIT
