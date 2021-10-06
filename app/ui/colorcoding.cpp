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
  Color(0.412, 0.188, 0.259),
  Color(0.561, 0.427, 0.239),
  Color(0.486, 0.306, 0.235),
  Color(0.631, 0.612, 0.212),
  Color(0.404, 0.478, 0.243),
  Color(0.349, 0.576, 0.275),
  Color(0.224, 0.459, 0.251),
  Color(0.259, 0.471, 0.541),
  Color(0.184, 0.376, 0.329),
  Color(0.259, 0.365, 0.541),
  Color(0.196, 0.216, 0.412),
  Color(0.612, 0.294, 0.502),
  Color(0.404, 0.220, 0.459),
  Color(0.800, 0.800, 0.800),
  Color(0.502, 0.502, 0.502)
};

QString ColorCoding::GetColorName(int c)
{
  // FIXME: I'm sure we could come up with more creative names for these colors
  switch (c) {
  case kRed:
    return tr("Red");
  case kMaroon:
    return tr("Maroon");
  case kOrange:
    return tr("Orange");
  case kBrown:
    return tr("Brown");
  case kYellow:
    return tr("Yellow");
  case kOlive:
    return tr("Olive");
  case kLime:
    return tr("Lime");
  case kGreen:
    return tr("Green");
  case kCyan:
    return tr("Cyan");
  case kTeal:
    return tr("Teal");
  case kBlue:
    return tr("Blue");
  case kNavy:
    return tr("Navy");
  case kPink:
    return tr("Pink");
  case kPurple:
    return tr("Purple");
  case kSilver:
    return tr("Silver");
  case kGray:
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
