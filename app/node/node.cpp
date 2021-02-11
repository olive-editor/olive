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

#include "node.h"

#include <QApplication>
#include <QGuiApplication>
#include <QDebug>
#include <QFile>

#include "common/bezier.h"
#include "common/lerp.h"
#include "common/timecodefunctions.h"
#include "common/xmlutils.h"
#include "core.h"
#include "config/config.h"
#include "project/project.h"
#include "project/item/footage/footage.h"
#include "project/item/footage/videostream.h"
#include "ui/colorcoding.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

#define super QObject

const QString Node::kDefaultOutput = QStringLiteral("output");

Node::Node(bool create_default_output) :
  can_be_deleted_(true),
  override_color_(-1)
{
  if (create_default_output) {
    AddOutput();
  }
}

Node::~Node()
{
  // Disconnect all edges
  DisconnectAll();

  // Remove self from anything while we're still a node rather than a base QObject
  setParent(nullptr);

  // Remove all immediates
  foreach (NodeInputImmediate* i, standard_immediates_) {
    delete i;
  }
  for (auto it=array_immediates_.cbegin(); it!=array_immediates_.cend(); it++) {
    foreach (NodeInputImmediate* i, it.value()) {
      delete i;
    }
  }
}

NodeGraph *Node::parent() const
{
  return static_cast<NodeGraph*>(QObject::parent());
}

void Node::Load(QXmlStreamReader *reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled)
{
  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("input")) {
      LoadInput(reader, xml_node_data, cancelled);
    } else if (reader->name() == QStringLiteral("ptr")) {
      xml_node_data.node_ptrs.insert(reader->readElementText().toULongLong(), this);
    } else if (reader->name() == QStringLiteral("pos")) {
      QPointF p;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("x")) {
          p.setX(reader->readElementText().toDouble());
        } else if (reader->name() == QStringLiteral("y")) {
          p.setY(reader->readElementText().toDouble());
        } else {
          reader->skipCurrentElement();
        }
      }

      SetPosition(p);
    } else if (reader->name() == QStringLiteral("label")) {
      SetLabel(reader->readElementText());
    } else if (reader->name() == QStringLiteral("color")) {
      override_color_ = reader->readElementText().toInt();
    } else if (reader->name() == QStringLiteral("links")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("link")) {
          xml_node_data.block_links.append({this, reader->readElementText().toULongLong()});
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("custom")) {
      LoadInternal(reader, xml_node_data);
    } else if (reader->name() == QStringLiteral("connections")) {
      // Load connections
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("connection")) {
          QString param_id;
          int ele = -1;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("element")) {
              ele = attr.value().toInt();
            } else if (attr.name() == QStringLiteral("input")) {
              param_id = attr.value().toString();
            }
          }

          QString output_node_id;
          QString output_param_id;

          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("node")) {
              output_node_id = reader->readElementText();
            } else if (reader->name() == QStringLiteral("output")) {
              output_param_id = reader->readElementText();
            } else {
              reader->skipCurrentElement();
            }
          }

          xml_node_data.desired_connections.append({NodeInput(this, param_id, ele), output_node_id.toULongLong(), output_param_id});
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Node::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(this)));

  writer->writeStartElement(QStringLiteral("pos"));
  writer->writeTextElement(QStringLiteral("x"), QString::number(GetPosition().x()));
  writer->writeTextElement(QStringLiteral("y"), QString::number(GetPosition().y()));
  writer->writeEndElement(); // pos

  writer->writeTextElement(QStringLiteral("label"), GetLabel());
  writer->writeTextElement(QStringLiteral("color"), QString::number(override_color_));

  foreach (const QString& input, input_ids_) {
    writer->writeStartElement(QStringLiteral("input"));

    SaveInput(writer, input);

    writer->writeEndElement(); // input
  }

  writer->writeStartElement(QStringLiteral("links"));
  foreach (Node* link, links_) {
    writer->writeTextElement(QStringLiteral("link"), QString::number(reinterpret_cast<quintptr>(link)));
  }
  writer->writeEndElement(); // links

  writer->writeStartElement(QStringLiteral("connections"));
  for (auto it=input_connections().cbegin(); it!=input_connections().cend(); it++) {
    writer->writeStartElement(QStringLiteral("connection"));

    writer->writeAttribute(QStringLiteral("input"), it->first.input());
    writer->writeAttribute(QStringLiteral("element"), QString::number(it->first.element()));

    writer->writeTextElement(QStringLiteral("node"), QString::number(reinterpret_cast<quintptr>(it->second.node())));
    writer->writeTextElement(QStringLiteral("output"), it->second.output());

    writer->writeEndElement(); // connection
  }
  writer->writeEndElement(); // connections

  writer->writeStartElement(QStringLiteral("custom"));
  SaveInternal(writer);
  writer->writeEndElement(); // custom
}

QString Node::ShortName() const
{
  return Name();
}

QString Node::Description() const
{
  // Return an empty string by default
  return QString();
}

void Node::Retranslate()
{
}

Color Node::color() const
{
  int c;

  if (override_color_ >= 0) {
    c = override_color_;
  } else {
    c = Config::Current()[QStringLiteral("CatColor%1").arg(this->Category().first())].toInt();
  }

  return ColorCoding::GetColor(c);
}

QLinearGradient Node::gradient_color(qreal top, qreal bottom) const
{
  QLinearGradient grad;

  grad.setStart(0, top);
  grad.setFinalStop(0, bottom);

  QColor c = color().toQColor();

  grad.setColorAt(0.0, c.lighter());
  grad.setColorAt(1.0, c);

  return grad;
}

QBrush Node::brush(qreal top, qreal bottom) const
{
  if (Config::Current()[QStringLiteral("UseGradients")].toBool()) {
    return gradient_color(top, bottom);
  } else {
    return color().toQColor();
  }
}

void Node::ConnectEdge(const NodeOutput &output, const NodeInput &input)
{
  // Ensure parameters exist on the nodes requested
  Q_ASSERT(input.node()->HasInputWithID(input.input()));
  Q_ASSERT(output.node()->HasOutputWithID(output.output()));

  // Ensure a connection isn't getting overwritten
  Q_ASSERT(input.node()->input_connections().find(input) == input.node()->input_connections().end());

  // Insert connection on both sides
  input.node()->input_connections_[input] = output;
  output.node()->output_connections_.push_back(std::pair<NodeOutput, NodeInput>({output, input}));

  // Call internal event
  input.node()->InputConnectedEvent(input.input(), input.element(), output);

  // Emit signals
  emit input.node()->InputConnected(output, input);
  emit output.node()->OutputConnected(output, input);

  // Invalidate all if this node isn't ignoring this input
  if (!input.node()->ignore_connections_.contains(input.input())) {
    input.node()->InvalidateAll(input.input(), input.element());
  }
}

