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

#ifndef NODEINPUT_H
#define NODEINPUT_H

#include "common/timerange.h"
#include "keyframe.h"
#include "node/connectable.h"
#include "node/inputimmediate.h"
#include "node/value.h"

namespace olive {

class Node;

/**
 * @brief A node parameter designed to take either user input or data from another node
 */
class NodeInput : public NodeConnectable
{
  Q_OBJECT
public:
  /**
   * @brief NodeInput Constructor
   *
   * @param id
   *
   * Unique ID associated with this parameter for this Node. This ID only has to be unique within this Node. Used for
   * saving/loading data from this Node so that parameter order can be changed without issues loading data saved by an
   * older version. This of course assumes that parameters don't change their ID.
   */
  NodeInput(Node* parent, const QString &id, NodeValue::Type type, const QVector<QVariant>& default_value);
  NodeInput(Node* parent, const QString &id, NodeValue::Type type, const QVariant& default_value);
  NodeInput(Node* parent, const QString &id, NodeValue::Type type);

  virtual ~NodeInput() override;

  const QString& id() const
  {
    return id_;
  }

  QString name() const;

  void set_name(const QString& name)
  {
    name_ = name;

    emit NameChanged(name_);
  }

  bool IsArray() const
  {
    return is_array_;
  }

  void SetIsArray(bool e)
  {
    is_array_ = e;
  }

  void DisconnectAll();

  void Load(QXmlStreamReader* reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled);

  void Save(QXmlStreamWriter* writer) const;

  /**
   * @brief Deliberately shadow QObject parent, since we only expect NodeInput to have Node as a parent
   */
  Node* parent() const;

  /**
   * @brief The data type this parameter outputs
   *
   * This can be used in conjunction with NodeInput::can_accept_type() to determine whether this parameter can be
   * connected to it.
   */
  NodeValue::Type GetDataType() const
  {
    return data_type_;
  }

  void SetDataType(NodeValue::Type type)
  {
    data_type_ = type;

    emit DataTypeChanged(type);
  }

  const QHash<int, Node*>& edges() const
  {
    return input_connections();
  }

  bool IsConnected(int element = -1) const
  {
    return input_connections().contains(element);
  }

  /**
   * @brief Returns TRUE if the value is expected to always be the same (i.e. no keyframes and
   * not connected to anything)
   */
  bool IsStatic(int element = -1) const
  {
    return !IsConnected(element) && !GetImmediate(element)->is_keyframing();
  }

  Node* GetConnectedNode(int element = -1) const
  {
    return input_connections().value(element);
  }

  bool IsConnectable() const
  {
    return connectable_;
  }

  void SetConnectable(bool e)
  {
    connectable_ = e;
  }

  const QVector<NodeKeyframeTrack> &GetKeyframeTracks(int element = -1) const
  {
    return GetImmediate(element)->keyframe_tracks();
  }

  NodeKeyframe* GetKeyframeAtTimeOnTrack(const rational& time, int track, int element) const
  {
    return GetImmediate(element)->get_keyframe_at_time_on_track(time, track);
  }

  NodeKeyframe::Type GetBestKeyframeTypeForTime(const rational& time, int track, int element) const
  {
    return GetImmediate(element)->get_best_keyframe_type_for_time(time, track);
  }

  /**
   * @brief Return whether this input can be keyframed or not
   */
  bool IsKeyframable() const;

  bool IsKeyframing(int element = -1) const
  {
    return GetImmediate(element)->is_keyframing();
  }

  /**
   * @brief Get non-keyframed value
   */
  QVariant GetStandardValue(int element = -1) const
  {
    return NodeValue::combine_track_values_into_normal_value(data_type_, GetSplitStandardValue(element));
  }

  /**
   * @brief Get non-keyframed value split into components (the way it's stored)
   */
  const QVector<QVariant>& GetSplitStandardValue(int element = -1) const
  {
    return GetImmediate(element)->get_split_standard_value();
  }

