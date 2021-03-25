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

#ifndef RENDERCACHE_H
#define RENDERCACHE_H

#include "codec/decoder.h"

namespace olive {

template <typename K, typename V>
class RenderCache : public QHash<K, V>
{
public:
  QMutex *mutex()
  {
    return &mutex_;
  }

private:
  QMutex mutex_;

};

using DecoderCache = RenderCache<Decoder::CodecStream, DecoderPtr>;
using ShaderCache = RenderCache<QString, QVariant>;

}

#endif // RENDERCACHE_H
