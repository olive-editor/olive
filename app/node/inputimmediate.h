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

#ifndef NODEINPUTIMMEDIATE_H
#define NODEINPUTIMMEDIATE_H

#include "common/timerange.h"
#include "common/xmlutils.h"
#include "node/keyframe.h"
#include "node/value.h"

namespace olive {

class NodeInput;

class NodeInputImmediate
{
public:
  NodeInputImmediate(NodeValue::Type type, const QVector<QVariant>& default_val);

  /**
   * @brief Internal insert function, automatically does an insertion sort based on the keyframe's time
   */
  void insert_keyframe(NodeKeyframe* key);

  void remove_keyframe(NodeKeyframe* key);

  /**
   * @brief Get non-keyframed value split into components (the way it's stored)
   */
  const QVector<QVariant>& get_split_standard_value() const
  {
    return standard_value_;
  }

  void set_standard_value_on_track(const QVariant &value, int track = 0);

  void set_split_standard_value(const QVector<QVariant>& value);

  /**
   * @brief Retrieve a list of keyframe objects for all tracks at a given time
   *
   * List may be empty if this input is not keyframing or has no keyframes at this time.
   */
  QVector<NodeKeyframe*> get_keyframe_at_time(const rational& time) const;

  /**
   * @brief Retrieve the keyframe object at a given time for a given track
   *
   * @return
   *
   * The keyframe object at this time or nullptr if there isn't one or if is_keyframing() is false.
   */
  NodeKeyframe* get_keyframe_at_time_on_track(const rational& time, int track) const;

  /**
   * @brief Gets the closest keyframe to a time
   *
   * If is_keyframing() is false or keyframes_ is empty, this will return nullptr.
   */
  NodeKeyframe* get_closest_keyframe_to_time_on_track(const rational& time, int track) const;

  /**
   * @brief Get closest keyframe that's before the time on any track
   *
   * If no keyframe is before this time, returns nullptr.
   */
  NodeKeyframe* get_closest_keyframe_before_time(const rational& time) const;

  /**
   * @brief Get closest keyframe that's before the time on any track
   *
   * If no keyframe is before this time, returns nullptr.
   */
  NodeKeyframe* get_closest_keyframe_after_time(const rational& time) const;

  /**
   * @brief A heuristic to determine what type a keyframe should be if it's inserted at a certain time (between keyframes)
   */
  NodeKeyframe::Type get_best_keyframe_type_for_time(const rational& time, int track) const;

  /**
   * @brief Return list of keyframes in this parameter
   */
  const QVector<NodeKeyframeTrack> &keyframe_tracks() const
  {
    return keyframe_tracks_;
  }

  /**
   * @brief Return whether keyframing is enabled on this input or not
   */
  bool is_keyframing() const
  {
    return keyframing_;
  }

  /**
   * @brief Set whether keyframing is enabled on this input or not
   */
  void set_is_keyframing(bool k)
  {
    keyframing_ = k;
  }

  /**
   * @brief Gets the earliest keyframe on any track
   */
  NodeKeyframe* get_earliest_keyframe() const;

  /**
   * @brief Gets the latest keyframe on any track
   */
  NodeKeyframe* get_latest_keyframe() const;

  /**
   * @brief Return whether a keyframe exists at this time
   *
   * If is_keyframing() is false, this will always return false. This checks all tracks and will return true if *any*
   * track has a keyframe.
   */
  bool has_keyframe_at_time(const rational &time) const;

  bool is_using_standard_value(int track) const
  {
    return (!is_keyframing() || keyframe_tracks_.at(track).isEmpty());
  }

private:
  /**
   * @brief Non-keyframed value
   */
  QVector<QVariant> standard_value_;

  /**
   * @brief Internal keyframe array
   *
   * If keyframing is enabled, this data is used instead of standard_value.
   */
  QVector<NodeKeyframeTrack> keyframe_tracks_;

  /**
   * @brief Internal keyframing enabled setting
   */
  bool keyframing_;

};

}

#endif // NODEINPUTIMMEDIATE_H
