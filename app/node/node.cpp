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

#include "node.h"

#include <QApplication>
#include <QGuiApplication>
#include <QDebug>
#include <QFile>

#include "common/lerp.h"
#include "common/qtutils.h"
#include "config/config.h"
#include "core.h"
#include "node/group/group.h"
#include "node/project/serializer/typeserializer.h"
#include "nodeundo.h"
#include "project.h"
#include "serializeddata.h"
#include "ui/colorcoding.h"
#include "ui/icons/icons.h"

namespace olive {

#define super QObject

const QString Node::kEnabledInput = QStringLiteral("enabled_in");

Node::Node() :
  override_color_(-1),
  folder_(nullptr),
  flags_(kNone),
  caches_enabled_(true)
{
  AddInput(kEnabledInput, TYPE_BOOL, true);
  AddOutput(QString());

  video_cache_ = new FrameHashCache(this);
  thumbnail_cache_ = new ThumbnailCache(this);
  audio_cache_ = new AudioPlaybackCache(this);
  waveform_cache_ = new AudioWaveformCache(this);

  waveform_cache_->SetSavingEnabled(false);
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

Project *Node::parent() const
{
  return static_cast<Project*>(QObject::parent());
}

Project *Node::project() const
{
  return Project::GetProjectFromObject(this);
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
  SetInputName(kEnabledInput, tr("Enabled"));
  SetOutputName(QString(), tr("Default"));
}

QVariant Node::data(const DataType &d) const
{
  if (d == ICON) {
    // Just a meaningless default icon to be used where necessary
    return icon::New;
  }

  return QVariant();
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
    c = OLIVE_CONFIG_STR(QStringLiteral("CatColor%1").arg(this->Category().first())).toInt();
  }

  return ColorCoding::GetColor(c);
}

QLinearGradient Node::gradient_color(qreal top, qreal bottom) const
{
  QLinearGradient grad;

  grad.setStart(0, top);
  grad.setFinalStop(0, bottom);

  QColor c = QtUtils::toQColor(color());

  grad.setColorAt(0.0, c.lighter());
  grad.setColorAt(1.0, c);

  return grad;
}

QBrush Node::brush(qreal top, qreal bottom) const
{
  if (OLIVE_CONFIG("UseGradients").toBool()) {
    return gradient_color(top, bottom);
  } else {
    return QtUtils::toQColor(color());
  }
}

bool Node::ConnectionExists(const NodeOutput &output, const NodeInput &input)
{
  for (auto it = output.node()->output_connections_.cbegin(); it != output.node()->output_connections_.cend(); it++) {
    if (it->first == output && it->second == input) {
      return true;
    }
  }

  return false;
}

void Node::ConnectEdge(const NodeOutput &output, const NodeInput &input)
{
  // Ensure graph is the same
  Q_ASSERT(input.node()->parent() == output.node()->parent());

  // Ensure connection doesn't already exist
  Q_ASSERT(!ConnectionExists(output, input));

  // Insert connection on both sides
  auto conn = std::pair<NodeOutput, NodeInput>({output, input});
  input.node()->input_connections_.push_back(conn);
  output.node()->output_connections_.push_back(conn);

  // Call internal events
  input.node()->InputConnectedEvent(input.input(), input.element(), output);
  output.node()->OutputConnectedEvent(input);

  // Emit signals
  emit input.node()->InputConnected(output, input);
  emit output.node()->OutputConnected(output, input);

  // Invalidate all if this node isn't ignoring this input
  if (!(input.node()->GetInputFlags(input.input()) & kInputFlagIgnoreInvalidations)) {
    input.node()->InvalidateAll(input.input(), input.element());
  }
}

void Node::DisconnectEdge(const NodeOutput &output, const NodeInput &input)
{
  // Ensure graph is the same
  Q_ASSERT(input.node()->parent() == output.node()->parent());

  // Ensure connection exists
  Q_ASSERT(ConnectionExists(output, input));

  // Remove connection from both sides
  auto conn = std::pair<NodeOutput, NodeInput>({output, input});

  Connections& inputs = input.node()->input_connections_;
  inputs.erase(std::find(inputs.begin(), inputs.end(), conn));

  Connections& outputs = output.node()->output_connections_;
  outputs.erase(std::find(outputs.begin(), outputs.end(), conn));

  // Call internal events
  input.node()->InputDisconnectedEvent(input.input(), input.element(), output);
  output.node()->OutputDisconnectedEvent(input);

  emit input.node()->InputDisconnected(output, input);
  emit output.node()->OutputDisconnected(output, input);

  if (!(input.node()->GetInputFlags(input.input()) & kInputFlagIgnoreInvalidations)) {
    input.node()->InvalidateAll(input.input(), input.element());
  }
}

void Node::CopyCacheUuidsFrom(Node *n)
{
  video_cache_->SetUuid(n->video_cache_->GetUuid());
  audio_cache_->SetUuid(n->audio_cache_->GetUuid());
  thumbnail_cache_->SetUuid(n->thumbnail_cache_->GetUuid());
  waveform_cache_->SetUuid(n->waveform_cache_->GetUuid());
}

QString Node::GetInputName(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->human_name;
  } else {
    ReportInvalidInput("get name of", id, -1);
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
    ReportInvalidInput("get keyframing state of", input, element);
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
    ReportInvalidInput("set keyframing state of", input, element);
  }
}

bool Node::IsInputConnected(const QString &input, int element) const
{
  return GetConnectedOutput(input, element);
}

NodeOutput Node::GetConnectedOutput2(const QString &input, int element) const
{
  // NOTE: Only returns the first output connected to this input, there may be more than one
  for (auto it = input_connections_.cbegin(); it != input_connections_.cend(); it++) {
    if (it->second.input() == input && it->second.element() == element) {
      return it->first;
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
    ReportInvalidInput("determine whether using standard value in", input, element);
    return true;
  }
}

type_t Node::GetInputDataType(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->type;
  } else {
    ReportInvalidInput("get data type of", id, -1);
    return TYPE_NONE;
  }
}

void Node::SetInputDataType(const QString &id, const type_t &type, size_t channels)
{
  Input* input_meta = GetInternalInputData(id);

  if (input_meta) {
    input_meta->type = type;
    input_meta->channel_count = channels;

    /*int array_sz = InputArraySize(id);
    for (int i=-1; i<array_sz; i++) {
      GetImmediate(id, i)->set_data_type(type, input_meta->channel_count);
    }*/

    emit InputDataTypeChanged(id, type);
  } else {
    ReportInvalidInput("set data type of", id, -1);
  }
}

bool Node::HasInputProperty(const QString &id, const QString &name) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->properties.contains(name);
  } else {
    ReportInvalidInput("get property of", id, -1);
    return false;
  }
}

QHash<QString, value_t> Node::GetInputProperties(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->properties;
  } else {
    ReportInvalidInput("get property table of", id, -1);
    return QHash<QString, value_t>();
  }
}

value_t Node::GetInputProperty(const QString &id, const QString &name) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->properties.value(name);
  } else {
    ReportInvalidInput("get property of", id, -1);
    return value_t();
  }
}

void Node::SetInputProperty(const QString &id, const QString &name, const value_t &value)
{
  Input* i = GetInternalInputData(id);

  if (i) {
    i->properties.insert(name, value);

    emit InputPropertyChanged(id, name, value);
  } else {
    ReportInvalidInput("set property of", id, -1);
  }
}