  QVariant GetStandardValueOnTrack(int track, int element = -1) const
  {
    return GetImmediate(element)->get_split_standard_value().at(track);
  }

  /**
   * @brief Set non-keyframed value
   *
   * This is the value used if keyframing is not enabled. If keyframing is enabled, it is
   * overwritten.
   */
  void SetStandardValue(const QVariant& value, int element = -1)
  {
    SetSplitStandardValue(NodeValue::split_normal_value_into_track_values(data_type_, value), element);
  }

  void SetSplitStandardValue(const QVector<QVariant>& value, int element = -1)
  {
    GetImmediate(element)->set_split_standard_value(value);

    for (int i=0; i<value.size(); i++) {
      if (IsUsingStandardValue(i, element)) {
        // If this standard value is being used, we need to send a value changed signal
        emit ValueChanged(TimeRange(RATIONAL_MIN, RATIONAL_MAX), element);

        break;
      }
    }
  }

  void SetStandardValueOnTrack(const QVariant& value, int track = 0, int element = -1)
  {
    GetImmediate(element)->set_standard_value_on_track(value, track);

    if (IsUsingStandardValue(track, element)) {
      // If this standard value is being used, we need to send a value changed signal
      emit ValueChanged(TimeRange(RATIONAL_MIN, RATIONAL_MAX), element);
    }
  }

  /**
   * @brief Set whether this input can be keyframed or not
   */
  void SetKeyframable(bool k);

  /**
   * @brief Copy all values including keyframe information and connections from another NodeInput
   */
  static void CopyValues(NodeInput* source, NodeInput* dest, bool include_connections = true, bool traverse_arrays = true);

  static void CopyValuesOfElement(NodeInput* source, NodeInput* dst, int element);

  QStringList get_combobox_strings() const;

  void set_combobox_strings(const QStringList& strings);

  void GetDependencies(QVector<Node *> &list, bool traverse, bool exclusive_only) const;

  QVariant GetDefaultValue() const
  {
    return NodeValue::combine_track_values_into_normal_value(data_type_, default_value_);
  }

  const QVector<QVariant>& GetSplitDefaultValue() const
  {
    return default_value_;
  }

  QVariant GetDefaultValueForTrack(int track) const;

  QVector<Node*> GetDependencies(bool traverse = true, bool exclusive_only = false) const;

  QVector<Node*> GetExclusiveDependencies() const;

  QVector<Node*> GetImmediateDependencies() const;

  NodeKeyframe* GetEarliestKeyframe(int element = -1)
  {
    return GetImmediate(element)->get_earliest_keyframe();
  }

  NodeKeyframe* GetLatestKeyframe(int element = -1)
  {
    return GetImmediate(element)->get_latest_keyframe();
  }

  bool HasKeyframeAtTime(const rational& time, int element = -1)
  {
    return GetImmediate(element)->has_keyframe_at_time(time);
  }

  QVector<NodeKeyframe*> GetKeyframesAtTime(const rational& time, int element = -1)
  {
    return GetImmediate(element)->get_keyframe_at_time(time);
  }

  void SetIsKeyframing(bool keyframing, int element)
  {
    Q_ASSERT(IsKeyframable());

    GetImmediate(element)->set_is_keyframing(keyframing);

    emit KeyframeEnableChanged(keyframing, element);
  }

  void ArrayAppend()
  {
    ArrayResize(ArraySize() + 1);
  }

  void ArrayInsert(int index);

  void ArrayRemove(int index);

  void ArrayPrepend();

  void ArrayResize(int size);

  int ArraySize() const
  {
    return array_size_;
  }

  void ArrayRemoveLast();

  int GetNumberOfKeyframeTracks() const
  {
    return NodeValue::get_number_of_keyframe_tracks(data_type_);
  }

  /**
   * @brief Calculate what the stored value should be at a certain time
   *
   * If this is a multi-track data type (e.g. kVec2), this will automatically combine the result into a QVector2D.
   */
  QVariant GetValueAtTime(const rational& time, int element = -1) const;

