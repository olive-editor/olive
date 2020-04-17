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

#ifndef VIEWERSAFEMARGININFO_H
#define VIEWERSAFEMARGININFO_H

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

class ViewerSafeMarginInfo {
public:
  ViewerSafeMarginInfo() :
    enabled_(false)
  {
  }

  ViewerSafeMarginInfo(bool enabled, double ratio = 0) :
    enabled_(enabled),
    ratio_(ratio)
  {
  }

  bool is_enabled() const
  {
    return enabled_;
  }

  bool custom_ratio() const
  {
    return (ratio_ > 0);
  }

  double ratio() const
  {
    return ratio_;
  }

private:
  bool enabled_;

  double ratio_;

};

OLIVE_NAMESPACE_EXIT

#endif // VIEWERSAFEMARGININFO_H
