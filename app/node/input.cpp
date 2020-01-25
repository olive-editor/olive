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
#include "common/xmlreadloop.h"
#include "node.h"
#include "output.h"
#include "inputarray.h"
#include "project/item/footage/stream.h"

NodeInput::NodeInput(const QString& id, const DataType &type, const QVariant &default_value) :
  NodeParam(id),
  data_type_(type),
  keyframable_(true),
  keyframing_(false),
  has_minimum_(false),
  has_maximum_(false)
{
  int track_size;

  switch (data_type_) {
  case kVec2:
    track_size = 2;
    break;
  case kVec3:
    track_size = 3;
    break;
  case kVec4:
    track_size = 4;
    break;
  default:
    track_size = 1;
  }

  keyframe_tracks_.resize(track_size);

  if (!default_value.isNull()) {
    standard_value_ = split_normal_value_into_track_values(default_value);

    Q_ASSERT(standard_value_.size() == track_size);
  } else {
    standard_value_.resize(track_size);
  }
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

void NodeInput::Load(QXmlStreamReader *reader, QHash<quintptr, NodeOutput*>& param_ptrs, QList<SerializedConnection> &input_connections, QList<FootageConnection>& footage_connections)
{
  XMLAttributeLoop(reader, attr) {
    if (attr.name() == "keyframing") {
      set_is_keyframing(attr.value() == "1");
    }
  }

  XMLReadLoop(reader, "input") {
    if (reader->isStartElement()) {
      if (reader->name() == "standard") {
        // Load standard value
        int val_index = 0;

        XMLReadLoop(reader, "standard") {
          if (reader->isStartElement() && reader->name() == "value") {
            reader->readNext();

            QString value_text = reader->text().toString();

            if (value_text.isEmpty()) {
              standard_value_.replace(val_index, QVariant());
            } else {
              standard_value_.replace(val_index, StringToValue(value_text, footage_connections));
            }

            val_index++;
          }
        }
      } else if (reader->name() == "keyframes") {
        int track = 0;

        XMLReadLoop(reader, "keyframes") {
          if (reader->isStartElement() && reader->name() == "track") {
            XMLReadLoop(reader, "track") {
              if (reader->name() == "key") {
                rational key_time;
                NodeKeyframe::Type key_type;
                QVariant key_value;
                QPointF key_in_handle;
                QPointF key_out_handle;

                XMLAttributeLoop(reader, attr) {
                  if (attr.name() == "time") {
                    key_time = rational::fromString(attr.value().toString());
                  } else if (attr.name() == "type") {
                    key_type = static_cast<NodeKeyframe::Type>(attr.value().toInt());
                  } else if (attr.name() == "inhandlex") {
                    key_in_handle.setX(attr.value().toDouble());
                  } else if (attr.name() == "inhandley") {
                    key_in_handle.setY(attr.value().toDouble());
                  } else if (attr.name() == "outhandlex") {
                    key_out_handle.setX(attr.value().toDouble());
                  } else if (attr.name() == "outhandley") {
                    key_out_handle.setY(attr.value().toDouble());
                  }
                }

                reader->readNext();

                key_value = StringToValue(reader->text().toString(), footage_connections);

                NodeKeyframePtr key = NodeKeyframe::Create(key_time, key_value, key_type, track);
                key->set_bezier_control_in(key_in_handle);
                key->set_bezier_control_out(key_out_handle);
                key->set_parent(this);
                keyframe_tracks_[track].append(key);
              }
            }

            track++;
          }
        }
      } else if (reader->name() == "connections") {
        XMLReadLoop(reader, "connections") {
          if (reader->isStartElement() && reader->name() == "connection") {
            reader->readNext();

            input_connections.append({this, reader->text().toULongLong()});
          }
        }
      } else {
        LoadInternal(reader, param_ptrs, input_connections, footage_connections);
      }
    }
  }
}

void NodeInput::Save(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("input");

  writer->writeAttribute("id", id());

  writer->writeAttribute("keyframing", QString::number(keyframing_));

  // Write standard value
  writer->writeStartElement("standard");

  foreach (const QVariant& v, standard_value_) {
    writer->writeTextElement("value", ValueToString(v));
  }

  writer->writeEndElement(); // standard

  // Write keyframes
  writer->writeStartElement("keyframes");

  foreach (const KeyframeTrack& track, keyframe_tracks()) {
    writer->writeStartElement("track");

    foreach (NodeKeyframePtr key, track) {
      writer->writeStartElement("key");

      writer->writeAttribute("time", key->time().toString());
      writer->writeAttribute("type", QString::number(key->type()));
      writer->writeAttribute("inhandlex", QString::number(key->bezier_control_in().x()));
      writer->writeAttribute("inhandley", QString::number(key->bezier_control_in().y()));
      writer->writeAttribute("outhandlex", QString::number(key->bezier_control_out().x()));
      writer->writeAttribute("outhandley", QString::number(key->bezier_control_out().y()));

      writer->writeCharacters(ValueToString(key->value()));

      writer->writeEndElement(); // key
    }

    writer->writeEndElement(); // track
  }

  writer->writeEndElement(); // keyframes

  SaveConnections(writer);

  SaveInternal(writer);

  writer->writeEndElement(); // input
}

void NodeInput::SaveConnections(QXmlStreamWriter *writer) const
{
  writer->writeStartElement("connections");

  foreach (NodeEdgePtr edge, edges_) {
    writer->writeTextElement("connection",
                             QString::number(reinterpret_cast<quintptr>(edge->output())));
  }

  writer->writeEndElement(); // connections
}


const NodeParam::DataType &NodeInput::data_type() const
{
  return data_type_;
}

void NodeInput::LoadInternal(QXmlStreamReader*, QHash<quintptr, NodeOutput *>&, QList<SerializedConnection>&, QList<FootageConnection>&)
{
}

void NodeInput::SaveInternal(QXmlStreamWriter*) const
{
}

QString NodeInput::ValueToString(const QVariant &value) const
{
  switch (data_type_) {
  case kRational:
    return value.value<rational>().toString();
  case kFootage:
    return QString::number(reinterpret_cast<quintptr>(value.value<StreamPtr>().get()));
  default:
    if (value.canConvert<QString>()) {
      return value.toString();
    }

    qWarning() << "Failed to convert type" << data_type_ << "to string";
    return QString();
  }
}

QVariant NodeInput::StringToValue(const QString &string, QList<NodeInput::FootageConnection>& footage_connections)
{
  switch (data_type_) {
  case kRational:
    return QVariant::fromValue(rational::fromString(string));
  case kFootage:
    footage_connections.append({this, string.toULongLong()});
    /* fall through */
  default:
    return string;
  }
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

  if (output) {
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
  return combine_track_values_into_normal_value(get_split_values_at_time(time));
}

QVector<QVariant> NodeInput::get_split_values_at_time(const rational &time) const
{
  QVector<QVariant> vals;

  for (int i=0;i<get_number_of_keyframe_tracks();i++) {
    if (is_using_standard_value(i)) {
      vals.append(standard_value_.at(i));
    } else {
      vals.append(get_value_at_time_for_track(time, i));
    }
  }

  return vals;
}

QVariant NodeInput::get_value_at_time_for_track(const rational& time, int track) const
{
  if (!is_using_standard_value(track)) {
    const KeyframeTrack& key_track = keyframe_tracks_.at(track);

    if (key_track.first()->time() >= time) {
      // This time precedes any keyframe, so we just return the first value
      return key_track.first()->value();
    }

    if (key_track.last()->time() <= time) {
      // This time is after any keyframes so we return the last value
      return key_track.last()->value();
    }

    // If we're here, the time must be somewhere in between the keyframes
    for (int i=0;i<key_track.size()-1;i++) {
      NodeKeyframePtr before = key_track.at(i);
      NodeKeyframePtr after = key_track.at(i+1);

      if (before->time() == time
          || !type_can_be_interpolated(data_type())
          || (before->time() < time && before->type() == NodeKeyframe::kHold)) {

        // Time == keyframe time, so value is precise
        return before->value();

      } else if (after->time() == time) {

        // Time == keyframe time, so value is precise
        return after->value();

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

          return lerp(before->value().toDouble(), after->value().toDouble(), period_progress);
        }
      }
    }
  }

  return standard_value_.at(track);
}

QList<NodeKeyframePtr> NodeInput::get_keyframe_at_time(const rational &time) const
{
  QList<NodeKeyframePtr> keys;

  for (int i=0;i<keyframe_tracks_.size();i++) {
    NodeKeyframePtr key_at_time = get_keyframe_at_time_on_track(time, i);

    if (key_at_time) {
      keys.append(key_at_time);
    }
  }

  return keys;
}

NodeKeyframePtr NodeInput::get_keyframe_at_time_on_track(const rational &time, int track) const
{
  if (!is_using_standard_value(track)) {
    foreach (NodeKeyframePtr key, keyframe_tracks_.at(track)) {
      if (key->time() == time) {
        return key;
      }
    }
  }

  return nullptr;
}

NodeKeyframePtr NodeInput::get_closest_keyframe_to_time_on_track(const rational &time, int track) const
{
  if (is_using_standard_value(track)) {
    return nullptr;
  }

  const KeyframeTrack& key_track = keyframe_tracks_.at(track);

  if (time <= key_track.first()->time()) {
    return key_track.first();
  }

  if (time >= key_track.last()->time()) {
    return key_track.last();
  }

  for (int i=1;i<key_track.size();i++) {
    NodeKeyframePtr prev_key = key_track.at(i-1);
    NodeKeyframePtr next_key = key_track.at(i);

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

NodeKeyframePtr NodeInput::get_closest_keyframe_before_time(const rational &time) const
{
  NodeKeyframePtr key = nullptr;

  foreach (const KeyframeTrack& track, keyframe_tracks_) {
    foreach (NodeKeyframePtr k, track) {
      if (k->time() >= time) {
        break;
      } else if (!key || k->time() > key->time()) {
        key = k;
      }
    }
  }

  return key;
}

NodeKeyframePtr NodeInput::get_closest_keyframe_after_time(const rational &time) const
{
  NodeKeyframePtr key = nullptr;

  foreach (const KeyframeTrack& track, keyframe_tracks_) {
    for (int i=track.size()-1;i>=0;i--) {
      NodeKeyframePtr k = track.at(i);

      if (k->time() <= time) {
        break;
      } else if (!key || k->time() < key->time()) {
        key = k;
      }
    }
  }

  return key;
}

NodeKeyframe::Type NodeInput::get_best_keyframe_type_for_time(const rational &time, int track) const
{
  NodeKeyframePtr closest_key = get_closest_keyframe_to_time_on_track(time, track);

  if (closest_key) {
    return closest_key->type();
  }

  return NodeKeyframe::kDefaultType;
}

int NodeInput::get_number_of_keyframe_tracks() const
{
  return keyframe_tracks_.size();
}

NodeKeyframePtr NodeInput::get_earliest_keyframe() const
{
  NodeKeyframePtr earliest = nullptr;

  foreach (const KeyframeTrack& track, keyframe_tracks_) {
    if (!track.isEmpty()) {
      NodeKeyframePtr earliest_in_track = track.first();

      if (!earliest
          || earliest_in_track->time() < earliest->time()) {
        earliest = earliest_in_track;
      }
    }
  }

  return earliest;
}

NodeKeyframePtr NodeInput::get_latest_keyframe() const
{
  NodeKeyframePtr latest = nullptr;

  foreach (const KeyframeTrack& track, keyframe_tracks_) {
    if (!track.isEmpty()) {
      NodeKeyframePtr latest_in_track = track.last();

      if (!latest
          || latest_in_track->time() > latest->time()) {
        latest = latest_in_track;
      }
    }
  }

  return latest;
}

void NodeInput::insert_keyframe(NodeKeyframePtr key)
{
  Q_ASSERT(is_keyframable());

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
  Q_ASSERT(is_keyframable());

  TimeRange time_affected = get_range_affected_by_keyframe(key.get());

  disconnect(key.get(), &NodeKeyframe::TimeChanged, this, &NodeInput::KeyframeTimeChanged);
  disconnect(key.get(), &NodeKeyframe::ValueChanged, this, &NodeInput::KeyframeValueChanged);
  disconnect(key.get(), &NodeKeyframe::TypeChanged, this, &NodeInput::KeyframeTypeChanged);
  disconnect(key.get(), &NodeKeyframe::BezierControlInChanged, this, &NodeInput::KeyframeBezierInChanged);
  disconnect(key.get(), &NodeKeyframe::BezierControlOutChanged, this, &NodeInput::KeyframeBezierOutChanged);

  keyframe_tracks_[key->track()].removeOne(key);
  key->set_parent(nullptr);

  emit KeyframeRemoved(key);
  emit_time_range(time_affected);
}

void NodeInput::KeyframeTimeChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  Q_ASSERT(keyframe_index > -1);

  TimeRange original_range = get_range_around_index(keyframe_index, key->track());

  if (!(original_range.in() < key->time() && original_range.out() > key->time())) {
    // This keyframe needs resorting, store it and remove it from the list
    NodeKeyframePtr key_shared_ptr = keyframe_tracks_.at(key->track()).at(keyframe_index);

    keyframe_tracks_[key->track()].removeAt(keyframe_index);

    // Automatically insertion sort
    insert_keyframe_internal(key_shared_ptr);

    // Invalidate new area that the keyframe has been moved to
    emit_time_range(get_range_around_index(FindIndexOfKeyframeFromRawPtr(key), key->track()));
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

  if (keyframe_tracks_.at(key->track()).size() == 1) {
    // If there are no other frames, the interpolation won't do anything
    return;
  }

  // Invalidate entire range
  emit_time_range(get_range_around_index(keyframe_index, key->track()));
}

void NodeInput::KeyframeBezierInChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  rational start = RATIONAL_MIN;
  rational end = key->time();

  if (keyframe_index > 0) {
    start = keyframe_tracks_.at(key->track()).at(keyframe_index - 1)->time();
  }

  emit ValueChanged(start, end);
}

void NodeInput::KeyframeBezierOutChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  rational start = key->time();
  rational end = RATIONAL_MAX;

  if (keyframe_index < keyframe_tracks_.at(key->track()).size() - 1) {
    end = keyframe_tracks_.at(key->track()).at(keyframe_index + 1)->time();
  }

  emit ValueChanged(start, end);
}

int NodeInput::FindIndexOfKeyframeFromRawPtr(NodeKeyframe *raw_ptr) const
{
  const KeyframeTrack& track = keyframe_tracks_.at(raw_ptr->track());

  for (int i=0;i<track.size();i++) {
    if (track.at(i).get() == raw_ptr) {
      return i;
    }
  }

  return -1;
}

void NodeInput::insert_keyframe_internal(NodeKeyframePtr key)
{
  KeyframeTrack& key_track = keyframe_tracks_[key->track()];

  key->set_parent(this);

  for (int i=0;i<key_track.size();i++) {
    NodeKeyframePtr compare = key_track.at(i);

    // Ensure we aren't trying to insert two keyframes at the same time
    Q_ASSERT(compare->time() != key->time());

    if (compare->time() > key->time()) {
      key_track.insert(i, key);
      return;
    }
  }

  key_track.append(key);
}

bool NodeInput::is_using_standard_value(int track) const
{
  return (!is_keyframing() || keyframe_tracks_.at(track).isEmpty());
}

TimeRange NodeInput::get_range_affected_by_keyframe(NodeKeyframe *key) const
{
  int keyframe_index = FindIndexOfKeyframeFromRawPtr(key);

  TimeRange range = get_range_around_index(keyframe_index, key->track());

  const KeyframeTrack& key_track = keyframe_tracks_.at(key->track());

  // If a previous key exists and it's a hold, we don't need to invalidate those frames
  if (key_track.size() > 1
      && keyframe_index > 0
      && key_track.at(keyframe_index - 1)->type() == NodeKeyframe::kHold) {
    range.set_in(key->time());
  }

  return range;
}

TimeRange NodeInput::get_range_around_index(int index, int track) const
{
  rational range_begin = RATIONAL_MIN;
  rational range_end = RATIONAL_MAX;

  const KeyframeTrack& key_track = keyframe_tracks_.at(track);

  if (key_track.size() > 1) {
    if (index > 0) {
      // If this is not the first key, we'll need to limit it to the key just before
      range_begin = key_track.at(index - 1)->time();
    }
    if (index < key_track.size() - 1) {
      // If this is not the last key, we'll need to limit it to the key just after
      range_end = key_track.at(index + 1)->time();
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
  if (!is_keyframing()) {
    return false;
  }

  // Loop through keyframes to see if any match
  foreach (const KeyframeTrack& track, keyframe_tracks_) {
    foreach (NodeKeyframePtr key, track) {
      if (key->time() == time) {
        return true;
      }
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

QVariant NodeInput::get_standard_value() const
{
  return combine_track_values_into_normal_value(standard_value_);
}

const QVector<QVariant> &NodeInput::get_split_standard_value() const
{
  return standard_value_;
}

void NodeInput::set_standard_value(const QVariant &value, int track)
{
  standard_value_.replace(track, value);

  if (is_using_standard_value(track)) {
    // If this standard value is being used, we need to send a value changed signal
    emit ValueChanged(RATIONAL_MIN, RATIONAL_MAX);
  }
}

const QVector<NodeInput::KeyframeTrack> &NodeInput::keyframe_tracks() const
{
  return keyframe_tracks_;
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
  for (int i=0;i<source->keyframe_tracks_.size();i++) {
    dest->keyframe_tracks_[i].clear();
    foreach (NodeKeyframePtr key, source->keyframe_tracks_.at(i)) {
      dest->keyframe_tracks_[i].append(key->copy());
    }
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

QVector<QVariant> NodeInput::split_normal_value_into_track_values(const QVariant &value) const
{
  QVector<QVariant> vals(get_number_of_keyframe_tracks());

  switch (data_type_) {
  case kVec2:
  {
    QVector2D vec = value.value<QVector2D>();
    vals.replace(0, vec.x());
    vals.replace(1, vec.y());
    break;
  }
  case kVec3:
  {
    QVector3D vec = value.value<QVector3D>();
    vals.replace(0, vec.x());
    vals.replace(1, vec.y());
    vals.replace(2, vec.z());
    break;
  }
  case kVec4:
  {
    QVector4D vec = value.value<QVector4D>();
    vals.replace(0, vec.x());
    vals.replace(1, vec.y());
    vals.replace(2, vec.z());
    vals.replace(3, vec.w());
    break;
  }
  default:
    vals.replace(0, value);
  }

  return vals;
}

QVariant NodeInput::combine_track_values_into_normal_value(const QVector<QVariant> &split) const
{
  switch (data_type_) {
  case kVec2:
  {
    return QVector2D(split.at(0).toFloat(),
                     split.at(1).toFloat());
  }
  case kVec3:
  {
    return QVector3D(split.at(0).toFloat(),
                     split.at(1).toFloat(),
                     split.at(2).toFloat());
  }
  case kVec4:
  {
    return QVector4D(split.at(0).toFloat(),
                     split.at(1).toFloat(),
                     split.at(2).toFloat(),
                     split.at(3).toFloat());
  }
  default:
    return split.first();
  }
}