void Node::DisconnectEdge(const NodeOutput &output, const NodeInput &input)
{
  // Ensure parameters exist on the nodes requested
  Q_ASSERT(input.node()->HasInputWithID(input.input()));
  Q_ASSERT(output.node()->HasOutputWithID(output.output()));

  // Ensure connection exists
  Q_ASSERT(input.node()->input_connections().at(input) == output);

  // Remove connection from both sides
  InputConnections& inputs = input.node()->input_connections_;
  inputs.erase(inputs.find(input));

  OutputConnections& outputs = output.node()->output_connections_;
  outputs.erase(std::find(outputs.begin(), outputs.end(), std::pair<NodeOutput, NodeInput>({output, input})));

  // Call internal event
  input.node()->InputDisconnectedEvent(input.input(), input.element(), output);

  emit input.node()->InputDisconnected(output, input);
  emit output.node()->OutputDisconnected(output, input);

  if (!input.node()->ignore_connections_.contains(input.input())) {
    input.node()->InvalidateAll(input.input(), input.element());
  }
}

QString Node::GetInputName(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->human_name;
  } else {
    ReportInvalidInput("get name of", id);
    return QString();
  }
}

void Node::LoadInput(QXmlStreamReader *reader, XMLNodeData &xml_node_data, const QAtomicInt *cancelled)
{
  QString param_id;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("id")) {
      param_id = attr.value().toString();

      break;
    }
  }

  if (param_id.isEmpty()) {
    qWarning() << "Failed to load parameter with missing ID";
    return;
  }

  if (!HasInputWithID(param_id)) {
    qWarning() << "Failed to load parameter that didn't exist";
    return;
  }

  while (XMLReadNextStartElement(reader)) {
    if (cancelled && *cancelled) {
      return;
    }

    if (reader->name() == QStringLiteral("primary")) {
      // Load primary immediate
      LoadImmediate(reader, param_id, -1, xml_node_data, cancelled);
    } else if (reader->name() == QStringLiteral("subelements")) {
      // Load subelements
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("count")) {
          InputArrayResize(param_id, attr.value().toInt());
        }
      }

      int element_counter = 0;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("element")) {
          LoadImmediate(reader, param_id, element_counter, xml_node_data, cancelled);

          element_counter++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Node::SaveInput(QXmlStreamWriter *writer, const QString &id) const
{
  writer->writeAttribute(QStringLiteral("id"), id);

  writer->writeStartElement(QStringLiteral("primary"));

  SaveImmediate(writer, id, -1);

  writer->writeEndElement(); // primary

  writer->writeStartElement(QStringLiteral("subelements"));

  int arr_sz = InputArraySize(id);

  writer->writeAttribute(QStringLiteral("count"), QString::number(arr_sz));

  for (int i=0; i<arr_sz; i++) {
    writer->writeStartElement(QStringLiteral("element"));

    SaveImmediate(writer, id, i);

    writer->writeEndElement(); // element
  }

  writer->writeEndElement(); // subelements
}

bool Node::IsInputConnectable(const QString &input) const
{
  return !(GetInputFlags(input) & kInputFlagNotConnectable);
}

bool Node::IsInputKeyframable(const QString &input) const
{
  return !(GetInputFlags(input) & kInputFlagNotKeyframable);
}

bool Node::IsInputKeyframing(const QString &input, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->is_keyframing();
  } else {
    ReportInvalidInput("get keyframing state of", input);
    return false;
  }
}

void Node::SetInputIsKeyframing(const QString &input, bool e, int element)
{
  if (!IsInputKeyframable(input)) {
    qDebug() << "Ignored set keyframing of" << input << "because this input is not keyframable";
    return;
  }

  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    imm->set_is_keyframing(e);

    emit KeyframeEnableChanged(NodeInput(this, input, element), e);
  } else {
    ReportInvalidInput("set keyframing state of", input);
  }
}

bool Node::IsInputConnected(const QString &input, int element) const
{
  return GetConnectedOutput(input, element).IsValid();
}

NodeOutput Node::GetConnectedOutput(const QString &input, int element) const
{
  for (auto it=input_connections_.cbegin(); it!=input_connections_.cend(); it++) {
    if (it->first.input() == input && it->first.element() == element) {
      return it->second;
    }
  }

  return NodeOutput();
}

bool Node::IsUsingStandardValue(const QString &input, int track, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->is_using_standard_value(track);
  } else {
    ReportInvalidInput("determine whether using standard value in", input);
    return true;
  }
}

NodeValue::Type Node::GetInputDataType(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->type;
  } else {
    ReportInvalidInput("get data type of", id);
    return NodeValue::kNone;
  }
}

void Node::SetInputDataType(const QString &id, const NodeValue::Type &type)
{
  Input* i = GetInternalInputData(id);

  if (i) {
    i->type = type;

    emit InputDataTypeChanged(id, type);
  } else {
    ReportInvalidInput("set data type of", id);
  }
}

bool Node::HasInputProperty(const QString &id, const QString &name) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->properties.contains(name);
  } else {
    ReportInvalidInput("get property of", id);
    return false;
  }
}

QHash<QString, QVariant> Node::GetInputProperties(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->properties;
  } else {
    ReportInvalidInput("get property table of", id);
    return QHash<QString, QVariant>();
  }
}

QVariant Node::GetInputProperty(const QString &id, const QString &name) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->properties.value(name);
  } else {
    ReportInvalidInput("get property of", id);
    return QVariant();
  }
}

void Node::SetInputProperty(const QString &id, const QString &name, const QVariant &value)
{
  Input* i = GetInternalInputData(id);

  if (i) {
    i->properties.insert(name, value);

    emit InputPropertyChanged(id, name, value);
  } else {
    ReportInvalidInput("set property of", id);
  }
}

SplitValue Node::GetSplitValueAtTime(const QString &input, const rational &time, int element) const
{
  SplitValue vals;

  int nb_tracks = GetNumberOfKeyframeTracks(input);

  for (int i=0;i<nb_tracks;i++) {
    if (IsUsingStandardValue(input, i, element)) {
      vals.append(GetSplitStandardValueOnTrack(input, i, element));
    } else {
      vals.append(GetSplitValueAtTimeOnTrack(input, time, i, element));
    }
  }

  return vals;
}

