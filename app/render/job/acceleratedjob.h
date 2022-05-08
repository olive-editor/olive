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

#ifndef ACCELERATEDJOB_H
#define ACCELERATEDJOB_H

#include "node/param.h"
#include "node/valuedatabase.h"

namespace olive {

class AcceleratedJob {
public:
  AcceleratedJob() = default;

  NodeValue GetValue(const QString& input) const
  {
    return value_map_.value(input);
  }

  void InsertValue(const QString &input, const NodeValueRow &row)
  {
    value_map_.insert(input, row.value(input));
  }

  void InsertValue(const QString& input, const NodeValue& value)
  {
    value_map_.insert(input, value);
  }

  void InsertValue(const NodeValueRow &row)
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    value_map_.insert(row);
#else
    for (auto it=row.cbegin(); it!=row.cend(); it++) {
      value_map_.insert(it.key(), it.value());
    }
#endif
  }

  const NodeValueRow &GetValues() const { return value_map_; }
  NodeValueRow &GetValues() { return value_map_; }

private:
  NodeValueRow value_map_;

};

}

#endif // ACCELERATEDJOB_H