value_t Node::GetValueAtTime(const QString &input, const rational &time, int element) const
{
  value_t v(GetInputDataType(input), GetNumberOfKeyframeTracks(input));

  for (size_t i = 0; i < v.data().size(); i++) {
    v.data()[i] = GetSplitValueAtTimeOnTrack(input, time, i, element);
  }

  return v;
}

value_t::component_t Node::GetSplitValueAtTimeOnTrack(const QString &input, const rational &time, int track, int element) const
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

    type_t type = GetInputDataType(input);

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

    bool can_be_interpolated = (type == TYPE_INTEGER || type == TYPE_DOUBLE || type == TYPE_RATIONAL);

    if (before) {
      if (before->time() == time
          || ((!can_be_interpolated || before->type() == NodeKeyframe::kHold) && after->time() > time)) {

        // Time == keyframe time, so value is precise
        return before->value();

      } else if (after->time() == time) {

        // Time == keyframe time, so value is precise
        return after->value();

      } else if (before->time() < time && after->time() > time) {
        // We must interpolate between these keyframes

        double before_val, after_val, interpolated;
        if (type == TYPE_RATIONAL) {
          before_val = before->value().value<rational>().toDouble();
          after_val = after->value().value<rational>().toDouble();
        } else if (type == TYPE_INTEGER) {
          before_val = before->value().value<int64_t>();
          after_val = after->value().value<int64_t>();
        } else {
          before_val = before->value().value<double>();
          after_val = after->value().value<double>();
        }

        if (before->type() == NodeKeyframe::kBezier && after->type() == NodeKeyframe::kBezier) {

          // Perform a cubic bezier with two control points
          interpolated = Bezier::CubicXtoY(time.toDouble(),
                                           Imath::V2d(before->time().toDouble(), before_val),
                                           Imath::V2d(before->time().toDouble() + before->valid_bezier_control_out().x(), before_val + before->valid_bezier_control_out().y()),
                                           Imath::V2d(after->time().toDouble() + after->valid_bezier_control_in().x(), after_val + after->valid_bezier_control_in().y()),
                                           Imath::V2d(after->time().toDouble(), after_val));

        } else if (before->type() == NodeKeyframe::kBezier || after->type() == NodeKeyframe::kBezier) {
          // Perform a quadratic bezier with only one control point

          Imath::V2d control_point;

          if (before->type() == NodeKeyframe::kBezier) {
            control_point.x = (before->valid_bezier_control_out().x() + before->time().toDouble());
            control_point.y = (before->valid_bezier_control_out().y() + before_val);
          } else {
            control_point.x = (after->valid_bezier_control_in().x() + after->time().toDouble());
            control_point.y = (after->valid_bezier_control_in().y() + after_val);
          }

          // Interpolate value using quadratic beziers
          interpolated = Bezier::QuadraticXtoY(time.toDouble(),
                                               Imath::V2d(before->time().toDouble(), before_val),
                                               control_point,
                                               Imath::V2d(after->time().toDouble(), after_val));

        } else {
          // To have arrived here, the keyframes must both be linear
          qreal period_progress = (time.toDouble() - before->time().toDouble()) / (after->time().toDouble() - before->time().toDouble());

          interpolated = lerp(before_val, after_val, period_progress);
        }

        if (type == TYPE_RATIONAL) {
          return rational::fromDouble(interpolated);
        } else if (type == TYPE_INTEGER) {
          return int64_t(std::round(interpolated));
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

value_t Node::GetDefaultValue(const QString &input) const
{
  const Input* i = GetInternalInputData(input);

  if (i) {
    return i->default_value;
  } else {
    ReportInvalidInput("retrieve default value of", input, -1);
    return value_t();
  }
}

value_t::component_t Node::GetSplitDefaultValueOnTrack(const QString &input, size_t track) const
{
  value_t val = GetDefaultValue(input);
  if (track < val.size()) {
    return val.at(track);
  } else {
    return value_t::component_t();
  }
}

void Node::SetDefaultValue(const QString &input, const value_t &val)
{
  Input* i = GetInternalInputData(input);

  if (i) {
    i->default_value = val;
  } else {
    ReportInvalidInput("set default value of", input, -1);
  }
}

void Node::SetSplitDefaultValueOnTrack(const QString &input, const value_t::component_t &val, size_t track)
{
  Input* i = GetInternalInputData(input);

  if (i) {
    if (track < i->default_value.size()) {
      i->default_value[track] = val;
    }
  } else {
    ReportInvalidInput("set default value on track of", input, -1);
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
    ReportInvalidInput("get keyframes at time from", input, element);
    return QVector<NodeKeyframe*>();
  }
}

NodeKeyframe *Node::GetKeyframeAtTimeOnTrack(const QString &input, const rational &time, int track, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->get_keyframe_at_time_on_track(time, track);
  } else {
    ReportInvalidInput("get keyframe at time on track from", input, element);
    return nullptr;
  }
}

NodeKeyframe::Type Node::GetBestKeyframeTypeForTimeOnTrack(const QString &input, const rational &time, int track, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->get_best_keyframe_type_for_time(time, track);
  } else {
    ReportInvalidInput("get closest keyframe before a time from", input, element);
    return NodeKeyframe::kDefaultType;
  }
}

size_t Node::GetNumberOfKeyframeTracks(const QString &id) const
{
  const Input* i = GetInternalInputData(id);
  if (i) {
    return i->channel_count;
  } else {
    ReportInvalidInput("get number of keyframe tracks of", id, -1);
    return 1;
  }
}

NodeKeyframe *Node::GetEarliestKeyframe(const QString &id, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_earliest_keyframe();
  } else {
    ReportInvalidInput("get earliest keyframe from", id, element);
    return nullptr;
  }
}

NodeKeyframe *Node::GetLatestKeyframe(const QString &id, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_latest_keyframe();
  } else {
    ReportInvalidInput("get latest keyframe from", id, element);
    return nullptr;
  }
}

NodeKeyframe *Node::GetClosestKeyframeBeforeTime(const QString &id, const rational &time, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_closest_keyframe_before_time(time);
  } else {
    ReportInvalidInput("get closest keyframe before a time from", id, element);
    return nullptr;
  }
}

NodeKeyframe *Node::GetClosestKeyframeAfterTime(const QString &id, const rational &time, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->get_closest_keyframe_after_time(time);
  } else {
    ReportInvalidInput("get closest keyframe after a time from", id, element);
    return nullptr;
  }
}

bool Node::HasKeyframeAtTime(const QString &id, const rational &time, int element) const
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return imm->has_keyframe_at_time(time);
  } else {
    ReportInvalidInput("determine if it has a keyframe at a time from", id, element);
    return false;
  }
}

QStringList Node::GetComboBoxStrings(const QString &id) const
{
  return GetInputProperty(id, QStringLiteral("combo_str")).value<QStringList>();
}

value_t Node::GetStandardValue(const QString &id, int element) const
{
  const Input *input = GetInternalInputData(id);
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    return value_t(input->type, imm->get_standard_value());
  } else {
    ReportInvalidInput("get standard value of", id, element);
    return value_t();
  }
}

