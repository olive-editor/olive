/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2023 Olive Studios LLC

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

#ifndef SWIZZLE_H
#define SWIZZLE_H

#include <cstddef>
#include <map>

namespace olive {

class SwizzleMap
{
public:
  SwizzleMap() = default;

  bool empty() const { return map_.empty(); }

  void clear() { map_.clear(); }

  void insert(size_t to, size_t from)
  {
    map_[to] = from;
  }

  void remove(size_t to)
  {
    map_.erase(to);
  }

  bool operator==(const SwizzleMap &m) const { return map_ == m.map_; }
  bool operator!=(const SwizzleMap &m) const { return map_ != m.map_; }

  std::map<size_t, size_t>::const_iterator cbegin() const { return map_.cbegin(); }
  std::map<size_t, size_t>::const_iterator cend() const { return map_.cend(); }

private:
  std::map<size_t, size_t> map_;

};

}

#endif // SWIZZLE_H
