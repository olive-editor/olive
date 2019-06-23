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

#ifndef VIDEOSTREAM_H
#define VIDEOSTREAM_H

#include "rational.h"

class Footage;

class VideoStream
{
public:
  VideoStream(Footage* footage, const int& index, const int& width, const int& height, const rational& timebase);

  Footage* footage();
  void set_footage(Footage* f);

  const int& index();
  void set_index(const int& index);

  const int& width();
  void set_width(const int& width);

  const int& height();
  void set_height(const int& height);

  const rational& timebase();
  void set_timebase(const rational& timebase);

private:
  Footage* footage_;

  int index_;
  int width_;
  int height_;
  rational timebase_;
};

#endif // VIDEOSTREAM_H