value_t::component_t Node::GetSplitStandardValueOnTrack(const QString &input, int track, int element) const
{
  NodeInputImmediate* imm = GetImmediate(input, element);

  if (imm) {
    return imm->get_split_standard_value_on_track(track);
  } else {
    ReportInvalidInput("get standard value of", input, element);
    return value_t::component_t();
  }
}

void Node::SetStandardValue(const QString &id, const value_t &value, int element)
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    imm->set_split_standard_value(value);

    for (size_t i=0; i<value.size(); i++) {
      if (IsUsingStandardValue(id, i, element)) {
        // If this standard value is being used, we need to send a value changed signal
        ParameterValueChanged(id, element, TimeRange(RATIONAL_MIN, RATIONAL_MAX));
        break;
      }
    }
  } else {
    ReportInvalidInput("set standard value of", id, element);
  }
}

void Node::SetSplitStandardValueOnTrack(const QString &id, int track, const value_t::component_t &value, int element)
{
  NodeInputImmediate* imm = GetImmediate(id, element);

  if (imm) {
    imm->set_standard_value_on_track(value, track);

    if (IsUsingStandardValue(id, track, element)) {
      // If this standard value is being used, we need to send a value changed signal
      ParameterValueChanged(id, element, TimeRange(RATIONAL_MIN, RATIONAL_MAX));
    }
  } else {
    ReportInvalidInput("set standard value of", id, element);
  }
}

bool Node::InputIsArray(const QString &id) const
{
  return GetInputFlags(id) & kInputFlagArray;
}

void Node::InputArrayInsert(const QString &id, int index)
{
  // Add new input
  ArrayResizeInternal(id, InputArraySize(id) + 1);

  // Move connections down
  Connections copied_edges = input_connections();
  for (auto it=copied_edges.crbegin(); it!=copied_edges.crend(); it++) {
    if (it->second.input() == id && it->second.element() >= index) {
      // Disconnect this and reconnect it one element down
      NodeInput new_edge = it->second;
      new_edge.set_element(new_edge.element() + 1);

      DisconnectEdge(it->first, it->second);
      ConnectEdge(it->first, new_edge);
    }
  }

  // Shift values and keyframes up one element
  for (int i=InputArraySize(id)-1; i>index; i--) {
    CopyValuesOfElement(this, this, id, i-1, i);
  }

  // Reset value of element we just "inserted"
  ClearElement(id, index);
}

void Node::InputArrayResize(const QString &id, int size)
{
  if (InputArraySize(id) == size) {
    return;
  }

  NodeArrayResizeCommand* c = new NodeArrayResizeCommand(this, id, size);
  c->redo_now();
  delete c;
}

