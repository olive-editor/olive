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

#include "input.h"

#include "common/bezier.h"
#include "common/lerp.h"
#include "common/xmlutils.h"
#include "node.h"
#include "project/item/footage/stream.h"

#define super NodeConnectable

namespace olive {

NodeInput::NodeInput(Node* parent, const QString& id, NodeValue::Type type, const QVector<QVariant> &default_value)
{
  Init(parent, id, type, default_value);
}

NodeInput::NodeInput(Node *parent, const QString &id, NodeValue::Type type, const QVariant &default_value)
{
  Init(parent, id, type, NodeValue::split_normal_value_into_track_values(type, default_value));
}

NodeInput::NodeInput(Node* parent, const QString &id, NodeValue::Type type)
{
  Init(parent, id, type, QVector<QVariant>());
}

NodeInput::~NodeInput()
{
  // Disconnect everything
  DisconnectAll();

  // Delete instances
  delete primary_;
  qDeleteAll(subinputs_);
}

QString NodeInput::name() const
{
  if (name_.isEmpty()) {
    return tr("Input");
  }

  return name_;
}

void NodeInput::DisconnectAll()
{
  std::map<int, Node*> copied_edges = edges();

  for (auto it=copied_edges.cbegin(); it!=copied_edges.cend(); it++) {
    DisconnectEdge(it->second, this, it->first);
  }
}

void NodeInput::Load(QXmlStreamReader *reader, XMLNodeData &xml_node_data, const QAtomicInt *cancelled)
{
  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("primary")) {
      // Load primary immediate
      LoadImmediate(reader, -1, xml_node_data, cancelled);
    } else if (reader->name() == QStringLiteral("subelements")) {
      // Load subelements
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("count")) {
          ArrayResize(attr.value().toInt());
        }
      }

      int element_counter = 0;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("element")) {
          LoadImmediate(reader, element_counter, xml_node_data, cancelled);

          element_counter++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("connections")) {
      // Load connections
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("connection")) {
          int ele = -1;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("element")) {
              ele = attr.value().toInt();
            }
          }

          xml_node_data.desired_connections.append({this, ele, reader->readElementText().toULongLong()});
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void NodeInput::Save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("id"), id());

  writer->writeStartElement(QStringLiteral("primary"));

  SaveImmediate(writer, -1);

  writer->writeEndElement(); // primary

  writer->writeStartElement(QStringLiteral("subelements"));

  writer->writeAttribute(QStringLiteral("count"), QString::number(array_size_));

  for (int i=0; i<array_size_; i++) {
    writer->writeStartElement(QStringLiteral("element"));

    SaveImmediate(writer, i);

    writer->writeEndElement(); // element
  }

  writer->writeEndElement(); // subelements

  writer->writeStartElement(QStringLiteral("connections"));

  for (auto it=input_connections().cbegin(); it!=input_connections().cend(); it++) {
    writer->writeStartElement(QStringLiteral("connection"));

    writer->writeAttribute(QStringLiteral("element"), QString::number(it->first));

    writer->writeCharacters(QString::number(reinterpret_cast<quintptr>(it->second)));

    writer->writeEndElement(); // connection
  }

  writer->writeEndElement(); // connections
}

Node *NodeInput::parent() const
{
  return static_cast<Node*>(QObject::parent());
}

bool NodeInput::event(QEvent *e)
{
  if (e->type() == QEvent::DynamicPropertyChange) {
    QByteArray key = static_cast<QDynamicPropertyChangeEvent*>(e)->propertyName();
    emit PropertyChanged(key, property(key));
    return true;
  }

  return QObject::event(e);
}