QVariant Node::GetSplitValueAtTimeOnTrack(const QString &input, const rational &time, int track, int element) const
{
  if (!IsUsingStandardValue(input, track, element)) {
    const NodeKeyframeTrack& key_track = GetKeyframeTracks(input, element).at(track);

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
          || !NodeValue::type_can_be_interpolated(GetInputDataType(input))
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

  return GetSplitStandardValueOnTrack(input, track, element);
}

QVariant Node::GetDefaultValue(const QString &input) const
{
  NodeValue::Type type = GetInputDataType(input);

  return NodeValue::combine_track_values_into_normal_value(type, GetSplitDefaultValue(input));
}

SplitValue Node::GetSplitDefaultValue(const QString &input) const
{
  const Input* i = GetInternalInputData(input);

  if (i) {
    return i->default_value;
  } else {
    ReportInvalidInput("retrieve default value of", input);
    return SplitValue();
  }
}

QVariant Node::GetSplitDefaultValueOnTrack(const QString &input, int track) const
{
  return GetSplitDefaultValue(input).at(track);
}

const QVector<NodeKeyframeTrack> &Node::GetKeyframeTracks(const QString &input, int element) const
{
  return GetImmediate(input, element)->keyframe_tracks();
}

QVector<NodeKeyframe *> Node::GetKeyframesAtTime(const QString &input, const rational &time, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->get_keyframe_at_time(time);
  } else {
    ReportInvalidInput("get keyframes at time from", input);
    return QVector<NodeKeyframe*>();
  }
}

NodeKeyframe *Node::GetKeyframeAtTimeOnTrack(const QString &input, const rational &time, int track, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->get_keyframe_at_time_on_track(time, track);
  } else {
    ReportInvalidInput("get keyframe at time on track from", input);
    return nullptr;
  }
}

NodeKeyframe::Type Node::GetBestKeyframeTypeForTimeOnTrack(const QString &input, const rational &time, int track, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->get_best_keyframe_type_for_time(time, track);
  } else {
    ReportInvalidInput("get closest keyframe before a time from", input);
    return NodeKeyframe::kDefaultType;
  }
}

int Node::GetNumberOfKeyframeTracks(const QString &id) const
{
  return NodeValue::get_number_of_keyframe_tracks(GetInputDataType(id));
}

NodeKeyframe *Node::GetEarliestKeyframe(const QString &id, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_earliest_keyframe();
  } else {
    ReportInvalidInput("get earliest keyframe from", id);
    return nullptr;
  }
}

NodeKeyframe *Node::GetLatestKeyframe(const QString &id, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_latest_keyframe();
  } else {
    ReportInvalidInput("get latest keyframe from", id);
    return nullptr;
  }
}

NodeKeyframe *Node::GetClosestKeyframeBeforeTime(const QString &id, const rational &time, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_closest_keyframe_before_time(time);
  } else {
    ReportInvalidInput("get closest keyframe before a time from", id);
    return nullptr;
  }
}

NodeKeyframe *Node::GetClosestKeyframeAfterTime(const QString &id, const rational &time, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_closest_keyframe_after_time(time);
  } else {
    ReportInvalidInput("get closest keyframe after a time from", id);
    return nullptr;
  }
}

bool Node::HasKeyframeAtTime(const QString &id, const rational &time, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->has_keyframe_at_time(time);
  } else {
    ReportInvalidInput("determine if it has a keyframe at a time from", id);
    return false;
  }
}

QStringList Node::GetComboBoxStrings(const QString &id) const
{
  return GetInputProperty(id, QStringLiteral("combo_str")).toStringList();
}

QVariant Node::GetStandardValue(const QString &id, int element) const
{
  NodeValue::Type type = GetInputDataType(id);

  return NodeValue::combine_track_values_into_normal_value(type, GetSplitStandardValue(id, element));
}

SplitValue Node::GetSplitStandardValue(const QString &id, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_split_standard_value();
  } else {
    ReportInvalidInput("get standard value of", id);
    return SplitValue();
  }
}

QVariant Node::GetSplitStandardValueOnTrack(const QString &input, int track, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->get_split_standard_value_on_track(track);
  } else {
    ReportInvalidInput("get standard value of", input);
    return QVariant();
  }
}

void Node::SetStandardValue(const QString &id, const QVariant &value, int element)
{
  NodeValue::Type type = GetInputDataType(id);

  SetSplitStandardValue(id, NodeValue::split_normal_value_into_track_values(type, value), element);
}

void Node::SetSplitStandardValue(const QString &id, const SplitValue &value, int element)
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    imm->set_split_standard_value(value);

    for (int i=0; i<value.size(); i++) {
      if (IsUsingStandardValue(id, i, element)) {
        // If this standard value is being used, we need to send a value changed signal
        ParameterValueChanged(id, element, TimeRange(RATIONAL_MIN, RATIONAL_MAX));
        break;
      }
    }
  } else {
    ReportInvalidInput("set standard value of", id);
  }
}

void Node::SetSplitStandardValueOnTrack(const QString &id, int track, const QVariant &value, int element)
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    imm->set_standard_value_on_track(value, track);

    if (IsUsingStandardValue(id, track, element)) {
      // If this standard value is being used, we need to send a value changed signal
      ParameterValueChanged(id, element, TimeRange(RATIONAL_MIN, RATIONAL_MAX));
    }
  } else {
    ReportInvalidInput("set standard value of", id);
  }
}

bool Node::InputIsArray(const QString &id) const
{
  return GetInputFlags(id) & kInputFlagArray;
}

void Node::InputArrayInsert(const QString &id, int index, bool undoable)
{
  if (undoable) {
    Core::instance()->undo_stack()->push(new ArrayInsertCommand(this, id, index));
  } else {
    // Add new input
    ArrayResizeInternal(id, InputArraySize(id) + 1);

    // Move connections down
    InputConnections copied_edges = input_connections();
    for (auto it=copied_edges.crbegin(); it!=copied_edges.crend(); it++) {
      if (it->first.element() >= index) {
        // Disconnect this and reconnect it one element down
        NodeInput new_edge = it->first;
        new_edge.set_element(new_edge.element() + 1);

        DisconnectEdge(it->second, it->first);
        ConnectEdge(it->second, new_edge);
      }
    }

    // Shift values and keyframes up one element
    for (int i=InputArraySize(id)-1; i>index; i--) {
      CopyValuesOfElement(this, this, id, i-1, i);
    }

    // Reset value of element we just "inserted"
    ClearElement(id, index);
  }
}

void Node::InputArrayResize(const QString &id, int size, bool undoable)
{
  if (InputArraySize(id) == size) {
    return;
  }

  ArrayResizeCommand* c = new ArrayResizeCommand(this, id, size);

  if (undoable) {
    Core::instance()->undo_stack()->push(c);
  } else {
    c->redo();
    delete c;
  }
}