void Node::InputArrayRemove(const QString &id, int index)
{
  // Remove input
  ArrayResizeInternal(id, InputArraySize(id) - 1);

  // Move connections up
  Connections copied_edges = input_connections();
  for (auto it=copied_edges.cbegin(); it!=copied_edges.cend(); it++) {
    if (it->second.input() == id && it->second.element() >= index) {
      // Disconnect this and reconnect it one element up if it's not the element being removed
      DisconnectEdge(it->first, it->second);

      if (it->second.element() > index) {
        NodeInput new_edge = it->second;
        new_edge.set_element(new_edge.element() - 1);

        ConnectEdge(it->first, new_edge);
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

int Node::InputArraySize(const QString &id) const
{
  const Input* i = GetInternalInputData(id);

  if (i) {
    return i->array_size;
  } else {
    ReportInvalidInput("retrieve array size of", id, -1);
    return 0;
  }
}

value_t Node::GetInputValue(const ValueParams &g, const QString &input, int element, bool autoconversion) const
{
  if (!g.is_cancelled()) {
    NodeOutput output = GetConnectedOutput2(input, element);
    if (output.IsValid()) {
      return GetFakeConnectedValue(g, output, input, element, autoconversion);
    } else {
      TimeRange adjusted_time = InputTimeAdjustment(input, element, g.time(), true);

      if (element == -1 && InputIsArray(input)) {
        NodeValueArray array(InputArraySize(input));

        for (size_t i = 0; i < array.size(); i++) {
          array[i] = GetValueAtTime(input, adjusted_time.in(), i);
        }

        return array;
      } else {
        return GetValueAtTime(input, adjusted_time.in(), element);
      }
    }
  }

  return value_t();
}

void ConvertDoubleToSamples(const void *context, const SampleJob &input, SampleBuffer &output)
{
  const Node *n = static_cast<const Node *>(context);

  const QString param = input.Get(QStringLiteral("input")).toString();
  const int element = input.Get(QStringLiteral("element")).toInt();
  const TimeRange &time = input.value_params().time();

  for (size_t i = 0; i < output.sample_count(); i++) {
    TimeRange this_sample_time = time + output.audio_params().sample_rate_as_time_base() * i;
    value_t v = n->GetInputValue(input.value_params().time_transformed(this_sample_time), param, element, false);
    for (int j = 0; j < output.channel_count(); j++) {
      output.data(j)[i] = v.toDouble();
    }
  }
}

value_t Node::GetFakeConnectedValue(const ValueParams &g, NodeOutput output, const QString &input, int element, bool autoconversion) const
{
  if (!g.is_cancelled()) {
    TimeRange adjusted_time = InputTimeAdjustment(input, element, g.time(), true);

    while (output.IsValid()) {
      if (output.node()->is_enabled()) {
        Node::ValueHint vh = this->GetValueHintForInput(input, element);
        ValueParams connp = g.time_transformed(adjusted_time).output_edited(output.output());

        // Find cached value with this
        value_t v;
        if (!g.get_cached_value(output.node(), connp, v)) {
          v = output.node()->Value(connp);
          g.insert_cached_value(output.node(), connp, v);
        }

        // Perform swizzle if requested
        const SwizzleMap &swizzle = vh.swizzle();
        if (!swizzle.empty()) {
          value_t swiz(v.type(), v.size());
          for (auto it = swizzle.cbegin(); it != swizzle.cend(); it++) {
            size_t from = it->second;
            size_t to = it->first;

            if (from < v.size()) {
              if (to >= swiz.size()) {
                swiz.resize(to + 1);
              }

              swiz[to] = v[from];
            }
          }
          v = swiz;
        }

        // Perform conversion if necessary
        type_t expected_type = this->GetInputDataType(input);
        if (autoconversion && expected_type != TYPE_NONE && expected_type != v.type() && !(this->GetInputFlags(input) & kInputFlagDontAutoConvert)) {
          if (v.type() == TYPE_DOUBLE && expected_type == TYPE_SAMPLES) {
            // Create a job to generate audio from numbers
            SampleJob job(g);

            job.Insert(QStringLiteral("input"), input);
            job.Insert(QStringLiteral("element"), element);

            job.set_function(ConvertDoubleToSamples, this);

            v = job;
          } else {
            bool ok;
            v = v.converted(expected_type, &ok);
            if (!ok) {
              // Return null value instead of converted value because node is probably not set up to
              // handle this type (unless kInputFlagDontAutoConvert is specified of course)
              v = value_t();
            }
          }
        }

        return v;
      } else {
        output = output.node()->GetConnectedOutput2(output.node()->GetEffectInput());
      }
    }
  }

  return value_t();
}

void Node::SetValueHintForInput(const QString &input, const ValueHint &hint, int element)
{
  value_hints_.insert({input, element}, hint);

  emit InputValueHintChanged(NodeInput(this, input, element));

  InvalidateAll(input, element);
}

AbstractParamWidget *Node::GetCustomWidget(const QString &input) const
{
  return nullptr;
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

InputFlag Node::GetInputFlags(const QString &input) const
{
  const Input* i = GetInternalInputData(input);

  if (i) {
    return i->flags;
  } else {
    ReportInvalidInput("retrieve flags of", input, -1);
    return kInputFlagNormal;
  }
}

void Node::SetInputFlag(const QString &input, InputFlag f, bool on)
{
  Input* i = GetInternalInputData(input);

  if (i) {
    if (on) {
      i->flags |= f;
    } else {
      i->flags &= ~f;
    }
    emit InputFlagsChanged(input, i->flags);
  } else {
    ReportInvalidInput("set flags of", input, -1);
  }
}

void Node::InvalidateCache(const TimeRange &range, const QString &from, int element, InvalidateCacheOptions options)
{
  Q_UNUSED(from)
  Q_UNUSED(element)

  if (AreCachesEnabled()) {
    if (range.in() != range.out()) {
      TimeRange vr = range.Intersected(GetVideoCacheRange());
      if (vr.length() != 0) {
        video_frame_cache()->Invalidate(vr);
        thumbnail_cache()->Invalidate(vr);
      }
      TimeRange ar = range.Intersected(GetAudioCacheRange());
      if (ar.length() != 0) {
        audio_playback_cache()->Invalidate(ar);
        waveform_cache()->Invalidate(ar);
      }
    }
  }

  SendInvalidateCache(range, options);
}

TimeRange Node::InputTimeAdjustment(const QString &, int, const TimeRange &input_time, bool clamp) const
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
    Project* graph = nodes.at(i)->parent();
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
      int connection_index = src.indexOf(it->first.node());

      if (connection_index > -1) {
        // Find the equivalent node in the dst list
        NodeOutput copied_output = NodeOutput(dst.at(connection_index), it->first.output());
        NodeInput copied_input = NodeInput(dst_node, it->second.input(), it->second.element());

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

  // Add it to the same graph
  command->add_child(new NodeAddCommand(node->parent(), copy));

  // Copy context children
  const PositionMap &map = node->GetContextPositions();
  for (auto it=map.cbegin(); it!=map.cend(); it++) {
    // Add either the copy (if it exists) or the original node to the context
    Node *child;

    if (it.key()->IsItem()) {
      child = it.key();
    } else {
      child = created.value(it.key());
      if (!child) {
        child = CopyNodeAndDependencyGraphMinusItemsInternal(created, it.key(), command);
      }
    }

    command->add_child(new NodeSetPositionCommand(child, copy, it.value()));
  }

  // If this is a group, copy input and output passthroughs
  if (NodeGroup *src_group = dynamic_cast<NodeGroup*>(node)) {
    NodeGroup *dst_group = static_cast<NodeGroup*>(copy);

    for (auto it=src_group->GetInputPassthroughs().cbegin(); it!=src_group->GetInputPassthroughs().cend(); it++) {
      // This node should have been created by the context loop above
      NodeInput input = it->second;
      input.set_node(created.value(input.node()));
      command->add_child(new NodeGroupAddInputPassthrough(dst_group, input, it->first));
    }

    command->add_child(new NodeGroupSetOutputPassthrough(dst_group, created.value(src_group->GetOutputPassthrough())));
  }

  // Copy values to the clone
  CopyInputs(node, copy, false, command);

  // Go through input connections and copy if non-item and connect if item
  for (auto it=node->input_connections_.cbegin(); it!=node->input_connections_.cend(); it++) {
    NodeInput input = it->second;
    NodeOutput output = it->first;
    //Node* connected = it->first.node();
    Node* connected_copy;

    if (output.node()->IsItem()) {
      // This is an item and we avoid copying those and just connect to them directly
      connected_copy = output.node();
    } else {
      // Non-item, we want to clone this too
      connected_copy = created.value(output.node(), nullptr);
      if (!connected_copy) {
        connected_copy = CopyNodeAndDependencyGraphMinusItemsInternal(created, output.node(), command);
      }
    }

    NodeInput copied_input = input;
    copied_input.set_node(copy);

    NodeOutput copied_output(connected_copy, output.output());

    command->add_child(new NodeEdgeAddCommand(copied_output, copied_input));
    command->add_child(new NodeSetValueHintCommand(copied_input, node->GetValueHintForInput(input.input(), input.element())));
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

  if (OLIVE_CONFIG("SplitClipsCopyNodes").toBool()) {
    copy = Node::CopyNodeAndDependencyGraphMinusItems(node, command);
  } else {
    copy = node->copy();

    command->add_child(new NodeAddCommand(node->parent(), copy));

    CopyInputs(node, copy, true, command);

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
  for (const Connection& conn : output_connections_) {
    // Send clear cache signal to the Node
    const NodeInput& in = conn.second;

    in.node()->InvalidateCache(range, in.input(), in.element(), options);
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

bool Node::Load(QXmlStreamReader *reader, SerializedData *data)
{
  uint version = 0;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("ptr")) {
      quintptr ptr = attr.value().toULongLong();
      data->node_ptrs.insert(ptr, this);
    } else if (attr.name() == QStringLiteral("version")) {
      version = attr.value().toUInt();
    }
  }

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("input")) {
      LoadInput(reader, data);
    } else if (reader->name() == QStringLiteral("label")) {
      this->SetLabel(reader->readElementText());
    } else if (reader->name() == QStringLiteral("color")) {
      this->SetOverrideColor(reader->readElementText().toInt());
    } else if (reader->name() == QStringLiteral("links")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("link")) {
          data->block_links.append({this, reader->readElementText().toULongLong()});
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("custom")) {
      if (!LoadCustom(reader, data)) {
        return false;
      }
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

          QString output_node_id, output_param;

          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("output")) {
              if (version < 2) {
                output_node_id = reader->readElementText();
              } else {
                while (XMLReadNextStartElement(reader)) {
                  if (reader->name() == QStringLiteral("node")) {
                    output_node_id = reader->readElementText();
                  } else if (reader->name() == QStringLiteral("param")) {
                    output_param = reader->readElementText();
                  } else {
                    reader->skipCurrentElement();
                  }
                }
              }
            } else {
              reader->skipCurrentElement();
            }
          }

          data->desired_connections.append({NodeInput(this, param_id, ele), output_node_id.toULongLong(), output_param});
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("hints")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("hint")) {
          QString input;
          int element = -1;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("input")) {
              input = attr.value().toString();
            } else if (attr.name() == QStringLiteral("element")) {
              element = attr.value().toInt();
            }
          }

          Node::ValueHint vh;
          if (!vh.load(reader)) {
            return false;
          }
          this->SetValueHintForInput(input, vh, element);
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("context")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("node")) {
          quintptr node_ptr = 0;

          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("ptr")) {
              node_ptr = attr.value().toULongLong();
            }
          }

          if (node_ptr) {
            Node::Position node_pos;
            if (!node_pos.load(reader)) {
              return false;
            }
            data->positions[this].insert(node_ptr, node_pos);
          } else {
            return false;
          }
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("caches")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("audio")) {
          this->audio_playback_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
        } else if (reader->name() == QStringLiteral("video")) {
          this->video_frame_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
        } else if (reader->name() == QStringLiteral("thumb")) {
          this->thumbnail_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
        } else if (reader->name() == QStringLiteral("waveform")) {
          this->waveform_cache()->SetUuid(QUuid::fromString(reader->readElementText()));
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  this->LoadFinishedEvent();

  return true;
}

void Node::Save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("version"), QString::number(2));
  writer->writeAttribute(QStringLiteral("id"), this->id());
  writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(this)));

  if (!this->GetLabel().isEmpty()) {
    writer->writeTextElement(QStringLiteral("label"), this->GetLabel());
  }

  if (this->GetOverrideColor() != -1) {
    writer->writeTextElement(QStringLiteral("color"), QString::number(this->GetOverrideColor()));
  }

  foreach (const QString& input, this->inputs()) {
    writer->writeStartElement(QStringLiteral("input"));

    SaveInput(writer, input);

    writer->writeEndElement(); // input
  }

  if (!this->links().empty()) {
    writer->writeStartElement(QStringLiteral("links"));
    foreach (Node* link, this->links()) {
      writer->writeTextElement(QStringLiteral("link"), QString::number(reinterpret_cast<quintptr>(link)));
    }
    writer->writeEndElement(); // links
  }

  if (!this->input_connections().empty()) {
    writer->writeStartElement(QStringLiteral("connections"));
    for (auto it=this->input_connections().cbegin(); it!=this->input_connections().cend(); it++) {
      writer->writeStartElement(QStringLiteral("connection"));

      writer->writeAttribute(QStringLiteral("input"), it->second.input());
      writer->writeAttribute(QStringLiteral("element"), QString::number(it->second.element()));

      writer->writeStartElement(QStringLiteral("output"));
      writer->writeTextElement(QStringLiteral("node"), QString::number(reinterpret_cast<quintptr>(it->first.node())));
      writer->writeTextElement(QStringLiteral("param"), it->first.output());
      writer->writeEndElement(); // output

      writer->writeEndElement(); // connection
    }
    writer->writeEndElement(); // connections
  }

  if (!this->GetValueHints().empty()) {
    writer->writeStartElement(QStringLiteral("hints"));
    for (auto it=this->GetValueHints().cbegin(); it!=this->GetValueHints().cend(); it++) {
      writer->writeStartElement(QStringLiteral("hint"));

      writer->writeAttribute(QStringLiteral("input"), it.key().input);
      writer->writeAttribute(QStringLiteral("element"), QString::number(it.key().element));

      it.value().save(writer);

      writer->writeEndElement(); // hint
    }
    writer->writeEndElement(); // hints
  }

  const Node::PositionMap &map = this->GetContextPositions();

  if (!map.isEmpty()) {
    writer->writeStartElement(QStringLiteral("context"));

    for (auto jt=map.cbegin(); jt!=map.cend(); jt++) {
      writer->writeStartElement(QStringLiteral("node"));
      writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(jt.key())));
      jt.value().save(writer);
      writer->writeEndElement(); // node
    }

    writer->writeEndElement(); // context
  }

  writer->writeStartElement(QStringLiteral("caches"));

  writer->writeTextElement(QStringLiteral("audio"), this->audio_playback_cache()->GetUuid().toString());
  writer->writeTextElement(QStringLiteral("video"), this->video_frame_cache()->GetUuid().toString());
  writer->writeTextElement(QStringLiteral("thumb"), this->thumbnail_cache()->GetUuid().toString());
  writer->writeTextElement(QStringLiteral("waveform"), this->waveform_cache()->GetUuid().toString());

  writer->writeEndElement(); // caches

  writer->writeStartElement(QStringLiteral("custom"));

  SaveCustom(writer);

  writer->writeEndElement(); // custom
}