  QVector<QVariant> GetSplitValuesAtTime(const rational& time, int element = -1) const;

  /**
   * @brief Calculate the stored value for a specific track
   *
   * For most data types, there is only one track (e.g. `track == 0`), but multi-track data types like kVec2 will
   * produce the X value on track 0 and the Y value on track 1.
   */
  QVariant GetValueAtTimeForTrack(const rational& time, int track, int element = -1) const;

  NodeKeyframe* GetClosestKeyframeBeforeTime(const rational& time, int element = -1) const
  {
    return GetImmediate(element)->get_closest_keyframe_before_time(time);
  }

  NodeKeyframe* GetClosestKeyframeAfterTime(const rational& time, int element = -1) const
  {
    return GetImmediate(element)->get_closest_keyframe_after_time(time);
  }

signals:
  void NameChanged(const QString& name);

  void ValueChanged(const olive::TimeRange& range, int element);

  void KeyframeAdded(NodeKeyframe* key);

  void KeyframeRemoved(NodeKeyframe* key);

  void PropertyChanged(const QString& s, const QVariant& v);

  void ArraySizeChanged(int size);

  void KeyframeEnableChanged(bool enabled, int element);

  void DataTypeChanged(NodeValue::Type type);

protected:
  virtual bool event(QEvent* e) override;

  virtual void childEvent(QChildEvent* e) override;

private:
  void Init(Node *parent, const QString& id, NodeValue::Type type, const QVector<QVariant> &default_val);

  void LoadImmediate(QXmlStreamReader *reader, int element, XMLNodeData& xml_node_data, const QAtomicInt* cancelled);

  void SaveImmediate(QXmlStreamWriter *writer, int element) const;

  const NodeInputImmediate* GetImmediate(int element = -1) const
  {
    return element > -1 ? subinputs_.at(element) : primary_;
  }

  NodeInputImmediate* GetImmediate(int element = -1)
  {
    return element > -1 ? subinputs_[element] : primary_;
  }

  const NodeKeyframeTrack& GetTrackFromKeyframe(NodeKeyframe* key) const
  {
    return GetImmediate(key->element())->keyframe_tracks().at(key->track());
  }

  bool IsUsingStandardValue(int track = 0, int element = -1) const
  {
    return GetImmediate(element)->is_using_standard_value(track);
  }

  QString ValueToString(const QVariant& value) const;

  QVariant StringToValue(const QString &string, QList<XMLNodeData::FootageConnection> &footage_connections, int element);

  /**
   * @brief Intelligently determine how what time range is affected by a keyframe
   */
  TimeRange get_range_affected_by_keyframe(NodeKeyframe *key) const;

  /**
   * @brief Gets a time range between the previous and next keyframes of index
   */
  TimeRange get_range_around_index(int index, int track, int element) const;

  NodeInputImmediate* primary_;

  QVector<NodeInputImmediate*> subinputs_;

  QVector<QVariant> default_value_;

  /**
   * @brief Unique identifier of this input within this node
   */
  QString id_;

  /**
   * @brief User displayable name of input
   */
  QString name_;

  /**
   * @brief Internal keyframable value
   */
  bool keyframable_;

  bool connectable_;

  bool is_array_;

  int array_size_;

  /**
   * @brief Default data type
   */
  NodeValue::Type data_type_;

private slots:
  /**
   * @brief Slot when a keyframe's time changes to keep the keyframes correctly sorted by time
   */
  void KeyframeTimeChanged();

  /**
   * @brief Slot when a keyframe's value changes to signal that the cache needs updating
   */
  void KeyframeValueChanged();

  /**
   * @brief Slot when a keyframe's type changes to signal that the cache needs updating
   */
  void KeyframeTypeChanged();

  /**
   * @brief Slot when a keyframe's bezier in value changes to signal that the cache needs updating
   */
  void KeyframeBezierInChanged();

  /**
   * @brief Slot when a keyframe's bezier out value changes to signal that the cache needs updating
   */
  void KeyframeBezierOutChanged();

};

}

#endif // NODEINPUT_H
