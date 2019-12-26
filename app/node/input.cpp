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

#include "input.h"

#include "common/lerp.h"
#include "node.h"
#include "output.h"
#include "inputarray.h"

NodeInput::NodeInput(const QString& id, const DataType &type, const QVariant &default_value) :
  NodeParam(id),
  data_type_(type),
  keyframable_(true),
  standard_value_(default_value),
  keyframing_(false),
  dependent_(true),
  has_minimum_(false),
  has_maximum_(false)
{
}

bool NodeInput::IsArray()
{
  return false;
}

NodeParam::Type NodeInput::type()
{
  return kInput;
}

QString NodeInput::name()
{
  if (name_.isEmpty()) {
    return tr("Input");
  }

  return NodeParam::name();
}

const NodeParam::DataType &NodeInput::data_type() const
{
  return data_type_;
}

NodeOutput *NodeInput::get_connected_output() const
{
  if (!edges_.isEmpty()) {
    return edges_.first()->output();
  }

  return nullptr;
}

Node *NodeInput::get_connected_node() const
{
  NodeOutput* output = get_connected_output();

  if (output != nullptr) {
    return output->parentNode();
  }

  return nullptr;
}

QVariant NodeInput::get_value_at_time(const rational &time) const
{
  if (is_using_standard_value()) {
    return standard_value_;
  }

  if (keyframes_.first()->time() >= time) {
    // This time precedes any keyframe, so we just return the first value
    return keyframes_.first()->value();
  }

  if (keyframes_.last()->time() <= time) {
    // This time is after any keyframes so we return the last value
    return keyframes_.last()->value();
  }

  // If we're here, the time must be somewhere in between the keyframes
  for (int i=0;i<keyframes_.size()-1;i++) {
    NodeKeyframePtr before = keyframes_.at(i);
    NodeKeyframePtr after = keyframes_.at(i+1);

    if (before->time() == time
        || data_type() != kFloat // FIXME: Expand this to other types that can be interpolated
        || (before->time() < time && before->type() == NodeKeyframe::kHold)) {

      // Time == keyframe time, so value is precise
      return before->value();

    } else if (before->time() < time && after->time() > time) {
      // We must interpolate between these keyframes

      if (before->type() == NodeKeyframe::kBezier && after->type() == NodeKeyframe::kBezier) {
        // FIXME: Perform a cubic bezier interpolation
      } else if (before->type() == NodeKeyframe::kLinear && after->type() == NodeKeyframe::kBezier) {
        // FIXME: Perform a quadratic bezier interpolation with anchors from the AFTER keyframe
      } else if (before->type() == NodeKeyframe::kLinear && after->type() == NodeKeyframe::kBezier) {
        // FIXME: Perform a quadratic bezier interpolation with anchors from the BEFORE keyframe
      } else {
        // To have arrived here, the keyframes must both be linear
        qreal period_progress = (time.toDouble() - before->time().toDouble()) / (after->time().toDouble() - before->time().toDouble());
        qreal interpolated_value = lerp(before->value().toDouble(), after->value().toDouble(), period_progress);

        return interpolated_value;
      }
    }
  }

  return standard_value_;
}

NodeKeyframePtr NodeInput::get_keyframe_at_time(const rational &time) const
{
  if (is_using_standard_value()) {
    return nullptr;
  }

  foreach (NodeKeyframePtr key, keyframes_) {
    if (key->time() == time) {
      return key;
    }
  }

  return nullptr;
}

NodeKeyframePtr NodeInput::get_closest_keyframe_to_time(const rational &time) const
{
  if (is_using_standard_value()) {
    return nullptr;
  }

  if (time <= keyframes_.first()->time()) {
    return keyframes_.first();
  }

  if (time >= keyframes_.last()->time()) {
    return keyframes_.last();
  }

  for (int i=1;i<keyframes_.size();i++) {
    NodeKeyframePtr prev_key = keyframes_.at(i-1);
    NodeKeyframePtr next_key = keyframes_.at(i);

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

NodeKeyframe::Type NodeInput::get_best_keyframe_type_for_time(const rational &time) const
{
  NodeKeyframePtr closest_key = get_closest_keyframe_to_time(time);

  if (closest_key) {
    return closest_key->type();
  }

  return NodeKeyframe::kDefaultType;
}

void NodeInput::insert_keyframe(NodeKeyframePtr key)
{
  Q_ASSERT(is_keyframable() || keyframes_.isEmpty());

  insert_keyframe_internal(key);

  connect(key.get(), &NodeKeyframe::TimeChanged, this, &NodeInput::KeyframeTimeChanged);
  connect(key.get(), &NodeKeyframe::ValueChanged, this, &NodeInput::KeyframeValueChanged);

  emit KeyframeAdded(key);
}

void NodeInput::remove_keyframe(NodeKeyframePtr key)
{
  Q_ASSERT(is_keyframable() && keyframes_.size() > 1);

  disconnect(key.get(), &NodeKeyframe::TimeChanged, this, &NodeInput::KeyframeTimeChanged);
  disconnect(key.get(), &NodeKeyframe::ValueChanged, this, &NodeInput::KeyframeValueChanged);

  keyframes_.removeOne(key);

  emit KeyframeRemoved(key);
}

void NodeInput::KeyframeTimeChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  Q_ASSERT(keyframe_index > -1);

  if ((keyframe_index > 0 && keyframes_.at(keyframe_index - 1)->time() > key->time())
      || (keyframe_index < keyframes_.size() - 1 && keyframes_.at(keyframe_index + 1)->time() < key->time())) {
    // This keyframe needs resorting, store it and remove it from the list
    NodeKeyframePtr key_shared_ptr = keyframes_.at(keyframe_index);
    keyframes_.removeAt(keyframe_index);

    // Automatically insertion sort
    insert_keyframe_internal(key_shared_ptr);
  }
}

void NodeInput::KeyframeValueChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  rational range_begin = RATIONAL_MIN;
  rational range_end = RATIONAL_MAX;

  if (keyframes_.size() > 1) {
    if (keyframe_index == 0) {
      // This is the earliest keyframe, all we need to do is invalidate the earliest point up until the next keyframe
      range_end = keyframes_.at(1)->time();
    } else {
      // Range is somewhere in the middle or towards the end

      // Check previous keyframe
      NodeKeyframePtr previous_key = keyframes_.at(keyframe_index - 1);
      if (previous_key->type() == NodeKeyframe::kHold) {
        // If the PREVIOUS keyframe is a hold, it won't be affected by this
        range_begin = key->time();
      } else {
        // Otherwise, the frames between the previous and this keyframe will be affected too
        range_begin = previous_key->time();
      }

      // Check if this keyframe is the last keyframe
      if (keyframe_index == keyframes_.size() - 1) {
        // If so, we'll invalidate until the latest point
        range_end = RATIONAL_MAX;
      } else {
        // Otherwise, we only need to invalidate up until the next keyframe
        range_end = keyframes_.at(keyframe_index + 1)->time();
      }
    }
  }

  emit ValueChanged(range_begin, range_end);
}

