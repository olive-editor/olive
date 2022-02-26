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

#ifndef NODEVALUEDATABASE_H
#define NODEVALUEDATABASE_H

#include "param.h"
#include "value.h"

namespace olive {

class NodeValueDatabase
{
public:
  NodeValueDatabase() = default;

  NodeValueTable& operator[](const QString& input_id)
  {
    return tables_[input_id];
  }

  void Insert(const QString& key, const NodeValueTable &value)
  {
    tables_.insert(key, value);
  }

  NodeValueTable Merge() const;

  using Tables = QHash<QString, NodeValueTable>;
  using const_iterator = Tables::const_iterator;
  using iterator = Tables::iterator;

  inline const_iterator cbegin() const
  {
    return tables_.cbegin();
  }

  inline const_iterator cend() const
  {
    return tables_.cend();
  }

  inline iterator begin()
  {
    return tables_.begin();
  }

  inline iterator end()
  {
    return tables_.end();
  }

  inline bool contains(const QString& s) const
  {
    return tables_.contains(s);
  }

private:
  Tables tables_;

};

}

Q_DECLARE_METATYPE(olive::NodeValueDatabase)

#endif // NODEVALUEDATABASE_H
