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

NodeInput::NodeInput(const QString& id) :
  NodeParam(id),
  keyframing_(false),
  dependent_(true),
  has_minimum_(false),
  has_maximum_(false)
{
  // Have at least one keyframe/value active at any time
  keyframes_.append(NodeKeyframe());
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

void NodeInput::set_data_type(const NodeParam::DataType &type)
{
  data_type_ = type;
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
  if (is_keyframing()) {
    if (keyframes_.first().time() >= time) {
      // This time precedes any keyframe, so we just return the first value
      return keyframes_.first().value();
    }

    if (keyframes_.last().time() <= time) {
      // This time is after any keyframes so we return the last value
      return keyframes_.last().value();
    }

    // If we're here, the time must be somewhere in between the keyframes
    for (int i=0;i<keyframes_.size()-1;i++) {
      const NodeKeyframe& before = keyframes_.at(i);
      const NodeKeyframe& after = keyframes_.at(i+1);

      if (before.time() == time
          || data_type() != kFloat // FIXME: Expand this to other types that can be interpolated
          || (before.time() < time && before.type() == NodeKeyframe::kHold)) {

        // Time == keyframe time, so value is precise
        return before.value();

      } else if (before.time() < time && after.time() > time) {
        // We must interpolate between these keyframes

        if (before.type() == NodeKeyframe::kBezier && after.type() == NodeKeyframe::kBezier) {
          // FIXME: Perform a cubic bezier interpolation
        } else if (before.type() == NodeKeyframe::kLinear && after.type() == NodeKeyframe::kBezier) {
          // FIXME: Perform a quadratic bezier interpolation with anchors from the AFTER keyframe
        } else if (before.type() == NodeKeyframe::kLinear && after.type() == NodeKeyframe::kBezier) {
          // FIXME: Perform a quadratic bezier interpolation with anchors from the BEFORE keyframe
        } else {
          // To have arrived here, the keyframes must both be linear
          qreal period_progress = (time.toDouble() - before.time().toDouble()) / (after.time().toDouble() - before.time().toDouble());
          qreal interpolated_value = lerp(before.value().toDouble(), after.value().toDouble(), period_progress);

          return interpolated_value;
        }
      }
    }
  }

  return keyframes_.first().value();
}

void NodeInput::set_value_at_time(const rational &time, const QVariant &value)
{
  if (parentNode() != nullptr)
    parentNode()->LockUserInput();

  // We set up these variables in advance (see end of function)
  bool signal_vc = false;
  TimeRange signal_vc_range;

  if (is_keyframing()) {
    // Insert value into the keyframe list chronologically

    if (keyframes_.first().time() > time) {

      // Store this away for the ValueChanged signal we emit later
      rational existing_first_key = keyframes_.first().time();

      // Insert at the beginning
      keyframes_.prepend(NodeKeyframe(time, value, keyframes_.first().type()));

      // Value has changed since the earliest point up until the ex-first keyframe (since the frames
      // interpolating between the key we're adding and the key that existed are changing too)
      signal_vc = true;
      signal_vc_range = TimeRange(RATIONAL_MIN, existing_first_key);

    } else if (keyframes_.first().time() == time) {

      // Replace first value
      keyframes_.first().set_value(value);

      // Value has changed since the earliest point up until the keyframe we just changed
      signal_vc = true;
      signal_vc_range = TimeRange(RATIONAL_MIN, time);

    } else if (keyframes_.last().time() < time) {

      // Store this away for the ValueChanged signal we emit later
      rational existing_last_key = keyframes_.last().time();

      // Append at the end
      keyframes_.append(NodeKeyframe(time, value, keyframes_.last().type()));

      // Value has changed since the ex-last point up until the latest possible point (since the frames
      // interpolating between the key we're adding and the key that existed are changing too)
      signal_vc = true;
      signal_vc_range = TimeRange(existing_last_key, RATIONAL_MAX);

    } else if (keyframes_.last().time() == time) {

      // Replace last value
      keyframes_.last().set_value(value);

      // Value has changed from this point until the latest possible point
      signal_vc = true;
      signal_vc_range = TimeRange(time, RATIONAL_MAX);

    } else {
      for (int i=0;i<keyframes_.size()-1;i++) {
        NodeKeyframe& before = keyframes_[i];
        NodeKeyframe& after = keyframes_[i+1];

        if (before.time() == time) {
          // Found exact match, replace it
          before.set_value(value);

          // Values have changed since the last keyframe and the next one
          signal_vc = true;
          signal_vc_range = TimeRange(keyframes_.at(i-1).time(), keyframes_.at(i+1).time());
          break;
        } else if (before.time() < time && after.time() > time) {
          // Insert value in between these two keyframes
          keyframes_.insert(i+1, NodeKeyframe(time, value, before.type()));

          // Values have changed since the last keyframe and the next one
          signal_vc = true;
          signal_vc_range = TimeRange(before.time(), after.time());
          break;
        }
      }
    }

  } else {
    keyframes_.first().set_value(value);

    // Values have changed for all times since the value is static
    signal_vc = true;
    signal_vc_range = TimeRange(RATIONAL_MIN, RATIONAL_MAX);
  }

  if (parentNode() != nullptr)
    parentNode()->UnlockUserInput();

  // We make sure this signal is emitted AFTER we've unlocked the node in case this leads to a tangent where something
  // tries to relock this node before it's unlocked here
  if (signal_vc)
    emit ValueChanged(signal_vc_range.in(), signal_vc_range.out());
}

bool NodeInput::is_keyframing() const
{
  return keyframing_;
}

void NodeInput::set_is_keyframing(bool k)
{
  keyframing_ = k;
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

  // Copy values
  dest->keyframes_ = source->keyframes_;

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

    dst_array->SetSize(src_array->GetSize());

    for (int i=0;i<dst_array->GetSize();i++) {
      CopyValues(src_array->At(i), dst_array->At(i), include_connections);
    }
  }
}
