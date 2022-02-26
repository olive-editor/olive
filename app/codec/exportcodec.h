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

#ifndef EXPORTCODEC_H
#define EXPORTCODEC_H

#include <QObject>
#include <QString>

#include "common/define.h"
#include "render/subtitleparams.h"

namespace olive {

class ExportCodec : public QObject
{
  Q_OBJECT
public:
  enum Codec {
    // Video codecs
    kCodecDNxHD,
    kCodecH264,
    kCodecH264rgb,
    kCodecH265,
    kCodecOpenEXR,
    kCodecPNG,
    kCodecProRes,
    kCodecCineform,
    kCodecTIFF,
    kCodecVP9,

    // Audio codecs
    kCodecMP2,
    kCodecMP3,
    kCodecAAC,
    kCodecPCM,
    kCodecOpus,
    kCodecVorbis,
    kCodecFLAC,

    // Subtitle codecs
    kCodecSRT,

    kCodecCount
  };

  static QString GetCodecName(Codec c);

  static bool IsCodecAStillImage(Codec c);

};

}

#endif // EXPORTCODEC_H