void NodeInput::childEvent(QChildEvent *event)
{
  super::childEvent(event);

  NodeKeyframe* key = dynamic_cast<NodeKeyframe*>(event->child());

  if (key) {
    if (event->type() == QEvent::ChildAdded) {
      GetImmediate(key->element())->insert_keyframe(key);

      connect(key, &NodeKeyframe::TimeChanged, this, &NodeInput::InvalidateFromKeyframeTimeChange);
      connect(key, &NodeKeyframe::TimeChanged, this, &NodeInput::KeyframeTimeChanged);
      connect(key, &NodeKeyframe::ValueChanged, this, &NodeInput::InvalidateFromKeyframeValueChange);
      connect(key, &NodeKeyframe::TypeChanged, this, &NodeInput::InvalidateFromKeyframeTypeChanged);
      connect(key, &NodeKeyframe::BezierControlInChanged, this, &NodeInput::InvalidateFromKeyframeBezierInChange);
      connect(key, &NodeKeyframe::BezierControlOutChanged, this, &NodeInput::InvalidateFromKeyframeBezierOutChange);

      emit KeyframeAdded(key);
      emit ValueChanged(get_range_affected_by_keyframe(key), key->element());
    } else if (event->type() == QEvent::ChildRemoved) {
      TimeRange time_affected = get_range_affected_by_keyframe(key);

      disconnect(key, &NodeKeyframe::TimeChanged, this, &NodeInput::InvalidateFromKeyframeTimeChange);
      disconnect(key, &NodeKeyframe::TimeChanged, this, &NodeInput::KeyframeTimeChanged);
      disconnect(key, &NodeKeyframe::ValueChanged, this, &NodeInput::InvalidateFromKeyframeValueChange);
      disconnect(key, &NodeKeyframe::TypeChanged, this, &NodeInput::InvalidateFromKeyframeTypeChanged);
      disconnect(key, &NodeKeyframe::BezierControlInChanged, this, &NodeInput::InvalidateFromKeyframeBezierInChange);
      disconnect(key, &NodeKeyframe::BezierControlOutChanged, this, &NodeInput::InvalidateFromKeyframeBezierOutChange);

      GetImmediate(key->element())->remove_keyframe(key);

      emit KeyframeRemoved(key);
      emit ValueChanged(time_affected, key->element());
    }
  }
}

void NodeInput::Init(Node* parent, const QString &id, NodeValue::Type type, const QVector<QVariant>& default_val)
{
  setParent(parent);

  id_ = id;
  keyframable_ = true;
  connectable_ = true;
  default_value_ = default_val;
  array_size_ = 0;
  data_type_ = type;
  is_array_ = false;

  primary_ = CreateImmediate();
}