int NodeInput::FindIndexOfKeyframeFromRawPtr(NodeKeyframe *raw_ptr) const
{
  for (int i=0;i<keyframes_.size();i++) {
    if (keyframes_.at(i).get() == raw_ptr) {
      return i;
    }
  }

  return -1;
}

void NodeInput::insert_keyframe_internal(NodeKeyframePtr key)
{
  for (int i=0;i<keyframes_.size();i++) {
    NodeKeyframePtr compare = keyframes_.at(i);

    // Ensure we aren't trying to insert two keyframes at the same time
    Q_ASSERT(compare->time() != key->time());

    if (compare->time() > key->time()) {
      keyframes_.insert(i, key);
      return;
    }
  }

  keyframes_.append(key);
}

bool NodeInput::is_using_standard_value() const
{
  return (!is_keyframing() || keyframes_.isEmpty());
}

bool NodeInput::has_keyframe_at_time(const rational &time) const
{
  // If we aren't keyframing, there definitely isn't a keyframe at a given time
  if (is_using_standard_value()) {
    return false;
  }

  // Loop through keyframes to see if any match
  foreach (NodeKeyframePtr key, keyframes_) {
    if (key->time() == time) {
      return true;
    }
  }

  // None match
  return false;
}

bool NodeInput::is_keyframing() const
{
  return keyframing_;
}

void NodeInput::set_is_keyframing(bool k)
{
  keyframing_ = k;

  emit KeyframeEnableChanged(keyframing_);
}

bool NodeInput::is_keyframable() const
{
  return keyframable_;
}

const QVariant &NodeInput::get_standard_value() const
{
  return standard_value_;
}

void NodeInput::set_standard_value(const QVariant &value)
{
  standard_value_ = value;

  if (is_using_standard_value()) {
    // If this standard value is being used, we need to send a value changed signal
    emit ValueChanged(RATIONAL_MIN, RATIONAL_MAX);
  }
}

const QList<NodeKeyframePtr> &NodeInput::keyframes() const
{
  return keyframes_;
}

void NodeInput::set_is_keyframable(bool k)
{
  keyframable_ = k;
}

const QVariant &NodeInput::minimum() const
{
  return minimum_;
}

bool NodeInput::has_minimum() const
{
  return has_minimum_;
}

void NodeInput::set_minimum(const QVariant &min)
{
  minimum_ = min;
  has_minimum_ = true;
}

const QVariant &NodeInput::maximum() const
{
  return maximum_;
}

bool NodeInput::has_maximum() const
{
  return has_maximum_;
}

void NodeInput::set_maximum(const QVariant &max)
{
  maximum_ = max;
  has_maximum_ = true;
}

void NodeInput::CopyValues(NodeInput *source, NodeInput *dest, bool include_connections, bool lock_connections)
{
  Q_ASSERT(source->id() == dest->id());

  // Copy standard value
  dest->standard_value_ = source->standard_value_;

  // Copy keyframes
  dest->keyframes_.clear();
  foreach (NodeKeyframePtr key, source->keyframes_) {
    NodeKeyframePtr copy = std::make_shared<NodeKeyframe>(key->time(), key->value(), key->type());
    dest->keyframes_.append(copy);
  }

  // Copy keyframing state
  dest->set_is_keyframing(source->is_keyframing());

  // Copy connections
  if (include_connections && source->get_connected_output() != nullptr) {
    ConnectEdge(source->get_connected_output(), dest, lock_connections);
  }

  // If these inputs are an array, copy the subparams too
  if (dest->IsArray()) {
    NodeInputArray* src_array = static_cast<NodeInputArray*>(source);
    NodeInputArray* dst_array = static_cast<NodeInputArray*>(dest);

    dst_array->SetSize(src_array->GetSize(), lock_connections);

    for (int i=0;i<dst_array->GetSize();i++) {
      CopyValues(src_array->At(i), dst_array->At(i), include_connections);
    }
  }

  emit dest->ValueChanged(RATIONAL_MIN, RATIONAL_MAX);
}
