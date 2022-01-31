/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "node/project/footage/footage.h"
#include "project/project.h"
#include "ui/colorcoding.h"
#include "ui/icons/icons.h"
#include "widget/nodeparamview/nodeparamviewundo.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

#define super QObject

Node::Node() :
  can_be_deleted_(true),
  override_color_(-1),
  folder_(nullptr),
  operation_stack_(0),
  cache_result_(false),
  flags_(kNone)
{
  uuid_ = QUuid::createUuid();
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

Project* Node::project() const
{
  QObject *t = this->parent();

  while (t) {
    if (Project *p = dynamic_cast<Project*>(t)) {
      return p;
    }
    t = t->parent();
  }

  return nullptr;
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

QIcon Node::icon() const
{
  // Just a meaningless default icon to be used where necessary
  return icon::New;
}

bool Node::SetNodePositionInContext(Node *node, const QPointF &pos)
{
  Position p = context_positions_.value(node);

  p.position = pos;

  return SetNodePositionInContext(node, p);
}

bool Node::SetNodePositionInContext(Node *node, const Position &pos)
{
  bool added = !ContextContainsNode(node);
  context_positions_.insert(node, pos);

  if (added) {
    emit NodeAddedToContext(node);
  }

  emit NodePositionInContextChanged(node, pos.position);

  return added;
}

bool Node::RemoveNodeFromContext(Node *node)
{
  if (ContextContainsNode(node)) {
    context_positions_.remove(node);
    emit NodeRemovedFromContext(node);
    return true;
  } else {
    return false;
  }
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

void Node::ConnectEdge(Node *output, const NodeInput &input)
{
  // Ensure graph is the same
  Q_ASSERT(input.node()->parent() == output->parent());

  // Ensure a connection isn't getting overwritten
  Q_ASSERT(input.node()->input_connections().find(input) == input.node()->input_connections().end());

  // Insert connection on both sides
  input.node()->input_connections_[input] = output;
  output->output_connections_.push_back(std::pair<Node*, NodeInput>({output, input}));

  // Call internal events
  input.node()->InputConnectedEvent(input.input(), input.element(), output);
  output->OutputConnectedEvent(input);

  // Emit signals
  emit input.node()->InputConnected(output, input);
  emit output->OutputConnected(output, input);

  // Invalidate all if this node isn't ignoring this input
  if (!input.node()->ignore_connections_.contains(input.input())) {
    input.node()->InvalidateAll(input.input(), input.element());
  }
}

void Node::DisconnectEdge(Node *output, const NodeInput &input)
{
  // Ensure graph is the same
  Q_ASSERT(input.node()->parent() == output->parent());

  // Ensure connection exists
  Q_ASSERT(input.node()->input_connections().at(input) == output);

  // Remove connection from both sides
  InputConnections& inputs = input.node()->input_connections_;
  inputs.erase(inputs.find(input));

  OutputConnections& outputs = output->output_connections_;
  outputs.erase(std::find(outputs.begin(), outputs.end(), std::pair<Node*, NodeInput>({output, input})));

  // Call internal events
  input.node()->InputDisconnectedEvent(input.input(), input.element(), output);
  output->OutputDisconnectedEvent(input);

  emit input.node()->InputDisconnected(output, input);
  emit output->OutputDisconnected(output, input);

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

bool Node::IsInputHidden(const QString &input) const
{
  return (GetInputFlags(input) & kInputFlagHidden);
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
  return GetConnectedOutput(input, element);
}

Node *Node::GetConnectedOutput(const QString &input, int element) const
{
  for (auto it=input_connections_.cbegin(); it!=input_connections_.cend(); it++) {
    if (it->first.input() == input && it->first.element() == element) {
      return it->second;
    }
  }

  return nullptr;
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
  Input* input_meta = GetInternalInputData(id);

  if (input_meta) {
    input_meta->type = type;

    int array_sz = InputArraySize(id);
    for (int i=-1; i<array_sz; i++) {
      GetImmediate(id, i)->set_data_type(type);
    }

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
    vals.append(GetSplitValueAtTimeOnTrack(input, time, i, element));
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

    NodeValue::Type type = GetInputDataType(input);

    // If we're here, the time must be somewhere in between the keyframes
    NodeKeyframe *before = nullptr, *after = nullptr;

    int low = 0;
    int high = key_track.size()-1;
    while (low <= high) {
      int mid = low + (high - low) / 2;
      NodeKeyframe *mid_key = key_track.at(mid);
      NodeKeyframe *next_key = key_track.at(mid + 1);

      if (mid_key->time() <= time && next_key->time() > time) {
        before = mid_key;
        after = next_key;
        break;
      } else if (mid_key->time() < time) {
        low = mid + 1;
      } else {
        high = mid - 1;
      }
    }

    if (before) {
      if (before->time() == time
          || ((!NodeValue::type_can_be_interpolated(type) || before->type() == NodeKeyframe::kHold) && after->time() > time)) {

        // Time == keyframe time, so value is precise
        return before->value();

      } else if (after->time() == time) {

        // Time == keyframe time, so value is precise
        return after->value();

      } else if (before->time() < time && after->time() > time) {
        // We must interpolate between these keyframes

        double before_val, after_val, interpolated;
        if (type == NodeValue::kRational) {
          before_val = before->value().value<rational>().toDouble();
          after_val = after->value().value<rational>().toDouble();
        } else {
          before_val = before->value().toDouble();
          after_val = after->value().toDouble();
        }

        if (before->type() == NodeKeyframe::kBezier && after->type() == NodeKeyframe::kBezier) {

          // Perform a cubic bezier with two control points
          interpolated = Bezier::CubicXtoY(time.toDouble(),
                                           QPointF(before->time().toDouble(), before_val),
                                           QPointF(before->time().toDouble() + before->valid_bezier_control_out().x(), before_val + before->valid_bezier_control_out().y()),
                                           QPointF(after->time().toDouble() + after->valid_bezier_control_in().x(), after_val + after->valid_bezier_control_in().y()),
                                           QPointF(after->time().toDouble(), after_val));

        } else if (before->type() == NodeKeyframe::kBezier || after->type() == NodeKeyframe::kBezier) {
          // Perform a quadratic bezier with only one control point

          QPointF control_point;

          if (before->type() == NodeKeyframe::kBezier) {
            control_point = before->valid_bezier_control_out();
            control_point.setX(control_point.x() + before->time().toDouble());
            control_point.setY(control_point.y() + before_val);
          } else {
            control_point = after->valid_bezier_control_in();
            control_point.setX(control_point.x() + after->time().toDouble());
            control_point.setY(control_point.y() + after_val);
          }

          // Interpolate value using quadratic beziers
          interpolated = Bezier::QuadraticXtoY(time.toDouble(),
                                               QPointF(before->time().toDouble(), before_val),
                                               control_point,
                                               QPointF(after->time().toDouble(), after_val));

        } else {
          // To have arrived here, the keyframes must both be linear
          qreal period_progress = (time.toDouble() - before->time().toDouble()) / (after->time().toDouble() - before->time().toDouble());

          interpolated = lerp(before_val, after_val, period_progress);
        }

        if (type == NodeValue::kRational) {
          return QVariant::fromValue(rational::fromDouble(interpolated));
        } else {
          return interpolated;
        }
      }
    } else {
      qWarning() << "Binary search for keyframes failed";
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
  SplitValue val = GetSplitDefaultValue(input);
  if (track < val.size()) {
    return val.at(track);
  } else {
    return QVariant();
  }
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
      if (it->first.input() == id && it->first.element() >= index) {
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
    c->redo_now();
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
      if (it->first.input() == id && it->first.element() >= index) {
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

void Node::Hash(const Node *node, const ValueHint &hint, QCryptographicHash &hash, const NodeGlobals &globals, const VideoParams &video_params)
{
  hint.Hash(hash);
  node->Hash(hash, globals, video_params);
}

void Node::SetValueHintForInput(const QString &input, const ValueHint &hint, int element)
{
  value_hints_.insert({input, element}, hint);

  emit InputValueHintChanged(NodeInput(this, input, element));

  InvalidateAll(input, element);
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

InputFlags Node::GetInputFlags(const QString &input) const
{
  const Input* i = GetInternalInputData(input);

  if (i) {
    return i->flags;
  } else {
    ReportInvalidInput("retrieve flags of", input);
    return InputFlags(kInputFlagNormal);
  }
}

void Node::Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const
{
  // Do nothing
  Q_UNUSED(value)
  Q_UNUSED(globals)
  Q_UNUSED(table)
}

void Node::InvalidateCache(const TimeRange &range, const QString &from, int element, InvalidateCacheOptions options)
{
  Q_UNUSED(from)
  Q_UNUSED(element)

  SendInvalidateCache(range, options);
}

void Node::BeginOperation()
{
  // Increase operation stack
  operation_stack_++;
}

void Node::EndOperation()
{
  // Decrease operation stack
  operation_stack_--;
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
    Node* c = nodes.at(i)->copy();

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
      int connection_index = src.indexOf(it->second);

      if (connection_index > -1) {
        // Find the equivalent node in the dst list
        Node *copied_output = dst.at(connection_index);
        NodeInput copied_input = NodeInput(dst_node, it->first.input(), it->first.element());

        if (command) {
          command->add_child(new NodeEdgeAddCommand(copied_output, copied_input));
          command->add_child(new NodeSetValueHintCommand(copied_input, src_node->GetValueHintForInput(copied_input.input(), copied_input.element())));
        } else {
          ConnectEdge(copied_output, copied_input);
          copied_input.node()->SetValueHintForInput(copied_input.input(), src_node->GetValueHintForInput(copied_input.input(), copied_input.element()), copied_input.element());
        }
      }
    }
  }
}

Node *Node::CopyNodeAndDependencyGraphMinusItemsInternal(QMap<Node*, Node*>& created, Node *node, MultiUndoCommand *command)
{
  // Make a new node of the same type
  Node* copy = node->copy();

  // Add to map
  created.insert(node, copy);

  // Copy values to the clone
  CopyInputs(node, copy, false);

  // Add it to the same graph
  command->add_child(new NodeAddCommand(node->parent(), copy));

  // Go through input connections and copy if non-item and connect if item
  for (auto it=node->input_connections_.cbegin(); it!=node->input_connections_.cend(); it++) {
    NodeInput input = it->first;
    Node* connected = it->second;
    Node* connected_copy;

    if (connected->IsItem()) {
      // This is an item and we avoid copying those and just connect to them directly
      connected_copy = connected;
    } else {
      // Non-item, we want to clone this too
      connected_copy = created.value(connected, nullptr);

      if (!connected_copy) {
        connected_copy = CopyNodeAndDependencyGraphMinusItemsInternal(created, connected, command);
      }
    }

    NodeInput copied_input(copy, input.input(), input.element());
    command->add_child(new NodeEdgeAddCommand(connected_copy, copied_input));
    command->add_child(new NodeSetValueHintCommand(copied_input, node->GetValueHintForInput(input.input(), input.element())));
  }

  const PositionMap &map = node->GetContextPositions();
  for (auto it=map.cbegin(); it!=map.cend(); it++) {
    // Add either the copy (if it exists) or the original node to the context
    command->add_child(new NodeSetPositionCommand(created.value(it.key(), it.key()), copy, it.value()));
  }

  return copy;
}

Node *Node::CopyNodeAndDependencyGraphMinusItems(Node *node, MultiUndoCommand *command)
{
  QMap<Node*, Node*> created;

  return CopyNodeAndDependencyGraphMinusItemsInternal(created, node, command);
}

Node *Node::CopyNodeInGraph(Node *node, MultiUndoCommand *command)
{
  Node* copy;

  if (Config::Current()[QStringLiteral("SplitClipsCopyNodes")].toBool()) {
    copy = Node::CopyNodeAndDependencyGraphMinusItems(node, command);
  } else {
    copy = node->copy();

    command->add_child(new NodeAddCommand(static_cast<NodeGraph*>(node->parent()),
                                          copy));

    command->add_child(new NodeCopyInputsCommand(node, copy, true));

    const PositionMap &map = node->GetContextPositions();
    for (auto it=map.cbegin(); it!=map.cend(); it++) {
      // Add to the context
      command->add_child(new NodeSetPositionCommand(it.key(), copy, it.value()));
    }
  }

  return copy;
}

void Node::SendInvalidateCache(const TimeRange &range, const InvalidateCacheOptions &options)
{
  if (GetOperationStack() == 0) {
    for (const OutputConnection& conn : output_connections_) {
      // Send clear cache signal to the Node
      const NodeInput& in = conn.second;

      in.node()->InvalidateCache(range, in.input(), in.element(), options);
    }
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

void Node::HashAddNodeSignature(QCryptographicHash &hash) const
{
  // Add node ID
  hash.addData(id().toUtf8());
}

void Node::InsertInput(const QString &id, NodeValue::Type type, const QVariant &default_value, InputFlags flags, int index)
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

  input_ids_.insert(index, id);
  input_data_.insert(index, i);

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

    int old_sz = imm->array_size;
    imm->array_size = size;
    emit InputArraySizeChanged(id, old_sz, size);
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

bool Node::HasGizmos() const
{
  return false;
}

void Node::DrawGizmos(const NodeValueRow &, const NodeGlobals &, QPainter *)
{
}

bool Node::GizmoPress(const NodeValueRow &, const NodeGlobals &, const QPointF &)
{
  return false;
}

void Node::GizmoMove(const QPointF &, const rational&, const Qt::KeyboardModifiers &)
{
}

void Node::GizmoRelease(MultiUndoCommand *)
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

QString Node::GetLabelAndName() const
{
  if (GetLabel().isEmpty()) {
    return Name();
  } else {
    return tr("%1 (%2)").arg(GetLabel(), Name());
  }
}

QString Node::GetLabelOrName() const
{
  if (GetLabel().isEmpty()) {
    return Name();
  }
  return GetLabel();
}

void Node::Hash(QCryptographicHash &hash, const NodeGlobals &globals, const VideoParams &video_params) const
{
  // Add this Node's ID and output being used
  HashAddNodeSignature(hash);

  foreach (const QString& input, inputs()) {
    // For each input, try to hash its value
    if (ignore_when_hashing_.contains(input)) {
      continue;
    }

    int arr_sz = InputArraySize(input);
    for (int i=-1; i<arr_sz; i++) {
      HashInputElement(hash, input, i, globals, video_params);
    }
  }
}

void Node::CopyInputs(const Node *source, Node *destination, bool include_connections)
{
  Q_ASSERT(source->id() == destination->id());

  foreach (const QString& input, source->inputs()) {
    CopyInput(source, destination, input, include_connections, true);
  }

  destination->SetLabel(source->GetLabel());
  destination->SetOverrideColor(source->GetOverrideColor());
}

void Node::CopyInput(const Node *src, Node *dst, const QString &input, bool include_connections, bool traverse_arrays)
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

void Node::CopyValuesOfElement(const Node *src, Node *dst, const QString &input, int src_element, int dst_element)
{
  if (dst_element >= dst->GetInternalInputArraySize(input)) {
    qDebug() << "Ignored destination element that was out of array bounds";
    return;
  }

  // Copy standard value
  dst->SetSplitStandardValue(input, src->GetSplitStandardValue(input, src_element), dst_element);

  // Copy keyframes
  if (NodeInputImmediate *immediate = dst->GetImmediate(input, dst_element)) {
    immediate->delete_all_keyframes();
  }
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
    dst->ArrayResizeInternal(input, src->InputArraySize(input));
  }

  // Copy value hint
  dst->SetValueHintForInput(input, src->GetValueHintForInput(input, src_element), dst_element);
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
    Node* connected_node = it->second;

    if (!exclusive_only || !connected_node->IsItem()) {
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

void Node::HashInputElement(QCryptographicHash &hash, const QString& input, int element, const NodeGlobals &globals, const VideoParams& video_params) const
{
  // Get time adjustment
  // For a single frame, we only care about one of the times
  TimeRange input_time = InputTimeAdjustment(input, element, globals.time());

  if (IsInputConnected(input, element)) {
    // Traverse down this edge
    Node *output = GetConnectedOutput(input, element);

    NodeGlobals new_globals = globals;
    new_globals.set_time(input_time);
    Node::Hash(output, GetValueHintForInput(input, element), hash, new_globals, video_params);
  } else {
    // Grab the value at this time
    QVariant value = GetValueAtTime(input, input_time.in(), element);
    hash.addData(NodeValue::ValueToBytes(GetInputDataType(input), value));
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

void Node::ProcessSamples(const NodeValueRow &, const SampleBufferPtr, SampleBufferPtr, int) const
{
}

void Node::GenerateFrame(FramePtr frame, const GenerateJob &job) const
{
  Q_UNUSED(frame)
  Q_UNUSED(job)
}

bool Node::OutputsTo(Node *n, bool recursively, const OutputConnections &ignore_edges, const OutputConnection &added_edge) const
{
  for (const OutputConnection& conn : output_connections_) {
    if (std::find(ignore_edges.cbegin(), ignore_edges.cend(), conn) != ignore_edges.cend()) {
      // If this edge is in the "ignore edges" list, skip it
      continue;
    }

    Node* connected = conn.second.node();

    if (connected == n) {
      return true;
    } else if (recursively && connected->OutputsTo(n, recursively, ignore_edges, added_edge)) {
      return true;
    } else if (added_edge.first == this) {
      Node *proposed_connected = added_edge.second.node();

      if (proposed_connected == n) {
        return true;
      } else if (recursively && proposed_connected->OutputsTo(n, recursively, ignore_edges, added_edge)) {
        return true;
      }
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
    Node *connected = it->second;

    if (connected == n) {
      return true;
    } else if (recursively && connected->InputsFrom(n, recursively)) {
      return true;
    }
  }

  return false;
}

bool Node::InputsFrom(const QString &id, bool recursively) const
{
  for (auto it=input_connections_.cbegin(); it!=input_connections_.cend(); it++) {
    Node *connected = it->second;

    if (connected->id() == id) {
      return true;
    } else if (recursively && connected->InputsFrom(id, recursively)) {
      return true;
    }
  }

  return false;
}

int Node::GetNumberOfRoutesTo(Node *n) const
{
  bool outputs_directly = false;
  int routes = 0;

  foreach (const OutputConnection& conn, output_connections_) {
    Node* connected_node = conn.second.node();

    if (connected_node == n) {
      outputs_directly = true;
    } else {
      routes += connected_node->GetNumberOfRoutesTo(n);
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
  case kCategoryProject:
    return tr("Project");
  case kCategoryVideoEffect:
    return tr("Video Effect");
  case kCategoryAudioEffect:
    return tr("Audio Effect");
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
      Node* connected = it->second;

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

void Node::ParameterValueChanged(const QString& input, int element, const TimeRange& range)
{
  InputValueChangedEvent(input, element);

  emit ValueChanged(NodeInput(this, input, element), range);

  if (ignore_connections_.contains(input)) {
    return;
  }

  InvalidateCache(range, input, element);
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
  p->setPen(Qt::NoPen);
  p->setBrush(Qt::white);

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

void Node::InputConnectedEvent(const QString &input, int element, Node *output)
{
  Q_UNUSED(input)
  Q_UNUSED(element)
  Q_UNUSED(output)
}

void Node::InputDisconnectedEvent(const QString &input, int element, Node *output)
{
  Q_UNUSED(input)
  Q_UNUSED(element)
  Q_UNUSED(output)
}

void Node::OutputConnectedEvent(const NodeInput &input)
{
  Q_UNUSED(input)
}

void Node::OutputDisconnectedEvent(const NodeInput &input)
{
  Q_UNUSED(input)
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
      connect(key, &NodeKeyframe::ValueChanged, this, &Node::InvalidateFromKeyframeValueChange);
      connect(key, &NodeKeyframe::TypeChanged, this, &Node::InvalidateFromKeyframeTypeChanged);
      connect(key, &NodeKeyframe::BezierControlInChanged, this, &Node::InvalidateFromKeyframeBezierInChange);
      connect(key, &NodeKeyframe::BezierControlOutChanged, this, &Node::InvalidateFromKeyframeBezierOutChange);

      emit KeyframeAdded(key);
      ParameterValueChanged(i, GetRangeAffectedByKeyframe(key));
    } else if (event->type() == QEvent::ChildRemoved) {
      TimeRange time_affected = GetRangeAffectedByKeyframe(key);

      disconnect(key, &NodeKeyframe::TimeChanged, this, &Node::InvalidateFromKeyframeTimeChange);
      disconnect(key, &NodeKeyframe::ValueChanged, this, &Node::InvalidateFromKeyframeValueChange);
      disconnect(key, &NodeKeyframe::TypeChanged, this, &Node::InvalidateFromKeyframeTypeChanged);
      disconnect(key, &NodeKeyframe::BezierControlInChanged, this, &Node::InvalidateFromKeyframeBezierInChange);
      disconnect(key, &NodeKeyframe::BezierControlOutChanged, this, &Node::InvalidateFromKeyframeBezierOutChange);

      emit KeyframeRemoved(key);

      GetImmediate(key->input(), key->element())->remove_keyframe(key);
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

  emit KeyframeTimeChanged(key);
}

void Node::InvalidateFromKeyframeValueChange()
{
  NodeKeyframe* key = static_cast<NodeKeyframe*>(sender());
  ParameterValueChanged(key->key_track_ref().input(), GetRangeAffectedByKeyframe(key));

  emit KeyframeValueChanged(key);
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

  emit KeyframeTypeChanged(key);
}

Project *Node::ArrayInsertCommand::GetRelevantProject() const
{
  return node_->project();
}

Project *Node::ArrayRemoveCommand::GetRelevantProject() const
{
  return node_->project();
}

Project *Node::ArrayResizeCommand::GetRelevantProject() const
{
  return node_->project();
}

void NodeSetPositionCommand::redo()
{
  added_ = !context_->ContextContainsNode(node_);

  if (!added_) {
    old_pos_ = context_->GetNodePositionDataInContext(node_);
  }

  context_->SetNodePositionInContext(node_, pos_);
}

void NodeSetPositionCommand::undo()
{
  if (added_) {
    context_->RemoveNodeFromContext(node_);
  } else {
    context_->SetNodePositionInContext(node_, old_pos_);
  }
}

void NodeRemovePositionFromContextCommand::redo()
{
  contained_ = context_->ContextContainsNode(node_);

  if (contained_) {
    old_pos_ = context_->GetNodePositionDataInContext(node_);
    context_->RemoveNodeFromContext(node_);
  }
}

void NodeRemovePositionFromContextCommand::undo()
{
  if (contained_) {
    context_->SetNodePositionInContext(node_, old_pos_);
  }
}

void NodeRemovePositionFromAllContextsCommand::redo()
{
  NodeGraph *graph = node_->parent();

  foreach (Node* context, graph->nodes()) {
    if (context->ContextContainsNode(node_)) {
      contexts_.insert({context, context->GetNodePositionInContext(node_)});
      context->RemoveNodeFromContext(node_);
    }
  }
}

void NodeRemovePositionFromAllContextsCommand::undo()
{
  for (auto it = contexts_.crbegin(); it != contexts_.crend(); it++) {
    it->first->SetNodePositionInContext(node_, it->second);
  }

  contexts_.clear();
}

void Node::ValueHint::Hash(QCryptographicHash &hash) const
{
  // Add value hint
  if (!types().isEmpty()) {
    hash.addData(reinterpret_cast<const char*>(types().constData()), sizeof(NodeValue::Type) * types().size());
  }
  hash.addData(reinterpret_cast<const char*>(&index()), sizeof(index()));
  hash.addData(tag().toUtf8());
}

void NodeSetPositionAndDependenciesRecursivelyCommand::prepare()
{
  move_recursively(node_, pos_.position - context_->GetNodePositionDataInContext(node_).position);
}

void NodeSetPositionAndDependenciesRecursivelyCommand::redo()
{
  for (auto it=commands_.cbegin(); it!=commands_.cend(); it++) {
    (*it)->redo_now();
  }
}

void NodeSetPositionAndDependenciesRecursivelyCommand::undo()
{
  for (auto it=commands_.crbegin(); it!=commands_.crend(); it++) {
    (*it)->undo_now();
  }
}

void NodeSetPositionAndDependenciesRecursivelyCommand::move_recursively(Node *node, const QPointF &diff)
{
  Node::Position pos = context_->GetNodePositionDataInContext(node);
  pos += diff;
  commands_.append(new NodeSetPositionCommand(node_, context_, pos));

  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    Node *output = it->second;
    if (context_->ContextContainsNode(output)) {
      move_recursively(output, diff);
    }
  }
}

}