void Node::InputArrayRemove(const QString &id, int index, bool undoable)
{
  if (undoable) {
    Core::instance()->undo_stack()->push(new ArrayRemoveCommand(this, id, index));
  } else {
    // Remove input
    ArrayResizeInternal(id, InputArraySize(id) - 1);

    // Move connections up
    InputConnections copied_edges = input_connections();
    for (auto it=copied_edges.cbegin(); it!=copied_edges.cend(); it++) {
      if (it->first.element() >= index) {
        // Disconnect this and reconnect it one element up if it's not the element being removed
        DisconnectEdge(it->second, it->first);

        if (it->first.element() > index) {
          NodeInput new_edge = it->first;
          new_edge.set_element(new_edge.element() - 1);

          ConnectEdge(it->second, new_edge);
        }
      }
    }

    // Shift values and keyframes down one element
    int arr_sz = InputArraySize(id);
    for (int i=index; i<arr_sz; i++) {
      // Copying ArraySize()+1 is actually legal because immediates are never deleted
      CopyValuesOfElement(this, this, id, i+1, i);
    }

    // Reset value of last element
    ClearElement(id, arr_sz);
  }
}

int Node::InputArraySize(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->array_size;
  } else {
    ReportInvalidInput("retrieve array size of", id);
    return 0;
  }
}

const NodeKeyframeTrack &Node::GetTrackFromKeyframe(NodeKeyframe *key) const
{
  return GetImmediate(key->input(), key->element())->keyframe_tracks().at(key->track());
}

NodeInputImmediate *Node::GetImmediate(const QString &input, int element) const
{
  if (element == -1) {
    return standard_immediates_.value(input, nullptr);
  } else if (array_immediates_.contains(input)) {
    const QVector<NodeInputImmediate*>& imm_arr = array_immediates_.value(input);

    if (element >= 0 && element < imm_arr.size()) {
      return imm_arr.at(element);
    }
  }

  return nullptr;
}

Node::InputFlags Node::GetInputFlags(const QString &input) const
{
  const Input* i = GetInternalInputData(input);

  if (i) {
    return i->flags;
  } else {
    ReportInvalidInput("retrieve flags of", input);
    return InputFlags(kInputFlagNormal);
  }
}

NodeValueTable Node::Value(const QString& output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  return value.Merge();
}

void Node::InvalidateCache(const TimeRange &range, const QString &from, int element)
{
  Q_UNUSED(from)
  Q_UNUSED(element)

  SendInvalidateCache(range);
}

void Node::BeginOperation()
{
  // Ripple through graph
  for (const std::pair<NodeOutput, NodeInput>& output : output_connections_) {
    output.second.node()->BeginOperation();
  }
}

void Node::EndOperation()
{
  // Ripple through graph
  for (const std::pair<NodeOutput, NodeInput>& output : output_connections_) {
    output.second.node()->EndOperation();
  }
}

TimeRange Node::InputTimeAdjustment(const QString &, int, const TimeRange &input_time) const
{
  // Default behavior is no time adjustment at all
  return input_time;
}

TimeRange Node::OutputTimeAdjustment(const QString &, int, const TimeRange &input_time) const
{
  // Default behavior is no time adjustment at all
  return input_time;
}

QVector<Node *> Node::CopyDependencyGraph(const QVector<Node *> &nodes, MultiUndoCommand *command)
{
  int nb_nodes = nodes.size();

  QVector<Node*> copies(nb_nodes);

  for (int i=0; i<nb_nodes; i++) {
    // Create another of the same node
    Node* c = nodes.at(i)->copy();;

    // Copy the values, but NOT the connections, since we'll be connecting to our own clones later
    Node::CopyInputs(nodes.at(i), c, false);

    // Add to graph
    NodeGraph* graph = static_cast<NodeGraph*>(nodes.at(i)->parent());
    if (command) {
      command->add_child(new NodeAddCommand(graph, c));
    } else {
      c->setParent(graph);
    }

    // Store in array at the same index as source
    copies[i] = c;
  }

  CopyDependencyGraph(nodes, copies, command);

  return copies;
}

void Node::CopyDependencyGraph(const QVector<Node *> &src, const QVector<Node *> &dst, MultiUndoCommand *command)
{
  for (int i=0; i<src.size(); i++) {
    Node* src_node = src.at(i);
    Node* dst_node = dst.at(i);

    for (auto it=src_node->input_connections_.cbegin(); it!=src_node->input_connections_.cend(); it++) {
      // Determine if the connected node is in our src list
      int connection_index = src.indexOf(it->second.node());

      if (connection_index > -1) {
        // Find the equivalent node in the dst list
        Node* dst_connection = dst.at(connection_index);

        NodeOutput copied_output = NodeOutput(dst_connection, it->second.output());
        NodeInput copied_input = NodeInput(dst_node, it->first.input(), it->first.element());

        if (command) {
          command->add_child(new NodeEdgeAddCommand(copied_output, copied_input));
        } else {
          ConnectEdge(copied_output, copied_input);
        }
      }
    }
  }
}

void Node::SendInvalidateCache(const TimeRange &range)
{
  for (const OutputConnection& conn : output_connections_) {
    // Send clear cache signal to the Node
    const NodeInput& in = conn.second;

    in.node()->InvalidateCache(range, in.input(), in.element());
  }
}

void Node::InvalidateAll(const QString &input, int element)
{
  InvalidateCache(TimeRange(RATIONAL_MIN, RATIONAL_MAX), input, element);
}

bool Node::Link(Node *a, Node *b)
{
  if (a == b || !a || !b) {
    return false;
  }

  if (AreLinked(a, b)) {
    return false;
  }

  a->links_.append(b);
  b->links_.append(a);

  a->LinkChangeEvent();
  b->LinkChangeEvent();

  emit a->LinksChanged();
  emit b->LinksChanged();

  return true;
}

bool Node::Unlink(Node *a, Node *b)
{
  if (!AreLinked(a, b)) {
    return false;
  }

  a->links_.removeOne(b);
  b->links_.removeOne(a);

  a->LinkChangeEvent();
  b->LinkChangeEvent();

  emit a->LinksChanged();
  emit b->LinksChanged();

  return true;
}

bool Node::AreLinked(Node *a, Node *b)
{
  return a->links_.contains(b);
}

void Node::AddInput(const QString &id, NodeValue::Type type, const QVariant &default_value, Node::InputFlags flags)
{
  if (id.isEmpty()) {
    qWarning() << "Rejected adding input with an empty ID on node" << this->id();
    return;
  }

  if (HasParamWithID(id)) {
    qWarning() << "Failed to add input to node" << this->id() << "- param with ID" << id << "already exists";
    return;
  }

  Node::Input i;

  i.type = type;
  i.default_value = NodeValue::split_normal_value_into_track_values(type, default_value);
  i.flags = flags;
  i.array_size = 0;

  input_ids_.append(id);
  input_data_.append(i);

  if (!standard_immediates_.value(id, nullptr)) {
    standard_immediates_.insert(id, CreateImmediate(id));
  }

  emit InputAdded(id);
}

