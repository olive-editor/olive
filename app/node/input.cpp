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

#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "common/bezier.h"
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

bool NodeInput::type_can_be_interpolated(NodeParam::DataType type)
{
  return type == kFloat
      || type == kVec2
      || type == kVec3
      || type == kVec4
      || type == kColor;
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
        || !type_can_be_interpolated(data_type())
        || (before->time() < time && before->type() == NodeKeyframe::kHold)) {

      // Time == keyframe time, so value is precise
      return before->value();

    } else if (before->time() < time && after->time() > time) {
      // We must interpolate between these keyframes

      if (before->type() == NodeKeyframe::kBezier && after->type() == NodeKeyframe::kBezier) {
        // Perform a cubic bezier with two control points

        double t = Bezier::CubicXtoT(time.toDouble(),
                                     before->time().toDouble(),
                                     before->time().toDouble() + before->bezier_control_out().x(),
                                     after->time().toDouble() + after->bezier_control_in().x(),
                                     after->time().toDouble());

        double y = Bezier::CubicTtoY(before->value().toDouble(),
                                     before->value().toDouble() + before->bezier_control_out().y(),
                                     after->value().toDouble() + after->bezier_control_in().y(),
                                     after->value().toDouble(),
                                     t);

        return y;

      } else if (before->type() == NodeKeyframe::kBezier || after->type() == NodeKeyframe::kBezier) {
        // Perform a quadratic bezier with only one control point

        QPointF control_point;
        double control_point_time;
        double control_point_value;

        if (before->type() == NodeKeyframe::kBezier) {
          control_point = before->bezier_control_out();
          control_point_time = before->time().toDouble() + control_point.x();
          control_point_value = before->value().toDouble() + control_point.y();
        } else {
          control_point = after->bezier_control_in();
          control_point_time = after->time().toDouble() + control_point.x();
          control_point_value = after->value().toDouble() + control_point.y();
        }

        // Generate T from time values - used to determine bezier progress
        double t = Bezier::QuadraticXtoT(time.toDouble(), before->time().toDouble(), control_point_time, after->time().toDouble());

        // Generate value using T
        double y = Bezier::QuadraticTtoY(before->value().toDouble(), control_point_value, after->value().toDouble(), t);

        return y;

      } else {
        // To have arrived here, the keyframes must both be linear
        qreal period_progress = (time.toDouble() - before->time().toDouble()) / (after->time().toDouble() - before->time().toDouble());

        QVariant interpolated_value;

        switch (data_type()) {
        case kFloat:
          interpolated_value = lerp(before->value().toDouble(), after->value().toDouble(), period_progress);
          break;
        case kVec2:
          interpolated_value = lerp(before->value().value<QVector2D>(), after->value().value<QVector2D>(), static_cast<float>(period_progress));
          break;
        case kVec3:
          interpolated_value = lerp(before->value().value<QVector3D>(), after->value().value<QVector3D>(), static_cast<float>(period_progress));
          break;
        case kVec4:
          interpolated_value = lerp(before->value().value<QVector4D>(), after->value().value<QVector4D>(), static_cast<float>(period_progress));
          break;
        default:
          interpolated_value = before->value();
        }

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
  connect(key.get(), &NodeKeyframe::TypeChanged, this, &NodeInput::KeyframeTypeChanged);
  connect(key.get(), &NodeKeyframe::BezierControlInChanged, this, &NodeInput::KeyframeBezierInChanged);
  connect(key.get(), &NodeKeyframe::BezierControlOutChanged, this, &NodeInput::KeyframeBezierOutChanged);

  emit KeyframeAdded(key);

  emit_range_affected_by_keyframe(key.get());
}

void NodeInput::remove_keyframe(NodeKeyframePtr key)
{
  Q_ASSERT(is_keyframable() && keyframes_.size() > 1);

  TimeRange time_affected = get_range_affected_by_keyframe(key.get());

  disconnect(key.get(), &NodeKeyframe::TimeChanged, this, &NodeInput::KeyframeTimeChanged);
  disconnect(key.get(), &NodeKeyframe::ValueChanged, this, &NodeInput::KeyframeValueChanged);
  disconnect(key.get(), &NodeKeyframe::TypeChanged, this, &NodeInput::KeyframeTypeChanged);
  disconnect(key.get(), &NodeKeyframe::BezierControlInChanged, this, &NodeInput::KeyframeBezierInChanged);
  disconnect(key.get(), &NodeKeyframe::BezierControlOutChanged, this, &NodeInput::KeyframeBezierOutChanged);

  keyframes_.removeOne(key);

  emit KeyframeRemoved(key);
  emit_time_range(time_affected);
}

void NodeInput::KeyframeTimeChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  Q_ASSERT(keyframe_index > -1);

  TimeRange original_range = get_range_around_index(keyframe_index);

  if (!(original_range.in() < key->time() && original_range.out() > key->time())) {
    // This keyframe needs resorting, store it and remove it from the list
    NodeKeyframePtr key_shared_ptr = keyframes_.at(keyframe_index);

    keyframes_.removeAt(keyframe_index);

    // Automatically insertion sort
    insert_keyframe_internal(key_shared_ptr);

    // Invalidate new area that the keyframe has been moved to
    emit_time_range(get_range_around_index(FindIndexOfKeyframeFromRawPtr(key)));
  }

  // Invalidate entire area surrounding the keyframe (either where it currently is, or where it used to be before it
  // was resorted in the if block above)
  emit_time_range(original_range);
}

