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

#include "colorcoding.h"

namespace olive {

QVector<Color> ColorCoding::colors_ = {
  Color(0.545, 0.255, 0.255),
  Color(0.459, 0.220, 0.290),
  Color(0.561, 0.427, 0.239),
  Color(0.486, 0.306, 0.235),
  Color(0.561, 0.541, 0.239),
  Color(0.447, 0.541, 0.259),
  Color(0.333, 0.541, 0.259),
  Color(0.224, 0.459, 0.251),
  Color(0.259, 0.471, 0.541),
  Color(0.184, 0.376, 0.329),
  Color(0.259, 0.365, 0.541),
  Color(0.239, 0.263, 0.502),
  Color(0.580, 0.282, 0.478),
  Color(0.475, 0.259, 0.541),
  Color(0.800, 0.800, 0.800),
  Color(0.502, 0.502, 0.502)
};

QString ColorCoding::GetColorName(int c)
{
  // FIXME: I'm sure we could come up with more creative names for these colors
  switch (c) {
  case 0:
    return tr("Red");
  case 1:
    return tr("Maroon");
  case 2:
    return tr("Orange");
  case 3:
    return tr("Brown");
  case 4:
    return tr("Yellow");
  case 5:
    return tr("Olive");
  case 6:
    return tr("Lime");
  case 7:
    return tr("Green");
  case 8:
    return tr("Cyan");
  case 9:
    return tr("Teal");
  case 10:
    return tr("Blue");
  case 11:
    return tr("Navy");
  case 12:
    return tr("Pink");
  case 13:
    return tr("Purple");
  case 14:
    return tr("Silver");
  case 15:
    return tr("Gray");
  }

  return QString();
}

Color ColorCoding::GetColor(int c)
{
  return colors_.at(c);
}

Qt::GlobalColor ColorCoding::GetUISelectorColor(const Color &c)
{
  if (c.GetRoughLuminance() > 0.40) {
    return Qt::black;
  } else {
    return Qt::white;
  }
}

}
