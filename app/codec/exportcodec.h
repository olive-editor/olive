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

#ifndef EXPORTCODEC_H
#define EXPORTCODEC_H

#include <QObject>
#include <QString>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class ExportCodec : public QObject {
public:
  enum Codec {
    kCodecDNxHD,
    kCodecH264,
    kCodecH265,
    kCodecOpenEXR,
    kCodecPNG,
    kCodecProRes,
    kCodecTIFF,

    kCodecMP2,
    kCodecMP3,
    kCodecAAC,
    kCodecPCM,

    kCodecCount
  };

  static QString GetCodecName(Codec c);

  static bool IsCodecAStillImage(Codec c);

  static QStringList GetPixelFormatsForCodec(Codec c);

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTCODEC_H