bool Node::LoadCustom(QXmlStreamReader *reader, SerializedData *data)
{
  reader->skipCurrentElement();
  return true;
}

void Node::PostLoadEvent(SerializedData *data)
{
  // Resolve positions
  const QMap<quintptr, Node::Position> &positions = data->positions.value(this);

  for (auto jt=positions.cbegin(); jt!=positions.cend(); jt++) {
    Node *n = data->node_ptrs.value(jt.key());
    if (n) {
      this->SetNodePositionInContext(n, jt.value());
    }
  }
}

bool Node::LoadInput(QXmlStreamReader *reader, SerializedData *data)
{
  if (dynamic_cast<NodeGroup*>(this)) {
    // Ignore input of group
    reader->skipCurrentElement();
    return true;
  }

  QString param_id;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("id")) {
      param_id = attr.value().toString();

      break;
    }
  }

  if (param_id.isEmpty()) {
    qWarning() << "Failed to load parameter with missing ID";
    reader->skipCurrentElement();
    return false;
  }

  if (!this->HasInputWithID(param_id)) {
    qWarning() << "Failed to load parameter that didn't exist:" << param_id;
    reader->skipCurrentElement();
    return false;
  }

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("primary")) {
      // Load primary immediate
      if (!LoadImmediate(reader, param_id, -1, data)) {
        return false;
      }
    } else if (reader->name() == QStringLiteral("subelements")) {
      // Load subelements
      XMLAttributeLoop(reader, attr) {
        if (attr.name() == QStringLiteral("count")) {
          this->InputArrayResize(param_id, attr.value().toInt());
        }
      }

      int element_counter = 0;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("element")) {
          if (!LoadImmediate(reader, param_id, element_counter, data)) {
            return false;
          }

          element_counter++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else {
      reader->skipCurrentElement();
    }
  }

  return true;
}

void Node::SaveInput(QXmlStreamWriter *writer, const QString &id) const
{
  writer->writeAttribute(QStringLiteral("id"), id);

  writer->writeStartElement(QStringLiteral("primary"));

  SaveImmediate(writer, id, -1);

  writer->writeEndElement(); // primary

  int arr_sz = this->InputArraySize(id);

  if (arr_sz > 0) {
    writer->writeStartElement(QStringLiteral("subelements"));

    writer->writeAttribute(QStringLiteral("count"), QString::number(arr_sz));

    for (int i=0; i<arr_sz; i++) {
      writer->writeStartElement(QStringLiteral("element"));

      SaveImmediate(writer, id, i);

      writer->writeEndElement(); // element
    }

    writer->writeEndElement(); // subelements
  }
}

