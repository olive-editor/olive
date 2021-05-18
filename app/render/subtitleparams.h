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

#ifndef SUBTITLEPARAMS_H
#define SUBTITLEPARAMS_H

#include <QString>

namespace olive {

class SubtitleParams {
public:
  enum Encoding {
    kEncodingInvalid = -1,
    kISO8859_1,
    kWindows1252,
    kUTF8,
    kUTF8WithBOM,
    kUTF16LE,
    kUTF16BE,
    kEncodingCount
  };

  static QString GetEncodingName(Encoding encoding);

  static bool EncodingHasUnicodeBOM(Encoding encoding);

  static QByteArray GetUnicodeBOM(Encoding encoding);

  static const char *GetQTextStreamCodec(Encoding encoding);

  static QString GenerateASSHeader();

};

}

#endif // SUBTITLEPARAMS_H
