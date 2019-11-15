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

#ifndef SAMPLEFORMAT_H
#define SAMPLEFORMAT_H

enum SampleFormat {
  SAMPLE_FMT_INVALID = -1,

  SAMPLE_FMT_U8,
  SAMPLE_FMT_S16,
  SAMPLE_FMT_S32,
  SAMPLE_FMT_S64,
  SAMPLE_FMT_FLT,
  SAMPLE_FMT_DBL,

  SAMPLE_FMT_COUNT
};

#endif // SAMPLEFORMAT_H