bool Node::LoadImmediate(QXmlStreamReader *reader, const QString &input, int element, SerializedData *data)
{
  type_t data_type = this->GetInputDataType(input);

  // HACK: SubtitleParams contain the actual subtitle data, so loading/replacing it will overwrite
  //       the valid subtitles. We hack around it by simply skipping loading subtitles, we'll see
  //       if this ends up being an issue in the future.
  if (data_type == ViewerOutput::TYPE_SPARAM) {
    reader->skipCurrentElement();
    return true;
  }

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("standard")) {
      // Load standard value
      int val_index = 0;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("track")) {
          value_t::component_t value_on_track;

          if (data_type == ViewerOutput::TYPE_VPARAM) {
            VideoParams vp;
            vp.Load(reader);
            value_on_track = vp;
          } else if (data_type == ViewerOutput::TYPE_APARAM) {
            AudioParams ap = TypeSerializer::LoadAudioParams(reader);
            value_on_track = ap;
          } else {
            QString value_text = reader->readElementText();

            if (!value_text.isEmpty()) {
              value_on_track = value_t::component_t(value_text).converted(TYPE_STRING, data_type);
            }
          }

          this->SetSplitStandardValueOnTrack(input, val_index, value_on_track, element);

          val_index++;
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("keyframing")) {
      bool k = reader->readElementText().toInt();
      if (this->IsInputKeyframable(input)) {
        this->SetInputIsKeyframing(input, k, element);
      }
    } else if (reader->name() == QStringLiteral("keyframes")) {
      int track = 0;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("track")) {
          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("key")) {
              NodeKeyframe* key = new NodeKeyframe();
              key->set_input(input);
              key->set_element(element);
              key->set_track(track);

              key->load(reader, data_type);
              key->setParent(this);
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
      this->SetInputProperty(input, QStringLiteral("col_input"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("csdisplay")) {
      this->SetInputProperty(input, QStringLiteral("col_display"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("csview")) {
      this->SetInputProperty(input, QStringLiteral("col_view"), reader->readElementText());
    } else if (reader->name() == QStringLiteral("cslook")) {
      this->SetInputProperty(input, QStringLiteral("col_look"), reader->readElementText());
    } else {
      reader->skipCurrentElement();
    }
  }

  return true;
}

void Node::SaveImmediate(QXmlStreamWriter *writer, const QString &input, int element) const
{
  bool is_keyframing = this->IsInputKeyframing(input, element);

  if (this->IsInputKeyframable(input)) {
    writer->writeTextElement(QStringLiteral("keyframing"), QString::number(is_keyframing));
  }

  type_t data_type = this->GetInputDataType(input);

  // Write standard value
  writer->writeStartElement(QStringLiteral("standard"));

  value_t value = this->GetStandardValue(input, element);
  for (size_t i = 0; i < value.size(); i++) {
    const value_t::component_t &v = value.at(i);

    writer->writeStartElement(QStringLiteral("track"));

    // FIXME: We now have converters for custom types, so this should be handled with those instead.
    //        It'll probably require bumping the project version though...
    if (data_type == ViewerOutput::TYPE_VPARAM) {
      v.value<VideoParams>().Save(writer);
    } else if (data_type == ViewerOutput::TYPE_APARAM) {
      TypeSerializer::SaveAudioParams(writer, v.value<AudioParams>());
    } else {
      writer->writeCharacters(v.toSerializedString(data_type));
    }

    writer->writeEndElement(); // track
  }

  writer->writeEndElement(); // standard

  // Write keyframes
  if (is_keyframing) {
    writer->writeStartElement(QStringLiteral("keyframes"));

    for (const NodeKeyframeTrack& track : this->GetKeyframeTracks(input, element)) {
      writer->writeStartElement(QStringLiteral("track"));

      for (NodeKeyframe* key : track) {
        writer->writeStartElement(QStringLiteral("key"));

        key->save(writer, data_type);

        writer->writeEndElement(); // key
      }

      writer->writeEndElement(); // track
    }

    writer->writeEndElement(); // keyframes
  }

  if (this->HasInputProperty(input, QStringLiteral("col_input"))) {
    // Save color management information
    writer->writeTextElement(QStringLiteral("csinput"), this->GetInputProperty(input, QStringLiteral("col_input")).toString());
    writer->writeTextElement(QStringLiteral("csdisplay"), this->GetInputProperty(input, QStringLiteral("col_display")).toString());
    writer->writeTextElement(QStringLiteral("csview"), this->GetInputProperty(input, QStringLiteral("col_view")).toString());
    writer->writeTextElement(QStringLiteral("cslook"), this->GetInputProperty(input, QStringLiteral("col_look")).toString());
  }
}

void Node::InsertInput(const QString &id, type_t type, size_t channel_count, const value_t &default_value, InputFlag flags, int index)
{
  if (id.isEmpty()) {
    qWarning() << "Rejected adding input with an empty ID on node" << this->id();
    return;
  }

  if (HasInputWithID(id)) {
    qWarning() << "Failed to add input to node" << this->id() << "- ID" << id << "already exists";
    return;
  }

  QString subtype;

  type = ResolveSpecialType(type, channel_count, subtype);

  Node::Input i;

  i.type = type;
  i.default_value = default_value;
  i.flags = flags;
  i.array_size = 0;
  i.channel_count = channel_count;

  //qDebug() << "creating" << id << "with channels" << i.channel_count;

  if (!subtype.isEmpty()) {
    i.properties.insert(QStringLiteral("subtype"), subtype);
  }

  input_ids_.insert(index, id);
  input_data_.insert(index, i);

  if (!standard_immediates_.value(id, nullptr)) {
    NodeInputImmediate *imm = CreateImmediate(id);
    standard_immediates_.insert(id, imm);
    if (default_value.isValid()) {
      imm->set_split_standard_value(default_value);
    }
  }

  emit InputAdded(id);
}

void Node::RemoveInput(const QString &id)
{
  int index = input_ids_.indexOf(id);

  if (index == -1) {
    ReportInvalidInput("remove", id, -1);
    return;
  }

  input_ids_.removeAt(index);
  input_data_.removeAt(index);

  emit InputRemoved(id);
}

void Node::AddOutput(const QString &id)
{
  for (auto it = outputs_.constBegin(); it != outputs_.constEnd(); it++) {
    if (it->id == id) {
      qWarning() << "Failed to add output to node" << this->id() << "- ID" << id << "already exists";
      return;
    }
  }

  Output o;
  o.id = id;
  outputs_.append(o);

  emit OutputAdded(id);
}

void Node::RemoveOutput(const QString &id)
{
  for (auto it = outputs_.begin(); it != outputs_.end(); it++) {
    if (it->id == id) {
      outputs_.erase(it);
      break;
    }
  }

  emit OutputRemoved(id);
}

void Node::ReportInvalidInput(const char *attempted_action, const QString& id, int element) const
{
  qWarning() << "Failed to" << attempted_action << "parameter" << id << "element" << element
             << "in node" << this->id() << "- input doesn't exist";
}

NodeInputImmediate *Node::CreateImmediate(const QString &input)
{
  const Input* i = GetInternalInputData(input);

  if (i) {
    return new NodeInputImmediate(i->type, i->channel_count);
  } else {
    ReportInvalidInput("create immediate", input, -1);
    return nullptr;
  }
}

void Node::ArrayResizeInternal(const QString &id, size_t size)
{
  Input* data = GetInternalInputData(id);

  if (!data) {
    ReportInvalidInput("set array size", id, -1);
    return;
  }

  if (data->array_size != size) {
    // Update array size
    if (data->array_size < size) {
      // Size is larger, create any immediates that don't exist
      QVector<NodeInputImmediate*>& subinputs = array_immediates_[id];
      for (size_t i=subinputs.size(); i<size; i++) {
        NodeInputImmediate *imm = CreateImmediate(id);
        imm->set_split_standard_value(data->default_value);
        subinputs.append(imm);
      }

      // Note that we do not delete any immediates when decreasing size since the user might still
      // want that data. Therefore it's important to note that array_size_ does NOT necessarily
      // equal subinputs_.size()
    }

    int old_sz = data->array_size;
    data->array_size = size;
    emit InputArraySizeChanged(id, old_sz, size);
    ParameterValueChanged(id, -1, TimeRange(RATIONAL_MIN, RATIONAL_MAX));
  }
}

QString Node::GetConnectCommandString(const NodeOutput &output, const NodeInput &input)
{
  return tr("Connected %1 to %2 - %3").arg(output.node()->GetLabelAndName(), input.node()->GetLabelAndName(), input.GetInputName());
}

QString Node::GetDisconnectCommandString(const NodeOutput &output, const NodeInput &input)
{
  return tr("Disconnected %1 from %2 - %3").arg(output.node()->GetLabelAndName(), input.node()->GetLabelAndName(), input.GetInputName());
}

int Node::GetInternalInputArraySize(const QString &input)
{
  return array_immediates_.value(input).size();
}

void FindWaysNodeArrivesHereRecursively(const Node *output, const Node *input, QVector<NodeInput> &v)
{
  for (auto it=input->input_connections().cbegin(); it!=input->input_connections().cend(); it++) {
    if (it->first.node() == output) {
      v.append(it->second);
    } else {
      FindWaysNodeArrivesHereRecursively(output, it->first.node(), v);
    }
  }
}

QVector<NodeInput> Node::FindWaysNodeArrivesHere(const Node *output) const
{
  QVector<NodeInput> v;

  FindWaysNodeArrivesHereRecursively(output, this, v);

  return v;
}

void Node::SetInputName(const QString &id, const QString &name)
{
  Input* i = GetInternalInputData(id);

  if (i) {
    i->human_name = name;

    emit InputNameChanged(id, name);
  } else {
    ReportInvalidInput("set name of", id, -1);
  }
}

void Node::SetOutputName(const QString &id, const QString &name)
{
  for (auto it = outputs_.begin(); it != outputs_.end(); it++) {
    if (it->id == id) {
      it->name = name;
      break;
    }
  }
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

void Node::CopyInputs(const Node *source, Node *destination, bool include_connections, MultiUndoCommand *command)
{
  Q_ASSERT(source->id() == destination->id());

  foreach (const QString& input, source->inputs()) {
    // NOTE: This assert is to ensure that inputs in the source also exist in the destination, which
    //       they should. If they don't and you hit this assert, check if you're handling group
    //       passthroughs correctly.
    Q_ASSERT(destination->HasInputWithID(input));

    CopyInput(source, destination, input, include_connections, true, command);
  }

  if (command) {
    command->add_child(new NodeRenameCommand(destination, source->GetLabel()));
  } else {
    destination->SetLabel(source->GetLabel());
  }

  if (command) {
    command->add_child(new NodeOverrideColorCommand(destination, source->GetOverrideColor()));
  } else {
    destination->SetOverrideColor(source->GetOverrideColor());
  }
}

void Node::CopyInput(const Node *src, Node *dst, const QString &input, bool include_connections, bool traverse_arrays, MultiUndoCommand *command)
{
  Q_ASSERT(src->id() == dst->id());

  CopyValuesOfElement(src, dst, input, -1, command);

  // Copy array size
  if (src->InputIsArray(input) && traverse_arrays) {
    int src_array_sz = src->InputArraySize(input);

    for (int i=0; i<src_array_sz; i++) {
      CopyValuesOfElement(src, dst, input, i, command);
    }
  }

  // Copy connections
  if (include_connections) {
    // Copy all connections
    for (auto it=src->input_connections().cbegin(); it!=src->input_connections().cend(); it++) {
      if (!traverse_arrays && it->second.element() != -1) {
        continue;
      }

      auto conn_output = it->first;
      NodeInput conn_input(dst, input, it->second.element());

      if (command) {
        command->add_child(new NodeEdgeAddCommand(conn_output, conn_input));
      } else {
        ConnectEdge(conn_output, conn_input);
      }
    }
  }
}

void Node::CopyValuesOfElement(const Node *src, Node *dst, const QString &input, int src_element, int dst_element, MultiUndoCommand *command)
{
  if (dst_element >= dst->GetInternalInputArraySize(input)) {
    qDebug() << "Ignored destination element that was out of array bounds";
    return;
  }

  NodeInput dst_input(dst, input, dst_element);

  // Copy standard value
  value_t standard = src->GetStandardValue(input, src_element);
  if (command) {
    command->add_child(new NodeParamSetSplitStandardValueCommand(dst_input, standard));
  } else {
    dst->SetStandardValue(input, standard, dst_element);
  }

  // Copy keyframes
  if (NodeInputImmediate *immediate = dst->GetImmediate(input, dst_element)) {
    if (command) {
      command->add_child(new NodeImmediateRemoveAllKeyframesCommand(immediate));
    } else {
      immediate->delete_all_keyframes();
    }
  }

  for (const NodeKeyframeTrack& track : src->GetImmediate(input, src_element)->keyframe_tracks()) {
    for (NodeKeyframe* key : track) {
      NodeKeyframe *copy = key->copy(dst_element, command ? nullptr : dst);
      if (command) {
        command->add_child(new NodeParamInsertKeyframeCommand(dst, copy));
      }
    }
  }

  // Copy keyframing state
  if (src->IsInputKeyframable(input)) {
    bool is_keying = src->IsInputKeyframing(input, src_element);
    if (command) {
      command->add_child(new NodeParamSetKeyframingCommand(dst_input, is_keying));
    } else {
      dst->SetInputIsKeyframing(input, is_keying, dst_element);
    }
  }

  // If this is the root of an array, copy the array size
  if (src_element == -1 && dst_element == -1) {
    int array_sz = src->InputArraySize(input);
    if (command) {
      command->add_child(new NodeArrayResizeCommand(dst, input, array_sz));
    } else {
      dst->ArrayResizeInternal(input, array_sz);
    }
  }

  // Copy value hint
  Node::ValueHint vh = src->GetValueHintForInput(input, src_element);
  if (command) {
    command->add_child(new NodeSetValueHintCommand(dst_input, vh));
  } else {
    dst->SetValueHintForInput(input, vh, dst_element);
  }
}

void GetDependenciesRecursively(QVector<Node*>& list, const Node* node, bool traverse, bool exclusive_only)
{
  for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
    Node* connected_node = it->first.node();

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

bool Node::InputsFrom(Node *n, bool recursively) const
{
  for (auto it=input_connections_.cbegin(); it!=input_connections_.cend(); it++) {
    Node *connected = it->first.node();

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
    Node *connected = it->first.node();

    if (connected->id() == id) {
      return true;
    } else if (recursively && connected->InputsFrom(id, recursively)) {
      return true;
    }
  }

  return false;
}

void Node::DisconnectAll()
{
  // Disconnect inputs (copy map since internal map will change as we disconnect)
  Connections copy = input_connections_;
  for (auto it=copy.cbegin(); it!=copy.cend(); it++) {
    DisconnectEdge(it->first, it->second);
  }

  while (!output_connections_.empty()) {
    Connection conn = output_connections_.back();
    DisconnectEdge(conn.first, conn.second);
  }
}

QString Node::GetCategoryName(const CategoryID &c)
{
  switch (c) {
  case kCategoryOutput:
    return tr("Output");
  case kCategoryDistort:
    return tr("Distort");
  case kCategoryMath:
    return tr("Math");
  case kCategoryKeying:
    return tr("Keying");
  case kCategoryColor:
    return tr("Color");
  case kCategoryFilter:
    return tr("Filter");
  case kCategoryTimeline:
    return tr("Timeline");
  case kCategoryGenerator:
    return tr("Generator");
  case kCategoryTransition:
    return tr("Transition");
  case kCategoryProject:
    return tr("Project");
  case kCategoryTime:
    return tr("Time");
  case kCategoryUnknown:
  case kCategoryCount:
    break;
  }

  return tr("Uncategorized");
}

TimeRange Node::TransformTimeTo(TimeRange time, Node *target, TransformTimeDirection dir, int path_index)
{
  Node *from = this;
  Node *to = target;

  if (dir == kTransformTowardsInput) {
    std::swap(from, to);
  }

  std::list<NodeInput> path = FindPath(from, to, path_index);

  if (!path.empty()) {
    if (dir == kTransformTowardsInput) {
      for (auto it=path.crbegin(); it!=path.crend(); it++) {
        const NodeInput &i = (*it);
        time = i.node()->InputTimeAdjustment(i.input(), i.element(), time, false);
      }
    } else {
      // Traverse in output direction
      for (auto it=path.cbegin(); it!=path.cend(); it++) {
        const NodeInput &i = (*it);
        time = i.node()->OutputTimeAdjustment(i.input(), i.element(), time);
      }
    }
  }

  return time;
}

void Node::ParameterValueChanged(const QString& input, int element, const TimeRange& range)
{
  InputValueChangedEvent(input, element);

  emit ValueChanged(NodeInput(this, input, element), range);

  if (GetInputFlags(input) & kInputFlagIgnoreInvalidations) {
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

  SetStandardValue(input, GetDefaultValue(input), index);
}

type_t Node::ResolveSpecialType(type_t type, size_t &channel_count, QString &subtype)
{
  if (type == TYPE_VEC2) {
    channel_count = 2;
    return TYPE_DOUBLE;
  } else if (type == TYPE_VEC3) {
    channel_count = 3;
    return TYPE_DOUBLE;
  } else if (type == TYPE_VEC4) {
    channel_count = 4;
    return TYPE_DOUBLE;
  } else if (type == TYPE_COLOR) {
    channel_count = 4;
    subtype = QStringLiteral("color");
    return TYPE_DOUBLE;
  } else if (type == TYPE_BOOL) {
    subtype = QStringLiteral("bool");
    return TYPE_INTEGER;
  } else if (type == TYPE_COMBO) {
    subtype = QStringLiteral("combo");
    return TYPE_INTEGER;
  } else if (type == TYPE_FONT) {
    subtype = QStringLiteral("font");
    return TYPE_STRING;
  } else if (type == TYPE_FILE) {
    subtype = QStringLiteral("file");
    return TYPE_STRING;
  }

  return type;
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

  if (NodeKeyframe* key = dynamic_cast<NodeKeyframe*>(event->child())) {
    NodeInput i(this, key->input(), key->element());

    if (event->type() == QEvent::ChildAdded) {
      NodeInputImmediate *imm = GetImmediate(key->input(), key->element());
      int old_sz = imm->keyframe_tracks().size();
      imm->insert_keyframe(key);
      for (int j = old_sz; j < imm->keyframe_tracks().size(); j++) {
        emit KeyframeTrackAdded(key->input(), key->element(), j);
      }

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
  } else if (NodeGizmo *gizmo = dynamic_cast<NodeGizmo*>(event->child())) {
    if (event->type() == QEvent::ChildAdded) {
      gizmos_.append(gizmo);
    } else if (event->type() == QEvent::ChildRemoved) {
      gizmos_.removeOne(gizmo);
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

void Node::SetValueAtTime(const NodeInput &input, const rational &time, const value_t::component_t &value, size_t track, MultiUndoCommand *command, bool insert_on_all_tracks_if_no_key)
{
  if (input.IsKeyframing()) {
    rational node_time = time;

    NodeKeyframe* existing_key = input.GetKeyframeAtTimeOnTrack(node_time, track);

    if (existing_key) {
      command->add_child(new NodeParamSetKeyframeValueCommand(existing_key, value));
    } else {
      // No existing key, create a new one
      size_t nb_tracks = input.node()->GetNumberOfKeyframeTracks(input.input());
      for (size_t i=0; i<nb_tracks; i++) {
        value_t::component_t track_value;

        if (i == track) {
          track_value = value;
        } else if (!insert_on_all_tracks_if_no_key) {
          continue;
        } else {
          track_value = input.node()->GetSplitValueAtTimeOnTrack(input.input(), node_time, i, input.element());
        }

        NodeKeyframe* new_key = new NodeKeyframe(node_time,
                                                 track_value,
                                                 input.node()->GetBestKeyframeTypeForTimeOnTrack(NodeKeyframeTrackReference(input, i), node_time),
                                                 i,
                                                 input.element(),
                                                 input.input());

        command->add_child(new NodeParamInsertKeyframeCommand(input.node(), new_key));
      }
    }
  } else {
    command->add_child(new NodeParamSetStandardValueCommand(NodeKeyframeTrackReference(input, track), value));
  }
}

bool FindPathInternal(std::list<NodeInput> &vec, Node *from, Node *to, int &path_index)
{
  for (auto it = to->input_connections().cbegin(); it != to->input_connections().cend(); it++) {
    vec.push_front(it->second);

    if (it->first.node() == from) {
      // Found a path! Determine if it's the index we want
      if (path_index == 0) {
        // It is!
        return true;
      } else {
        // It isn't, keep looking...
        path_index--;
      }
    }

    if (FindPathInternal(vec, from, it->first.node(), path_index)) {
      return true;
    }

    vec.pop_front();
  }

  return false;
}

std::list<NodeInput> Node::FindPath(Node *from, Node *to, int path_index)
{
  std::list<NodeInput> v;

  FindPathInternal(v, from, to, path_index);

  return v;
}

bool Node::ValueHint::load(QXmlStreamReader *reader)
{
  uint version = 0;
  XMLAttributeLoop(reader, attr) {
    version = attr.value().toUInt();
  }

  Q_UNUSED(version)

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("tag")) {
      QString output = reader->readElementText();
      qDebug() << "FIXME: need to propagate output" << output;
    } else if (reader->name() == QStringLiteral("swizzle")) {
      this->swizzle_.load(reader);
    } else {
      reader->skipCurrentElement();
    }
  }

  return true;
}

void Node::ValueHint::save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("version"), QString::number(2));

  if (!this->swizzle_.empty()) {
    writer->writeStartElement(QStringLiteral("swizzle"));

    this->swizzle_.save(writer);

    writer->writeEndElement(); // swizzle
  }
}

bool Node::Position::load(QXmlStreamReader *reader)
{
  bool got_pos_x = false;
  bool got_pos_y = false;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("x")) {
      this->position.setX(reader->readElementText().toDouble());
      got_pos_x = true;
    } else if (reader->name() == QStringLiteral("y")) {
      this->position.setY(reader->readElementText().toDouble());
      got_pos_y = true;
    } else if (reader->name() == QStringLiteral("expanded")) {
      this->expanded = reader->readElementText().toInt();
    } else {
      reader->skipCurrentElement();
    }
  }

  return got_pos_x && got_pos_y;
}

void Node::Position::save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("x"), QString::number(this->position.x()));
  writer->writeTextElement(QStringLiteral("y"), QString::number(this->position.y()));
  writer->writeTextElement(QStringLiteral("expanded"), QString::number(this->expanded));
}

}