void NodeInput::LoadImmediate(QXmlStreamReader *reader, int element, XMLNodeData &xml_node_data, const QAtomicInt *cancelled)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("standard")) {
      // Load standard value
      int val_index = 0;

      while (XMLReadNextStartElement(reader)) {
        if (cancelled && *cancelled) {
          return;
        }

        if (reader->name() == QStringLiteral("track")) {
          QString value_text = reader->readElementText();
          QVariant value_on_track;

          if (!value_text.isEmpty()) {
            value_on_track = StringToValue(value_text, xml_node_data.footage_connections, element);
          }

          SetStandardValueOnTrack(value_on_track, val_index, element);

          val_index++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("keyframing") && IsKeyframable()) {
      SetIsKeyframing(reader->readElementText().toInt(), element);
    } else if (reader->name() == QStringLiteral("keyframes")) {
      int track = 0;

      while (XMLReadNextStartElement(reader)) {
        if (cancelled && *cancelled) {
          return;
        }

        if (reader->name() == QStringLiteral("track")) {
          while (XMLReadNextStartElement(reader)) {
            if (cancelled && *cancelled) {
              return;
            }

            if (reader->name() == QStringLiteral("key")) {
              rational key_time;
              NodeKeyframe::Type key_type;
              QVariant key_value;
              QPointF key_in_handle;
              QPointF key_out_handle;

              XMLAttributeLoop(reader, attr) {
                if (cancelled && *cancelled) {
                  return;
                }

                if (attr.name() == QStringLiteral("time")) {
                  key_time = rational::fromString(attr.value().toString());
                } else if (attr.name() == QStringLiteral("type")) {
                  key_type = static_cast<NodeKeyframe::Type>(attr.value().toInt());
                } else if (attr.name() == QStringLiteral("inhandlex")) {
                  key_in_handle.setX(attr.value().toDouble());
                } else if (attr.name() == QStringLiteral("inhandley")) {
                  key_in_handle.setY(attr.value().toDouble());
                } else if (attr.name() == QStringLiteral("outhandlex")) {
                  key_out_handle.setX(attr.value().toDouble());
                } else if (attr.name() == QStringLiteral("outhandley")) {
                  key_out_handle.setY(attr.value().toDouble());
                }
              }

              key_value = StringToValue(reader->readElementText(), xml_node_data.footage_connections, element);

              NodeKeyframe* key = new NodeKeyframe(key_time, key_value, key_type, track, element, this);
              key->set_bezier_control_in(key_in_handle);
              key->set_bezier_control_out(key_out_handle);
            } else {
              reader->skipCurrentElement();
            }
          }

          track++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("csinput")) {
      setProperty("col_input", reader->readElementText());
    } else if (reader->name() == QStringLiteral("csdisplay")) {
      setProperty("col_display", reader->readElementText());
    } else if (reader->name() == QStringLiteral("csview")) {
      setProperty("col_view", reader->readElementText());
    } else if (reader->name() == QStringLiteral("cslook")) {
      setProperty("col_look", reader->readElementText());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void NodeInput::SaveImmediate(QXmlStreamWriter* writer, int element) const
{
  if (IsKeyframable()) {
    writer->writeTextElement(QStringLiteral("keyframing"), QString::number(IsKeyframing(element)));
  }

  // Write standard value
  writer->writeStartElement("standard");

  foreach (const QVariant& v, GetSplitStandardValue(element)) {
    writer->writeTextElement("track", ValueToString(v));
  }

  writer->writeEndElement(); // standard

  // Write keyframes
  writer->writeStartElement("keyframes");

  foreach (const NodeKeyframeTrack& track, GetKeyframeTracks(element)) {
    writer->writeStartElement("track");

    foreach (NodeKeyframe* key, track) {
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

  if (data_type_ == NodeValue::kColor) {
    // Save color management information
    writer->writeTextElement(QStringLiteral("csinput"), property("col_input").toString());
    writer->writeTextElement(QStringLiteral("csdisplay"), property("col_display").toString());
    writer->writeTextElement(QStringLiteral("csview"), property("col_view").toString());
    writer->writeTextElement(QStringLiteral("cslook"), property("col_look").toString());
  }
}

void NodeInput::ChangeArraySizeInternal(int size)
{
  array_size_ = size;
  emit ArraySizeChanged(array_size_);
  emit ValueChanged(TimeRange(RATIONAL_MIN, RATIONAL_MAX), -1);
}

NodeInputImmediate *NodeInput::CreateImmediate()
{
  return new NodeInputImmediate(data_type_, default_value_);
}

void NodeInput::GetDependencies(QVector<Node *> &list, bool traverse, bool exclusive_only) const
{
  for (auto it=input_connections().cbegin(); it!=input_connections().cend(); it++) {
    Node* connected = it->second;

    if (connected->edges().size() == 1 || !exclusive_only) {
      if (!list.contains(connected)) {
        list.append(connected);

        if (traverse) {
          foreach (NodeInput* i, connected->parameters()) {
            i->GetDependencies(list, traverse, exclusive_only);
          }
        }
      }
    }
  }
}

QVariant NodeInput::GetDefaultValueForTrack(int track) const
{
  if (default_value_.isEmpty()) {
    return QVariant();
  }

  return default_value_.at(track);
}

QVector<Node *> NodeInput::GetDependencies(bool traverse, bool exclusive_only) const
{
  QVector<Node *> list;

  GetDependencies(list, traverse, exclusive_only);

  return list;
}

QVector<Node *> NodeInput::GetExclusiveDependencies() const
{
  return GetDependencies(true, true);
}

QVector<Node *> NodeInput::GetImmediateDependencies() const
{
  return GetDependencies(false, false);
}

void NodeInput::ArrayInsert(int index)
{
  // Add new input
  subinputs_.insert(index, CreateImmediate());

  ChangeArraySizeInternal(array_size_ + 1);

  // Move connections down
  std::map<int, Node*> copied_edges = edges();
  for (auto it=copied_edges.crbegin(); it!=copied_edges.crend(); it++) {
    if (it->first >= index) {
      // Disconnect this and reconnect it one element down
      DisconnectEdge(it->second, this, it->first);
      ConnectEdge(it->second, this, it->first + 1);
    }
  }
}

void NodeInput::ArrayRemove(int index)
{
  // Remove input
  delete subinputs_.takeAt(index);
  ChangeArraySizeInternal(array_size_ - 1);

  // Move connections up
  std::map<int, Node*> copied_edges = edges();
  for (auto it=copied_edges.cbegin(); it!=copied_edges.cend(); it++) {
    if (it->first >= index) {
      // Disconnect this and reconnect it one element up if it's not the element being removed
      DisconnectEdge(it->second, this, it->first);

      if (it->first > index) {
        ConnectEdge(it->second, this, it->first - 1);
      }
    }
  }
}

void NodeInput::ArrayPrepend()
{
  ArrayInsert(0);
}

void NodeInput::ArrayResize(int size)
{
  if (array_size_ != size) {
    // Update array size
    if (array_size_ < size) {
      // Size is larger, create any immediates that don't exist
      for (int i=subinputs_.size(); i<size; i++) {
        subinputs_.append(CreateImmediate());
      }
    }

    ChangeArraySizeInternal(size);

    if (array_size_ > size) {
      // Decreasing in size, disconnect any extraneous edges
      for (int i=size; i<array_size_; i++) {
        try {
          DisconnectEdge(edges().at(i), this, i);
        } catch (std::out_of_range&) {}
      }

      // Note that we do not delete any immediates since the user might still want that data.
      // Therefore it's important to note that array_size_ does NOT necessarily equal subinputs_.size()
    }
  }
}

void NodeInput::ArrayRemoveLast()
{
  ArrayResize(ArraySize() - 1);
}

QVariant NodeInput::GetValueAtTime(const rational &time, int element) const
{
  return NodeValue::combine_track_values_into_normal_value(data_type_, GetSplitValuesAtTime(time, element));
}

QVector<QVariant> NodeInput::GetSplitValuesAtTime(const rational &time, int element) const
{
  QVector<QVariant> vals;

  int nb_tracks = GetNumberOfKeyframeTracks();

  for (int i=0;i<nb_tracks;i++) {
    if (IsUsingStandardValue(i, element)) {
      vals.append(GetStandardValueOnTrack(i, element));
    } else {
      vals.append(GetValueAtTimeForTrack(time, i, element));
    }
  }

  return vals;
}

QVariant NodeInput::GetValueAtTimeForTrack(const rational &time, int track, int element) const
{
  if (!IsUsingStandardValue(track, element)) {
    const NodeKeyframeTrack& key_track = GetKeyframeTracks(element).at(track);

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
      NodeKeyframe* before = key_track.at(i);
      NodeKeyframe* after = key_track.at(i+1);

      if (before->time() == time
          || !NodeValue::type_can_be_interpolated(data_type_)
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
                                       before->time().toDouble() + before->valid_bezier_control_out().x(),
                                       after->time().toDouble() + after->valid_bezier_control_in().x(),
                                       after->time().toDouble());

          double y = Bezier::CubicTtoY(before->value().toDouble(),
                                       before->value().toDouble() + before->valid_bezier_control_out().y(),
                                       after->value().toDouble() + after->valid_bezier_control_in().y(),
                                       after->value().toDouble(),
                                       t);

          return y;

        } else if (before->type() == NodeKeyframe::kBezier || after->type() == NodeKeyframe::kBezier) {
          // Perform a quadratic bezier with only one control point

          QPointF control_point;
          double control_point_time;
          double control_point_value;

          if (before->type() == NodeKeyframe::kBezier) {
            control_point = before->valid_bezier_control_out();
            control_point_time = before->time().toDouble() + control_point.x();
            control_point_value = before->value().toDouble() + control_point.y();
          } else {
            control_point = after->valid_bezier_control_in();
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

  return GetStandardValueOnTrack(track, element);
}

bool NodeInput::IsKeyframable() const
{
  return keyframable_;
}

void NodeInput::SetKeyframable(bool k)
{
  keyframable_ = k;
}

void NodeInput::CopyValues(NodeInput *source, NodeInput *dest, bool include_connections, bool traverse_arrays)
{
  Q_ASSERT(source->id() == dest->id());

  CopyValuesOfElement(source, dest, -1);

  // Copy array size
  dest->ArrayResize(source->ArraySize());

  // If enabled, copy arrays too
  if (traverse_arrays) {
    for (int i=0; i<source->ArraySize(); i++) {
      CopyValuesOfElement(source, dest, i);
    }
  }

  // Copy connections
  if (include_connections) {
    if (traverse_arrays) {
      // Copy all connections
      for (auto it=source->input_connections().cbegin(); it!=source->input_connections().cend(); it++) {
        ConnectEdge(it->second, dest, it->first);
      }
    } else {
      // Just copy the primary connection (at -1)
      if (source->IsConnected()) {
        ConnectEdge(source->GetConnectedNode(), dest);
      }
    }
  }
}

void NodeInput::CopyValuesOfElement(NodeInput *src, NodeInput *dst, int element)
{
  // Copy standard value
  dst->SetSplitStandardValue(src->GetSplitStandardValue(element), element);

  // Copy keyframes
  dst->GetImmediate(element)->delete_all_keyframes();
  foreach (const NodeKeyframeTrack& track, src->GetImmediate(element)->keyframe_tracks()) {
    foreach (NodeKeyframe* key, track) {
      key->copy(dst);
    }
  }

  // Copy keyframing state
  if (src->IsKeyframable()) {
    dst->SetIsKeyframing(src->IsKeyframing(element), element);
  }

  // If this is the root of an array, copy the array size
  if (element == -1) {
    dst->ArrayResize(src->ArraySize());
  }
}

QStringList NodeInput::get_combobox_strings() const
{
  return property("combo_str").toStringList();
}

void NodeInput::set_combobox_strings(const QStringList &strings)
{
  setProperty("combo_str", strings);
}

void NodeInput::InvalidateFromKeyframeBezierInChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  const NodeKeyframeTrack& track = GetTrackFromKeyframe(key);
  int keyframe_index = track.indexOf(key);

  rational start = RATIONAL_MIN;
  rational end = key->time();

  if (keyframe_index > 0) {
    start = track.at(keyframe_index - 1)->time();
  }

  emit ValueChanged(TimeRange(start, end), key->element());
}

void NodeInput::InvalidateFromKeyframeBezierOutChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  const NodeKeyframeTrack& track = GetTrackFromKeyframe(key);
  int keyframe_index = track.indexOf(key);

  rational start = key->time();
  rational end = RATIONAL_MAX;

  if (keyframe_index < track.size() - 1) {
    end = track.at(keyframe_index + 1)->time();
  }

  emit ValueChanged(TimeRange(start, end), key->element());
}

void NodeInput::InvalidateFromKeyframeTimeChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  NodeInputImmediate* immediate = GetImmediate(key->element());
  TimeRange original_range = get_range_affected_by_keyframe(key);

  TimeRangeList invalidate_range;
  invalidate_range.insert(original_range);

  if (!(original_range.in() < key->time() && original_range.out() > key->time())) {
    // This keyframe needs resorting, store it and remove it from the list
    immediate->remove_keyframe(key);

    // Automatically insertion sort
    immediate->insert_keyframe(key);

    // Invalidate new area that the keyframe has been moved to
    invalidate_range.insert(get_range_affected_by_keyframe(key));
  }

  // Invalidate entire area surrounding the keyframe (either where it currently is, or where it used to be before it
  // was resorted in the if block above)
  foreach (const TimeRange& r, invalidate_range) {
    emit ValueChanged(r, key->element());
  }
}

void NodeInput::InvalidateFromKeyframeValueChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  emit ValueChanged(get_range_affected_by_keyframe(key), key->element());
}

void NodeInput::InvalidateFromKeyframeTypeChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  const NodeKeyframeTrack& track = GetTrackFromKeyframe(key);

  if (track.size() == 1) {
    // If there are no other frames, the interpolation won't do anything
    return;
  }

  // Invalidate entire range
  emit ValueChanged(get_range_around_index(track.indexOf(key), key->track(), key->element()), key->element());
}

TimeRange NodeInput::get_range_affected_by_keyframe(NodeKeyframe *key) const
{
  const NodeKeyframeTrack& key_track = GetTrackFromKeyframe(key);
  int keyframe_index = key_track.indexOf(key);

  TimeRange range = get_range_around_index(keyframe_index, key->track(), key->element());

  // If a previous key exists and it's a hold, we don't need to invalidate those frames
  if (key_track.size() > 1
      && keyframe_index > 0
      && key_track.at(keyframe_index - 1)->type() == NodeKeyframe::kHold) {
    range.set_in(key->time());
  }

  return range;
}

TimeRange NodeInput::get_range_around_index(int index, int track, int element) const
{
  rational range_begin = RATIONAL_MIN;
  rational range_end = RATIONAL_MAX;

  const NodeKeyframeTrack& key_track = GetImmediate(element)->keyframe_tracks().at(track);

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

QString NodeInput::ValueToString(const QVariant &value) const
{
  return NodeValue::ValueToString(data_type_, value, true);
}

QVariant NodeInput::StringToValue(const QString &string, QList<XMLNodeData::FootageConnection>& footage_connections, int element)
{
  if (data_type_ == NodeValue::kFootage) {
    footage_connections.append({this, element, string.toULongLong()});
  }

  return NodeValue::StringToValue(data_type_, string, true);
}

uint qHash(const NodeInput::KeyframeTrackReference &ref, uint seed)
{
  return qHash(ref.input, seed) ^ qHash(ref.element, seed) ^ qHash(ref.track, seed);
}

}
