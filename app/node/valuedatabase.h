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

  using const_iterator = QHash<QString, NodeValueTable>::const_iterator;

  inline QHash<QString, NodeValueTable>::const_iterator begin() const
  {
    return tables_.cbegin();
  }

  inline QHash<QString, NodeValueTable>::const_iterator end() const
  {
    return tables_.cend();
  }

  inline bool contains(const QString& s) const
  {
    return tables_.contains(s);
  }

private:
  QHash<QString, NodeValueTable> tables_;

};

using NodeValueMap = QHash<QString, NodeValue>;

}

Q_DECLARE_METATYPE(olive::NodeValueDatabase)

#endif // NODEVALUEDATABASE_H
