/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "globals.h"

#include "common/qtutils.h"

namespace olive {

ValueParams ValueParams::time_transformed(const TimeRange &time) const
{
  ValueParams g = *this;
  g.time_ = time;
  return g;
}

ValueParams ValueParams::output_edited(const QString &output) const
{
  ValueParams g = *this;
  g.output_ = output;
  return g;
}

ValueParams ValueParams::loop_mode_edited(const LoopMode &lm) const
{
  ValueParams g = *this;
  g.loop_mode_ = lm;
  return g;
}

ValueParams ValueParams::with_cache(Cache *cache) const
{
  ValueParams g = *this;
  g.cache_ = cache;
  return g;
}

bool ValueParams::get_cached_value(const Node *node, const ValueParams &p, value_t &out) const
{
  if (cache_) {
    if (cache_->contains(node)) {
      const QHash<ValueParams, value_t> &map = cache_->value(node);
      if (map.contains(p)) {
        out = map.value(p);
        return true;
      }
    }
  }

  return false;
}

void ValueParams::insert_cached_value(const Node *node, const ValueParams &p, const value_t &in) const
{
  if (cache_) {
    (*cache_)[node][p] = in;
  }
}

bool ValueParams::operator==(const ValueParams &p) const
{
  return this->video_params_ == p.video_params_
         && this->audio_params_ == p.audio_params_
         && this->time_ == p.time_
         && this->loop_mode_ == p.loop_mode_
         && this->output_ == p.output_;
}

uint qHash(const ValueParams &p, uint seed)
{
  return qHash(p.vparams(), seed) ^ qHash(p.aparams(), seed) ^ qHash(p.time(), seed) ^ qHash(int(p.loop_mode()), seed) ^ qHash(p.output(), seed);
}

}