void Node::RemoveInput(const QString &id)
{
  int index = input_ids_.indexOf(id);

  if (index == -1) {
    ReportInvalidInput("remove", id);
    return;
  }

  input_ids_.removeAt(index);
  input_data_.removeAt(index);

  emit InputRemoved(id);
}

void Node::AddOutput(const QString &id)
{
  if (id.isEmpty()) {
    qWarning() << "Rejected adding output with an empty ID on node" << this->id();
    return;
  }

  if (HasParamWithID(id)) {
    qWarning() << "Failed to add output to node" << this->id() << "- param with ID" << id << "already exists";
    return;
  }

  outputs_.append(id);

  emit OutputAdded(id);
}

void Node::RemoveOutput(const QString &id)
{
  if (outputs_.removeOne(id)) {
    emit OutputRemoved(id);
  } else {
    ReportInvalidInput("remove", id);
  }
}

void Node::ReportInvalidInput(const char *attempted_action, const QString& id) const
{
  qWarning() << "Failed to" << attempted_action << "parameter" << id
             << "in node" << this->id() << "- input doesn't exist";
}

NodeInputImmediate *Node::CreateImmediate(const QString &input)
{
  const Input* i = GetInternalInputData(input);

  if (i) {
    return new NodeInputImmediate(i->type, i->default_value);
  } else {
    ReportInvalidInput("create immediate", input);
    return nullptr;
  }
}

void Node::ArrayResizeInternal(const QString &id, int size)
{
  Input* imm = GetInternalInputData(id);

  if (!imm) {
    ReportInvalidInput("set array size", id);
    return;
  }

  if (imm->array_size != size) {
    // Update array size
    if (imm->array_size < size) {
      // Size is larger, create any immediates that don't exist
      QVector<NodeInputImmediate*>& subinputs = array_immediates_[id];
      for (int i=subinputs.size(); i<size; i++) {
        subinputs.append(CreateImmediate(id));
      }

      // Note that we do not delete any immediates when decreasing size since the user might still
      // want that data. Therefore it's important to note that array_size_ does NOT necessarily
      // equal subinputs_.size()
    }

    imm->array_size = size;
    emit InputArraySizeChanged(id, size);
    ParameterValueChanged(id, -1, TimeRange(RATIONAL_MIN, RATIONAL_MAX));
  }
}

int Node::GetInternalInputArraySize(const QString &input)
{
  return array_immediates_.value(input).size();
}

void Node::SetInputName(const QString &id, const QString &name)
{
  Input* i = GetInternalInputData(id);

  if (i) {
    i->human_name = name;

    emit InputNameChanged(id, name);
  } else {
    ReportInvalidInput("set name of", id);
  }
}

void Node::IgnoreInvalidationsFrom(const QString& input_id)
{
  ignore_connections_.append(input_id);
}

void Node::IgnoreHashingFrom(const QString &input_id)
{
  ignore_when_hashing_.append(input_id);
}

void Node::LoadInternal(QXmlStreamReader *reader, XMLNodeData &)
{
  reader->skipCurrentElement();
}

void Node::SaveInternal(QXmlStreamWriter *) const
{
}

bool Node::HasGizmos() const
{
  return false;
}

void Node::DrawGizmos(NodeValueDatabase &, QPainter *)
{
}

bool Node::GizmoPress(NodeValueDatabase &, const QPointF &)
{
  return false;
}

void Node::GizmoMove(const QPointF &, const rational&)
{
}

void Node::GizmoRelease()
{
}

const QString &Node::GetLabel() const
{
  return label_;
}

void Node::SetLabel(const QString &s)
{
  if (label_ != s) {
    label_ = s;

    emit LabelChanged(label_);
  }
}

void Node::Hash(QCryptographicHash &hash, const rational& time) const
{
  // Add this Node's ID
  hash.addData(id().toUtf8());

  foreach (const QString& input, input_ids_) {
    // For each input, try to hash its value
    if (ignore_when_hashing_.contains(input)) {
      continue;
    }

    int arr_sz = InputArraySize(input);
    for (int i=-1; i<arr_sz; i++) {
      HashInputElement(hash, input, i, time);
    }
  }
}

void Node::CopyInputs(Node *source, Node *destination, bool include_connections)
{
  Q_ASSERT(source->id() == destination->id());

  foreach (const QString& input, source->inputs()) {
    CopyInput(source, destination, input, include_connections, true);
  }

  destination->SetPosition(source->GetPosition());
  destination->SetLabel(source->GetLabel());
}

void Node::CopyInput(Node *src, Node *dst, const QString &input, bool include_connections, bool traverse_arrays)
{
  Q_ASSERT(src->id() == dst->id());

  CopyValuesOfElement(src, dst, input, -1);

  // Copy array size
  if (src->InputIsArray(input) && traverse_arrays) {
    int src_array_sz = src->InputArraySize(input);

    for (int i=0; i<src_array_sz; i++) {
      CopyValuesOfElement(src, dst, input, i);
    }
  }

  // Copy connections
  if (include_connections) {
    if (traverse_arrays) {
      // Copy all connections
      for (auto it=src->input_connections().cbegin(); it!=src->input_connections().cend(); it++) {
        ConnectEdge(it->second, NodeInput(dst, input, it->first.element()));
      }
    } else {
      // Just copy the primary connection (at -1)
      if (src->IsInputConnected(input)) {
        ConnectEdge(src->GetConnectedOutput(input), NodeInput(dst, input));
      }
    }
  }
}

void Node::CopyValuesOfElement(Node *src, Node *dst, const QString &input, int src_element, int dst_element)
{
  if (dst_element >= dst->GetInternalInputArraySize(input)) {
    qDebug() << "Ignored destination element that was out of array bounds";
    return;
  }

  // Copy standard value
  dst->SetSplitStandardValue(input, src->GetSplitStandardValue(input, src_element), dst_element);

  // Copy keyframes
  dst->GetImmediate(input, dst_element)->delete_all_keyframes();
  foreach (const NodeKeyframeTrack& track, src->GetImmediate(input, src_element)->keyframe_tracks()) {
    foreach (NodeKeyframe* key, track) {
      key->copy(dst_element, dst);
    }
  }

  // Copy keyframing state
  if (src->IsInputKeyframable(input)) {
    dst->SetInputIsKeyframing(input, src->IsInputKeyframing(input, src_element), dst_element);
  }

  // If this is the root of an array, copy the array size
  if (src_element == -1 && dst_element == -1) {
    dst->InputArrayResize(input, dst->InputArraySize(input));
  }
}

bool Node::CanBeDeleted() const
{
  return can_be_deleted_;
}

void Node::SetCanBeDeleted(bool s)
{
  can_be_deleted_ = s;
}

