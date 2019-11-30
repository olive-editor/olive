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

#include "output.h"

#include "node/node.h"

NodeOutput::NodeOutput(const QString &id) :
  NodeParam(id)
{
}

NodeParam::Type NodeOutput::type()
{
  return kOutput;
}

QVariant NodeOutput::get_realtime_value()
{
  QVariant v = parentNode()->Value(this);

  return v;
}

bool NodeOutput::has_cached_value(const TimeRange &time)
{
  cache_lock_.lock();

  bool contains = cached_values_.contains(time);

  cache_lock_.unlock();

  return contains;
}

QVariant NodeOutput::get_cached_value(const TimeRange &time)
{
  cache_lock_.lock();

  QVariant val = cached_values_.value(time);

  cache_lock_.unlock();

  return val;
}

void NodeOutput::cache_value(const TimeRange &time, const QVariant &value)
{
  cache_lock_.lock();
  cached_values_.insert(time, value);
  cache_lock_.unlock();
}

void NodeOutput::drop_cached_values()
{
  cache_lock_.lock();
  cached_values_.clear();
  cache_lock_.unlock();
}

void NodeOutput::drop_cached_values_overlapping(const TimeRange &time)
{
  cache_lock_.lock();

  QHash<TimeRange, QVariant>::iterator i = cached_values_.begin();

  while (i != cached_values_.end()) {
    if (i.key().OverlapsWith(time)) {
      i = cached_values_.erase(i);
    } else {
      i++;
    }
  }

  cache_lock_.unlock();
}
