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

#ifndef RENDERCACHE_H
#define RENDERCACHE_H

#include <QMap>
#include <QMutex>

OLIVE_NAMESPACE_ENTER

template<class K, class V>
class RenderCache
{
public:
  RenderCache() = default;

  void Clear(){values_.clear();}

  void Add(K key, V val){values_.insert(key, val);}

  V Get(K key) const {return values_.value(key);}

  bool Has(K key) const {return values_.contains(key);}

  void Remove(K key) {values_.remove(key);}

private:
  QHash<K, V> values_;

};

OLIVE_NAMESPACE_EXIT

#endif // RENDERCACHE_H
