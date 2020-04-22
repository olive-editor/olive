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

#include "sampleformat.h"

#include "core.h"

OLIVE_NAMESPACE_ENTER

const SampleFormat::Format SampleFormat::kInternalFormat = SAMPLE_FMT_FLT;

QString SampleFormat::GetSampleFormatName(const SampleFormat::Format &f)
{
  switch (f) {
  case SAMPLE_FMT_U8:
    return tr("Unsigned 8-bit");
  case SAMPLE_FMT_S16:
    return tr("Signed 16-bit");
  case SAMPLE_FMT_S32:
    return tr("Signed 32-bit");
  case SAMPLE_FMT_S64:
    return tr("Signed 64-bit");
  case SAMPLE_FMT_FLT:
    return tr("32-bit Float");
  case SAMPLE_FMT_DBL:
    return tr("64-bit Float");
  case SAMPLE_FMT_COUNT:
  case SAMPLE_FMT_INVALID:
    break;
  }

  return tr("Invalid");
}

OLIVE_NAMESPACE_EXIT