void NodeInput::KeyframeValueChanged()
{
  emit_range_affected_by_keyframe(static_cast<NodeKeyframe*>(sender()));
}

void NodeInput::KeyframeTypeChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  if (keyframes_.size() <= 1) {
    // If there are no other frames, the interpolation won't do anything
    return;
  }

  // Invalidate entire range
  emit_time_range(get_range_around_index(keyframe_index));
}

void NodeInput::KeyframeBezierInChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  rational start = RATIONAL_MIN;
  rational end = key->time();

  if (keyframe_index > 0) {
    start = keyframes_.at(keyframe_index - 1)->time();
  }

  emit ValueChanged(start, end);
}

void NodeInput::KeyframeBezierOutChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  rational start = key->time();
  rational end = RATIONAL_MAX;

  if (keyframe_index < keyframes_.size() - 1) {
    end = keyframes_.at(keyframe_index + 1)->time();
  }

  emit ValueChanged(start, end);
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

TimeRange NodeInput::get_range_affected_by_keyframe(NodeKeyframe *key) const
{
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  TimeRange range = get_range_around_index(keyframe_index);

  // If a previous key exists and it's a hold, we don't need to invalidate those frames
  if (keyframes().size() > 1
      && keyframe_index > 0
      && keyframes_.at(keyframe_index - 1)->type() == NodeKeyframe::kHold) {
    range.set_in(key->time());
  }

  return range;
}

TimeRange NodeInput::get_range_around_index(int index) const
{
  rational range_begin = RATIONAL_MIN;
  rational range_end = RATIONAL_MAX;

  if (keyframes_.size() > 1) {
    if (index > 0) {
      // If this is not the first key, we'll need to limit it to the key just before
      range_begin = keyframes_.at(index - 1)->time();
    }
    if (index < keyframes_.size() - 1) {
      // If this is not the last key, we'll need to limit it to the key just after
      range_end = keyframes_.at(index + 1)->time();
    }
  }

  return TimeRange(range_begin, range_end);
}

void NodeInput::emit_time_range(const TimeRange &range)
{
  emit ValueChanged(range.in(), range.out());
}

void NodeInput::emit_range_affected_by_keyframe(NodeKeyframe *key)
{
  emit_time_range(get_range_affected_by_keyframe(key));
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
    dest->keyframes_.append(key->copy());
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
