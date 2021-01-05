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

#include "inputimmediate.h"

#include "common/bezier.h"
#include "common/lerp.h"
#include "common/tohex.h"
#include "input.h"

namespace olive {

NodeInputImmediate::NodeInputImmediate(NodeValue::Type type, const QVector<QVariant> &default_val) :
  keyframing_(false)
{
  int track_size = NodeValue::get_number_of_keyframe_tracks(type);

  keyframe_tracks_.resize(track_size);
  standard_value_.resize(track_size);

  set_split_standard_value(default_val);
}

void NodeInputImmediate::set_standard_value_on_track(const QVariant &value, int track)
{
  standard_value_.replace(track, value);
}

void NodeInputImmediate::set_split_standard_value(const QVector<QVariant> &value)
{
  for (int i=0; i<value.size() && i<standard_value_.size(); i++) {
    standard_value_[i] = value[i];
  }
}

QVector<NodeKeyframe*> NodeInputImmediate::get_keyframe_at_time(const rational &time) const
{
  QVector<NodeKeyframe*> keys;

  for (int i=0;i<keyframe_tracks_.size();i++) {
    NodeKeyframe* key_at_time = get_keyframe_at_time_on_track(time, i);

    if (key_at_time) {
      keys.append(key_at_time);
    }
  }

  return keys;
}

NodeKeyframe* NodeInputImmediate::get_keyframe_at_time_on_track(const rational &time, int track) const
{
  if (!is_using_standard_value(track)) {
    foreach (NodeKeyframe* key, keyframe_tracks_.at(track)) {
      if (key->time() == time) {
        return key;
      }
    }
  }

  return nullptr;
}

NodeKeyframe* NodeInputImmediate::get_closest_keyframe_to_time_on_track(const rational &time, int track) const
{
  if (is_using_standard_value(track)) {
    return nullptr;
  }

  const NodeKeyframeTrack& key_track = keyframe_tracks_.at(track);

  if (time <= key_track.first()->time()) {
    return key_track.first();
  }

  if (time >= key_track.last()->time()) {
    return key_track.last();
  }

  for (int i=1;i<key_track.size();i++) {
    NodeKeyframe* prev_key = key_track.at(i-1);
    NodeKeyframe* next_key = key_track.at(i);

    if (prev_key->time() <= time && next_key->time() >= time) {
      // Return whichever is closer
      rational prev_diff = time - prev_key->time();
      rational next_diff = next_key->time() - time;

      if (next_diff < prev_diff) {
        return next_key;
      } else {
        return prev_key;
      }
    }
  }

  return nullptr;
}

NodeKeyframe *NodeInputImmediate::get_closest_keyframe_before_time(const rational &time) const
{
  NodeKeyframe* key = nullptr;

  foreach (const NodeKeyframeTrack& track, keyframe_tracks_) {
    foreach (NodeKeyframe* k, track) {
      if (k->time() >= time) {
        break;
      } else if (!key || k->time() > key->time()) {
        key = k;
      }
    }
  }

  return key;
}

NodeKeyframe* NodeInputImmediate::get_closest_keyframe_after_time(const rational &time) const
{
  NodeKeyframe* key = nullptr;

  foreach (const NodeKeyframeTrack& track, keyframe_tracks_) {
    for (int i=track.size()-1;i>=0;i--) {
      NodeKeyframe* k = track.at(i);

      if (k->time() <= time) {
        break;
      } else if (!key || k->time() < key->time()) {
        key = k;
      }
    }
  }

  return key;
}

NodeKeyframe::Type NodeInputImmediate::get_best_keyframe_type_for_time(const rational &time, int track) const
{
  NodeKeyframe* closest_key = get_closest_keyframe_to_time_on_track(time, track);

  if (closest_key) {
    return closest_key->type();
  }

  return NodeKeyframe::kDefaultType;
}

bool NodeInputImmediate::has_keyframe_at_time(const rational &time) const
{
  if (!is_keyframing()) {
    return false;
  }

  // Loop through keyframes to see if any match
  foreach (const NodeKeyframeTrack& track, keyframe_tracks_) {
    foreach (NodeKeyframe* key, track) {
      if (key->time() == time) {
        return true;
      }
    }
  }

  // None match
  return false;
}

NodeKeyframe *NodeInputImmediate::get_earliest_keyframe() const
{
  NodeKeyframe* earliest = nullptr;

  foreach (const NodeKeyframeTrack& track, keyframe_tracks_) {
    if (!track.isEmpty()) {
      NodeKeyframe* earliest_in_track = track.first();

      if (!earliest
          || earliest_in_track->time() < earliest->time()) {
        earliest = earliest_in_track;
      }
    }
  }

  return earliest;
}

NodeKeyframe *NodeInputImmediate::get_latest_keyframe() const
{
  NodeKeyframe* latest = nullptr;

  foreach (const NodeKeyframeTrack& track, keyframe_tracks_) {
    if (!track.isEmpty()) {
      NodeKeyframe* latest_in_track = track.last();

      if (!latest
          || latest_in_track->time() > latest->time()) {
        latest = latest_in_track;
      }
    }
  }

  return latest;
}

void NodeInputImmediate::insert_keyframe(NodeKeyframe* key)
{
  NodeKeyframeTrack& key_track = keyframe_tracks_[key->track()];

  for (int i=0;i<key_track.size();i++) {
    NodeKeyframe* compare = key_track.at(i);

    // Ensure we aren't trying to insert two keyframes at the same time
    Q_ASSERT(compare->time() != key->time());

    if (compare->time() > key->time()) {
      key_track.insert(i, key);
      return;
    }
  }

  key_track.append(key);
}

void NodeInputImmediate::remove_keyframe(NodeKeyframe *key)
{
  keyframe_tracks_[key->track()].removeOne(key);
}

}
