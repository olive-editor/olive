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

#ifndef VIEWERSAFEMARGININFO_H
#define VIEWERSAFEMARGININFO_H

#include <QtMath>

#include "common/define.h"

namespace olive {

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

  bool operator==(const ViewerSafeMarginInfo& rhs) const {
    return (enabled_ == rhs.enabled_ && qFuzzyCompare(ratio_, rhs.ratio_));
  }

  bool operator!=(const ViewerSafeMarginInfo& rhs) const {
    return (enabled_ != rhs.enabled_ || !qFuzzyCompare(ratio_, rhs.ratio_));
  }

private:
  bool enabled_;

  double ratio_;

};

}

#endif // VIEWERSAFEMARGININFO_H
