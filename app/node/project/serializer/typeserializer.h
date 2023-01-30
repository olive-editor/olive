/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Team

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

#ifndef TYPESERIALIZER_H
#define TYPESERIALIZER_H

#include <olive/core/core.h>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/xmlutils.h"

namespace olive {

using namespace core;

class TypeSerializer
{
public:
  TypeSerializer() = default;

  static AudioParams LoadAudioParams(QXmlStreamReader *reader);
  static void SaveAudioParams(QXmlStreamWriter *writer, const AudioParams &a);

};

}

#endif // TYPESERIALIZER_H
