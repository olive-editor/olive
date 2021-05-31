/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef EXPORTFORMAT_H
#define EXPORTFORMAT_H

#include <QList>
#include <QString>

#include "common/define.h"
#include "exportcodec.h"

namespace olive {

class ExportFormat : public QObject
{
  Q_OBJECT
public:
  enum Format {
    kFormatDNxHD,
    kFormatMatroska,
    kFormatMPEG4,
    kFormatOpenEXR,
    kFormatQuickTime,
    kFormatPNG,
    kFormatTIFF,
    kFormatWAV,
    kFormatAIFF,
    kFormatMP3,
    kFormatFLAC,
    kFormatOgg,
    kFormatWebM,
    kFormatSRT,

    kFormatCount
  };

  static QString GetName(Format f);
  static QString GetExtension(Format f);
  static QList<ExportCodec::Codec> GetVideoCodecs(ExportFormat::Format f);
  static QList<ExportCodec::Codec> GetAudioCodecs(ExportFormat::Format f);
  static QList<ExportCodec::Codec> GetSubtitleCodecs(ExportFormat::Format f);

  static QStringList GetPixelFormatsForCodec(Format f, ExportCodec::Codec c);

};

}

#endif // EXPORTFORMAT_H