void GetDependenciesRecursively(QVector<Node*>& list, const Node* node, bool traverse, bool exclusive_only)
{
  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    Node* connected_node = it->second.node();

    if (connected_node->outputs().size() == 1 || !exclusive_only) {
      if (!list.contains(connected_node)) {
        list.append(connected_node);

        if (traverse) {
          GetDependenciesRecursively(list, connected_node, traverse, exclusive_only);
        }
      }
    }
  }
}

/**
 * @brief Recursively collects dependencies of Node `n` and appends them to QList `list`
 *
 * @param traverse
 *
 * TRUE to recursively traverse each node for a complete dependency graph. FALSE to return only the immediate
 * dependencies.
 */
QVector<Node *> Node::GetDependenciesInternal(bool traverse, bool exclusive_only) const
{
  QVector<Node*> list;

  GetDependenciesRecursively(list, this, traverse, exclusive_only);

  return list;
}

void Node::HashInputElement(QCryptographicHash &hash, const QString& input, int element, const rational &time) const
{
  // Get time adjustment
  // For a single frame, we only care about one of the times
  rational input_time = InputTimeAdjustment(input, element, TimeRange(time, time)).in();

  if (IsInputConnected(input, element)) {
    // Traverse down this edge
    GetConnectedNode(input, element)->Hash(hash, input_time);
  } else {
    // Grab the value at this time
    QVariant value = GetValueAtTime(input, input_time, element);
    hash.addData(NodeValue::ValueToBytes(GetInputDataType(input), value));
  }

  // We have one exception for FOOTAGE types, since we resolve the footage into a frame in the renderer
  if (GetInputDataType(input) == NodeValue::kFootage) {
    Stream* stream = Node::ValueToPtr<Stream>(GetStandardValue(input, element));

    if (stream) {
      // Add footage details to hash

      // Footage filename
      hash.addData(stream->footage()->filename().toUtf8());

      // Footage last modified date
      hash.addData(QString::number(stream->footage()->timestamp()).toUtf8());

      // Footage stream
      hash.addData(QString::number(stream->index()).toUtf8());

      if (stream->type() == Stream::kVideo) {
        VideoStream* image_stream = static_cast<VideoStream*>(stream);

        // Current color config and space
        hash.addData(image_stream->footage()->project()->color_manager()->GetConfigFilename().toUtf8());
        hash.addData(image_stream->colorspace().toUtf8());

        // Alpha associated setting
        hash.addData(QString::number(image_stream->premultiplied_alpha()).toUtf8());

        // Pixel aspect ratio
        hash.addData(reinterpret_cast<const char*>(&image_stream->pixel_aspect_ratio()), sizeof(rational));
      }

      // Footage timestamp
      if (stream->type() == Stream::kVideo) {
        VideoStream* video_stream = static_cast<VideoStream*>(stream);

        int64_t video_ts = Timecode::time_to_timestamp(input_time, video_stream->timebase());

        // Add timestamp in units of the video stream's timebase
        hash.addData(reinterpret_cast<const char*>(&video_ts), sizeof(int64_t));

        // Add start time - used for both image sequences and video streams
        hash.addData(QString::number(video_stream->start_time()).toUtf8());
      }
    }
  }
}

QVector<Node *> Node::GetDependencies() const
{
  return GetDependenciesInternal(true, false);
}

QVector<Node *> Node::GetExclusiveDependencies() const
{
  return GetDependenciesInternal(true, true);
}

QVector<Node *> Node::GetImmediateDependencies() const
{
  return GetDependenciesInternal(false, false);
}

ShaderCode Node::GetShaderCode(const QString &shader_id) const
{
  Q_UNUSED(shader_id)

  return ShaderCode(QString(), QString());
}

void Node::ProcessSamples(NodeValueDatabase &, const SampleBufferPtr, SampleBufferPtr, int) const
{
}

void Node::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  Q_UNUSED(frame)
  Q_UNUSED(job)
}

