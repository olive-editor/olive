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

#ifndef SEQUENCEPARAM_H
#define SEQUENCEPARAM_H

#include "common/rational.h"
#include "render/pixelformat.h"

OLIVE_NAMESPACE_ENTER

struct SequencePreset {
  QString name;
  int width;
  int height;
  rational frame_rate;
  int sample_rate;
  uint64_t channel_layout;
  int preview_divider;
  PixelFormat::Format preview_format;
};

OLIVE_NAMESPACE_EXIT

#endif // SEQUENCEPARAM_H