bool Node::OutputsTo(Node *n, bool recursively) const
{
  for (const OutputConnection& conn : output_connections_) {
    Node* connected = conn.second.node();

    if (connected == n) {
      return true;
    } else if (recursively && connected->OutputsTo(n, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::OutputsTo(const QString &id, bool recursively) const
{
  for (const OutputConnection& conn : output_connections_) {
    Node* connected = conn.second.node();

    if (connected->id() == id) {
      return true;
    } else if (recursively && connected->OutputsTo(id, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::OutputsTo(const NodeInput &input, bool recursively) const
{
  for (const OutputConnection& conn : output_connections_) {
    const NodeInput& connected = conn.second;

    if (connected == input) {
      return true;
    } else if (recursively && connected.node()->OutputsTo(input, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::InputsFrom(Node *n, bool recursively) const
{
  for (auto it=input_connections_.cbegin(); it!=input_connections_.cend(); it++) {
    const NodeOutput& connected = it->second;

    if (connected.node() == n) {
      return true;
    } else if (recursively && connected.node()->InputsFrom(n, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::InputsFrom(const QString &id, bool recursively) const
{
  for (auto it=input_connections_.cbegin(); it!=input_connections_.cend(); it++) {
    const NodeOutput& connected = it->second;

    if (connected.node()->id() == id) {
      return true;
    } else if (recursively && connected.node()->InputsFrom(id, recursively)) {
      return true;
    }
  }

  return false;
}

int Node::GetRoutesTo(Node *n) const
{
  bool outputs_directly = false;
  int routes = 0;

  foreach (const OutputConnection& conn, output_connections_) {
    Node* connected_node = conn.second.node();

    if (connected_node == n) {
      outputs_directly = true;
    } else {
      routes += connected_node->GetRoutesTo(n);
    }
  }

  if (outputs_directly) {
    routes++;
  }

  return routes;
}

void Node::DisconnectAll()
{
  // Disconnect inputs (copy map since internal map will change as we disconnect)
  InputConnections copy = input_connections_;
  for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
    DisconnectEdge(it->second, it->first);
  }

  while (!output_connections_.empty()) {
    OutputConnection conn = output_connections_.back();
    DisconnectEdge(conn.first, conn.second);
  }
}

QString Node::GetCategoryName(const CategoryID &c)
{
  switch (c) {
  case kCategoryInput:
    return tr("Input");
  case kCategoryOutput:
    return tr("Output");
  case kCategoryGeneral:
    return tr("General");
  case kCategoryDistort:
    return tr("Distort");
  case kCategoryMath:
    return tr("Math");
  case kCategoryColor:
    return tr("Color");
  case kCategoryFilter:
    return tr("Filter");
  case kCategoryTimeline:
    return tr("Timeline");
  case kCategoryGenerator:
    return tr("Generator");
  case kCategoryChannels:
    return tr("Channel");
  case kCategoryTransition:
    return tr("Transition");
  case kCategoryUnknown:
  case kCategoryCount:
    break;
  }

  return tr("Uncategorized");
}

QVector<TimeRange> Node::TransformTimeTo(const TimeRange &time, Node *target, bool input_dir)
{
  QVector<TimeRange> paths_found;

  if (input_dir) {
    // If this input is connected, traverse it to see if we stumble across the specified `node`
    for (auto it=input_connections_.cbegin(); it!=input_connections_.cend(); it++) {
      TimeRange input_adjustment = InputTimeAdjustment(it->first.input(), it->first.element(), time);
      Node* connected = it->second.node();

      if (connected == target) {
        // We found the target, no need to keep traversing
        if (!paths_found.contains(input_adjustment)) {
          paths_found.append(input_adjustment);
        }
      } else {
        // We did NOT find the target, traverse this
        paths_found.append(connected->TransformTimeTo(input_adjustment, target, input_dir));
      }
    }
  } else {
    // If this input is connected, traverse it to see if we stumble across the specified `node`
    foreach (const OutputConnection& conn, output_connections_) {
      Node* connected_node = conn.second.node();

      TimeRange output_adjustment = connected_node->OutputTimeAdjustment(conn.second.input(), conn.second.element(), time);

      if (connected_node == target) {
        paths_found.append(output_adjustment);
      } else {
        paths_found.append(connected_node->TransformTimeTo(output_adjustment, target, input_dir));
      }
    }
  }

  return paths_found;
}

QVariant Node::PtrToValue(void *ptr)
{
  return reinterpret_cast<quintptr>(ptr);
}

const QPointF &Node::GetPosition() const
{
  return position_;
}

void Node::SetPosition(const QPointF &pos)
{
  position_ = pos;

  emit PositionChanged(position_);
}

void Node::ParameterValueChanged(const QString& input, int element, const TimeRange& range)
{
  InputValueChangedEvent(input, element);

  emit ValueChanged(NodeInput(this, input, element), range);

  if (ignore_connections_.contains(input)) {
    return;
  }

  InvalidateCache(range, input, element);
}

void Node::LoadImmediate(QXmlStreamReader *reader, const QString& input, int element, XMLNodeData &xml_node_data, const QAtomicInt *cancelled)
{
  NodeValue::Type data_type = GetInputDataType(input);

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
            value_on_track = NodeValue::StringToValue(data_type, value_text, element);
          }

          SetSplitStandardValueOnTrack(input, val_index, value_on_track, element);

          val_index++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("keyframing") && IsInputKeyframable(input)) {
      SetInputIsKeyframing(input, reader->readElementText().toInt(), element);
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
              QString key_input;
              rational key_time;
              NodeKeyframe::Type key_type;
              QVariant key_value;
              QPointF key_in_handle;
              QPointF key_out_handle;

              XMLAttributeLoop(reader, attr) {
                if (cancelled && *cancelled) {
                  return;
                }

                if (attr.name() == QStringLiteral("input")) {
                  key_input = attr.value().toString();
                } else if (attr.name() == QStringLiteral("time")) {
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

              key_value = NodeValue::StringToValue(data_type, reader->readElementText(), true);

              NodeKeyframe* key = new NodeKeyframe(key_time, key_value, key_type, track, element, key_input, this);
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
      SetInputProperty(input, QStringLiteral("col_input"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("csdisplay")) {
      SetInputProperty(input, QStringLiteral("col_display"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("csview")) {
      SetInputProperty(input, QStringLiteral("col_view"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("cslook")) {
      SetInputProperty(input, QStringLiteral("col_look"), reader->readElementText());
    } else {
      reader->skipCurrentElement();
    }
  }
}

void Node::SaveImmediate(QXmlStreamWriter *writer, const QString& input, int element) const
{
  if (IsInputKeyframable(input)) {
    writer->writeTextElement(QStringLiteral("keyframing"), QString::number(IsInputKeyframing(input, element)));
  }

  NodeValue::Type data_type = GetInputDataType(input);

  // Write standard value
  writer->writeStartElement(QStringLiteral("standard"));

  foreach (const QVariant& v, GetSplitStandardValue(input, element)) {
    writer->writeTextElement(QStringLiteral("track"), NodeValue::ValueToString(data_type, v, true));
  }

  writer->writeEndElement(); // standard

  // Write keyframes
  writer->writeStartElement(QStringLiteral("keyframes"));

  foreach (const NodeKeyframeTrack& track, GetKeyframeTracks(input, element)) {
    writer->writeStartElement(QStringLiteral("track"));

    foreach (NodeKeyframe* key, track) {
      writer->writeStartElement(QStringLiteral("key"));

      writer->writeAttribute(QStringLiteral("input"), key->input());
      writer->writeAttribute(QStringLiteral("time"), key->time().toString());
      writer->writeAttribute(QStringLiteral("type"), QString::number(key->type()));
      writer->writeAttribute(QStringLiteral("inhandlex"), QString::number(key->bezier_control_in().x()));
      writer->writeAttribute(QStringLiteral("inhandley"), QString::number(key->bezier_control_in().y()));
      writer->writeAttribute(QStringLiteral("outhandlex"), QString::number(key->bezier_control_out().x()));
      writer->writeAttribute(QStringLiteral("outhandley"), QString::number(key->bezier_control_out().y()));

      writer->writeCharacters(NodeValue::ValueToString(data_type, key->value(), true));

      writer->writeEndElement(); // key
    }

    writer->writeEndElement(); // track
  }

  writer->writeEndElement(); // keyframes

  if (data_type == NodeValue::kColor) {
    // Save color management information
    writer->writeTextElement(QStringLiteral("csinput"), GetInputProperty(input, QStringLiteral("col_input")).toString());
    writer->writeTextElement(QStringLiteral("csdisplay"), GetInputProperty(input, QStringLiteral("col_display")).toString());
    writer->writeTextElement(QStringLiteral("csview"), GetInputProperty(input, QStringLiteral("col_view")).toString());
    writer->writeTextElement(QStringLiteral("cslook"), GetInputProperty(input, QStringLiteral("col_look")).toString());
  }
}

TimeRange Node::GetRangeAffectedByKeyframe(NodeKeyframe *key) const
{
  const NodeKeyframeTrack& key_track = GetTrackFromKeyframe(key);
  int keyframe_index = key_track.indexOf(key);

  TimeRange range = GetRangeAroundIndex(key->input(), keyframe_index, key->track(), key->element());

  // If a previous key exists and it's a hold, we don't need to invalidate those frames
  if (key_track.size() > 1
      && keyframe_index > 0
      && key_track.at(keyframe_index - 1)->type() == NodeKeyframe::kHold) {
    range.set_in(key->time());
  }

  return range;
}

TimeRange Node::GetRangeAroundIndex(const QString &input, int index, int track, int element) const
{
  rational range_begin = RATIONAL_MIN;
  rational range_end = RATIONAL_MAX;

  const NodeKeyframeTrack& key_track = GetImmediate(input, element)->keyframe_tracks().at(track);

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

void Node::ClearElement(const QString& input, int index)
{
  GetImmediate(input, index)->delete_all_keyframes();

  if (IsInputKeyframable(input)) {
    SetInputIsKeyframing(input, false, index);
  }

  SetSplitStandardValue(input, GetSplitDefaultValue(input), index);
}

QRectF Node::CreateGizmoHandleRect(const QPointF &pt, int radius)
{
  return QRectF(pt.x() - radius,
                pt.y() - radius,
                2*radius,
                2*radius);
}

double Node::GetGizmoHandleRadius(const QTransform &transform)
{
  double raw_value = QFontMetrics(qApp->font()).height() * 0.25;

  raw_value /= transform.m11();

  return raw_value;
}

void Node::DrawAndExpandGizmoHandles(QPainter *p, int handle_radius, QRectF *rects, int count)
{
  for (int i=0; i<count; i++) {
    QRectF& r = rects[i];

    // Draw rect on screen
    p->drawRect(r);

    // Extend rect so it's easier to drag with handle
    r.adjust(-handle_radius, -handle_radius, handle_radius, handle_radius);
  }
}

void Node::InputValueChangedEvent(const QString &input, int element)
{
  Q_UNUSED(input)
  Q_UNUSED(element)
}

void Node::InputConnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  Q_UNUSED(input)
  Q_UNUSED(element)
  Q_UNUSED(output)
}

void Node::InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output)
{
  Q_UNUSED(input)
  Q_UNUSED(element)
  Q_UNUSED(output)
}

void Node::childEvent(QChildEvent *event)
{
  super::childEvent(event);

  NodeKeyframe* key = dynamic_cast<NodeKeyframe*>(event->child());

  if (key) {
    NodeInput i(this, key->input(), key->element());

    if (event->type() == QEvent::ChildAdded) {
      GetImmediate(key->input(), key->element())->insert_keyframe(key);

      connect(key, &NodeKeyframe::TimeChanged, this, &Node::InvalidateFromKeyframeTimeChange);
      connect(key, &NodeKeyframe::TimeChanged, this, &Node::KeyframeTimeChanged);
      connect(key, &NodeKeyframe::ValueChanged, this, &Node::InvalidateFromKeyframeValueChange);
      connect(key, &NodeKeyframe::TypeChanged, this, &Node::InvalidateFromKeyframeTypeChanged);
      connect(key, &NodeKeyframe::BezierControlInChanged, this, &Node::InvalidateFromKeyframeBezierInChange);
      connect(key, &NodeKeyframe::BezierControlOutChanged, this, &Node::InvalidateFromKeyframeBezierOutChange);

      emit KeyframeAdded(key);
      ParameterValueChanged(i, GetRangeAffectedByKeyframe(key));
    } else if (event->type() == QEvent::ChildRemoved) {
      TimeRange time_affected = GetRangeAffectedByKeyframe(key);

      disconnect(key, &NodeKeyframe::TimeChanged, this, &Node::InvalidateFromKeyframeTimeChange);
      disconnect(key, &NodeKeyframe::TimeChanged, this, &Node::KeyframeTimeChanged);
      disconnect(key, &NodeKeyframe::ValueChanged, this, &Node::InvalidateFromKeyframeValueChange);
      disconnect(key, &NodeKeyframe::TypeChanged, this, &Node::InvalidateFromKeyframeTypeChanged);
      disconnect(key, &NodeKeyframe::BezierControlInChanged, this, &Node::InvalidateFromKeyframeBezierInChange);
      disconnect(key, &NodeKeyframe::BezierControlOutChanged, this, &Node::InvalidateFromKeyframeBezierOutChange);

      GetImmediate(key->input(), key->element())->remove_keyframe(key);

      emit KeyframeRemoved(key);
      ParameterValueChanged(i, time_affected);
    }
  }
}

void Node::InvalidateFromKeyframeBezierInChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  const NodeKeyframeTrack& track = GetTrackFromKeyframe(key);
  int keyframe_index = track.indexOf(key);

  rational start = RATIONAL_MIN;
  rational end = key->time();

  if (keyframe_index > 0) {
    start = track.at(keyframe_index - 1)->time();
  }

  ParameterValueChanged(key->key_track_ref().input(), TimeRange(start, end));
}

void Node::InvalidateFromKeyframeBezierOutChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  const NodeKeyframeTrack& track = GetTrackFromKeyframe(key);
  int keyframe_index = track.indexOf(key);

  rational start = key->time();
  rational end = RATIONAL_MAX;

  if (keyframe_index < track.size() - 1) {
    end = track.at(keyframe_index + 1)->time();
  }

  ParameterValueChanged(key->key_track_ref().input(), TimeRange(start, end));
}

void Node::InvalidateFromKeyframeTimeChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  NodeInputImmediate* immediate = GetImmediate(key->input(), key->element());
  TimeRange original_range = GetRangeAffectedByKeyframe(key);

  TimeRangeList invalidate_range;
  invalidate_range.insert(original_range);

  if (!(original_range.in() < key->time() && original_range.out() > key->time())) {
    // This keyframe needs resorting, store it and remove it from the list
    immediate->remove_keyframe(key);

    // Automatically insertion sort
    immediate->insert_keyframe(key);

    // Invalidate new area that the keyframe has been moved to
    invalidate_range.insert(GetRangeAffectedByKeyframe(key));
  }

  // Invalidate entire area surrounding the keyframe (either where it currently is, or where it used to be before it
  // was resorted in the if block above)
  foreach (const TimeRange& r, invalidate_range) {
    ParameterValueChanged(key->key_track_ref().input(), r);
  }
}

void Node::InvalidateFromKeyframeValueChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  ParameterValueChanged(key->key_track_ref().input(), GetRangeAffectedByKeyframe(key));
}

void Node::InvalidateFromKeyframeTypeChanged()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  const NodeKeyframeTrack& track = GetTrackFromKeyframe(key);

  if (track.size() == 1) {
    // If there are no other frames, the interpolation won't do anything
    return;
  }

  // Invalidate entire range
  ParameterValueChanged(key->key_track_ref().input(), GetRangeAroundIndex(key->input(), track.indexOf(key), key->track(), key->element()));
}

Project *Node::ArrayInsertCommand::GetRelevantProject() const
{
  return node_->parent()->project();
}

Project *Node::ArrayRemoveCommand::GetRelevantProject() const
{
  return node_->parent()->project();
}

Project *Node::ArrayResizeCommand::GetRelevantProject() const
{
  return node_->parent()->project();
}

}
