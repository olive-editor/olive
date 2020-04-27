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

#ifndef NODEINPUT_H
#define NODEINPUT_H

#include "common/timerange.h"
#include "keyframe.h"
#include "param.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief A node parameter designed to take either user input or data from another node
 */
class NodeInput : public NodeParam
{
  Q_OBJECT
public:
  using KeyframeTrack = QList<NodeKeyframePtr>;

  /**
   * @brief NodeInput Constructor
   *
   * @param id
   *
   * Unique ID associated with this parameter for this Node. This ID only has to be unique within this Node. Used for
   * saving/loading data from this Node so that parameter order can be changed without issues loading data saved by an
   * older version. This of course assumes that parameters don't change their ID.
   */
  NodeInput(const QString &id, const DataType& type, const QVector<QVariant>& default_value);
  NodeInput(const QString &id, const DataType& type, const QVariant& default_value);
  NodeInput(const QString &id, const DataType& type);

  virtual bool IsArray() const;

  /**
   * @brief Returns kInput
   */
  virtual Type type() override;

  virtual QString name() override;

  virtual void Load(QXmlStreamReader* reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled) override;

  virtual void Save(QXmlStreamWriter* writer) const override;

  /**
   * @brief The data type this parameter outputs
   *
   * This can be used in conjunction with NodeInput::can_accept_type() to determine whether this parameter can be
   * connected to it.
   */
  const DataType& data_type() const;

  /**
   * @brief If this input is connected to an output, retrieve the output parameter
   *
   * @return
   *
   * The output parameter if connected or nullptr if not
   */
  NodeOutput* get_connected_output() const;

  /**
   * @brief If this input is connected to an output, retrieve the Node whose output is connected
   *
   * @return
   *
   * The connected Node if connected or nullptr if not
   */
  Node* get_connected_node() const;

  /**
   * @brief Calculate what the stored value should be at a certain time
   *
   * If this is a multi-track data type (e.g. kVec2), this will automatically combine the result into a QVector2D.
   */
  QVariant get_value_at_time(const rational& time) const;

  QVector<QVariant> get_split_values_at_time(const rational& time) const;

  /**
   * @brief Calculate the stored value for a specific track
   *
   * For most data types, there is only one track (e.g. `track == 0`), but multi-track data types like kVec2 will
   * produce the X value on track 0 and the Y value on track 1.
   */
  QVariant get_value_at_time_for_track(const rational& time, int track) const;

  /**
   * @brief Retrieve a list of keyframe objects for all tracks at a given time
   *
   * List may be empty if this input is not keyframing or has no keyframes at this time.
   */
  QList<NodeKeyframePtr> get_keyframe_at_time(const rational& time) const;

  /**
   * @brief Retrieve the keyframe object at a given time for a given track
   *
   * @return
   *
   * The keyframe object at this time or nullptr if there isn't one or if is_keyframing() is false.
   */
  NodeKeyframePtr get_keyframe_at_time_on_track(const rational& time, int track) const;

  /**
   * @brief Gets the closest keyframe to a time
   *
   * If is_keyframing() is false or keyframes_ is empty, this will return nullptr.
   */
  NodeKeyframePtr get_closest_keyframe_to_time_on_track(const rational& time, int track) const;

  /**
   * @brief Get closest keyframe that's before the time on any track
   *
   * If no keyframe is before this time, returns nullptr.
   */
  NodeKeyframePtr get_closest_keyframe_before_time(const rational& time) const;

  /**
   * @brief Get closest keyframe that's before the time on any track
   *
   * If no keyframe is before this time, returns nullptr.
   */
  NodeKeyframePtr get_closest_keyframe_after_time(const rational& time) const;

  /**
   * @brief A heuristic to determine what type a keyframe should be if it's inserted at a certain time (between keyframes)
   */
  NodeKeyframe::Type get_best_keyframe_type_for_time(const rational& time, int track) const;

  /**
   * @brief Retrieve the number of
   */
  int get_number_of_keyframe_tracks() const;

  /**
   * @brief Gets the earliest keyframe on any track
   */
  NodeKeyframePtr get_earliest_keyframe() const;

  /**
   * @brief Gets the latest keyframe on any track
   */
  NodeKeyframePtr get_latest_keyframe() const;

  /**
   * @brief Inserts a keyframe at the given time and returns a reference to it
   */
  void insert_keyframe(NodeKeyframePtr key);

  /**
   * @brief Removes the keyframe
   */
  void remove_keyframe(NodeKeyframePtr key);

  /**
   * @brief Return whether a keyframe exists at this time
   *
   * If is_keyframing() is false, this will always return false. This checks all tracks and will return true if *any*
   * track has a keyframe.
   */
  bool has_keyframe_at_time(const rational &time) const;

  /**
   * @brief Return whether keyframing is enabled on this input or not
   */
  bool is_keyframing() const;

  /**
   * @brief Set whether keyframing is enabled on this input or not
   */
  void set_is_keyframing(bool k);

  /**
   * @brief Return whether this input can be keyframed or not
   */
  bool is_keyframable() const;

  /**
   * @brief Get non-keyframed value
   */
  QVariant get_standard_value() const;

  /**
   * @brief Get non-keyframed value split into components (the way it's stored)
   */
  const QVector<QVariant>& get_split_standard_value() const;

  /**
   * @brief Set non-keyframed value
   */
  void set_standard_value(const QVariant& value, int track = 0);

  /**
   * @brief Return list of keyframes in this parameter
   */
  const QVector<KeyframeTrack> &keyframe_tracks() const;

  /**
   * @brief Set whether this input can be keyframed or not
   */
  void set_is_keyframable(bool k);

  /**
   * @brief Copy all values including keyframe information and connections from another NodeInput
   */
  static void CopyValues(NodeInput* source, NodeInput* dest, bool include_connections = true);

  /**
   * @brief Set an arbitrary property on this input to influence a UI representation's behavior
   *
   * NodeInputs also utilize QObject's property key/value system for arbitrary properties that can influence the UI
   * representation's behavior.
   *
   * Currently supported properties:
   *
   * - `min` - For any numeral type represented with a slider, prevents values going BELOW this number
   * - `max` - For any numeral type represented with a slider, prevents values going ABOVE this number
   * - `view` - For any numeral type represented with a slider, shows number either as `db`, `percent`, or `normal`
   * - `disablex` - For kVec2, kVec3, kVec4 and kColor types, disables the first/X/R UI widget
   * - `disabley` - For kVec2, kVec3, kVec4 and kColor types, disables the second/Y/G UI widget
   * - `disablez` - For kVec3, kVec4 and kColor types, disables the third/Z/B UI widget
   * - `disablew` - For kVec4 and kColor types, disables the fourth/W/A UI widget
   */
  void set_property(const QString& key, const QVariant& value);

  /**
   * @brief Retrieve a property (or an empty QVariant if it hasn't been set)
   */
  QVariant get_property(const QString& key) const;

  /**
   * @brief Return whether a certain property has been set or not
   */
  bool has_property(const QString& key) const;

  /**
   * @brief Get properties hashmap (useful for iterating)
   */
  const QHash<QString, QVariant>& properties() const;

  QVector<QVariant> split_normal_value_into_track_values(const QVariant &value) const;

  QVariant combine_track_values_into_normal_value(const QVector<QVariant>& split) const;

  QStringList get_combobox_strings() const;

  void set_combobox_strings(const QStringList& strings);

  static QString ValueToString(const DataType& data_type, const QVariant& value);

  static QVariant StringToValue(const DataType &data_type, const QString &string);

  void GetDependencies(QList<Node*>& list, bool traverse, bool exclusive_only) const;

  QList<Node*> GetDependencies(bool traverse = true, bool exclusive_only = false) const;

  QList<Node*> GetExclusiveDependencies() const;

  QList<Node*> GetImmediateDependencies() const;

signals:
  void ValueChanged(const OLIVE_NAMESPACE::TimeRange& range);

  void KeyframeEnableChanged(bool);

  void KeyframeAdded(NodeKeyframePtr key);

  void KeyframeRemoved(NodeKeyframePtr key);

  void PropertyChanged(const QString& s, const QVariant& v);

protected:
  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled);

  virtual void SaveInternal(QXmlStreamWriter* writer) const;

private:
  void Init(NodeParam::DataType type);

  void SetDefaultValue(const QVector<QVariant> &default_value);

  QString ValueToString(const QVariant& value) const;

  QVariant StringToValue(const QString &string, QList<XMLNodeData::FootageConnection> &footage_connections);

  void SaveConnections(QXmlStreamWriter* writer) const;

  /**
   * @brief Returns whether a data type can be interpolated or not
   */
  static bool type_can_be_interpolated(DataType type);

  /**
   * @brief We use Qt signals/slots for keyframe communication but store them as shared ptrs. This function converts
   * a raw ptr to a list index
   */
  int FindIndexOfKeyframeFromRawPtr(NodeKeyframe* raw_ptr) const;

  /**
   * @brief Internal insert function, automatically does an insertion sort based on the keyframe's time
   */
  void insert_keyframe_internal(NodeKeyframePtr key);

  /**
   * @brief Return whether the standard value should be used over keyframe data
   */
  bool is_using_standard_value(int track) const;

  /**
   * @brief Intelligently determine how what time range is affected by a keyframe
   */
  TimeRange get_range_affected_by_keyframe(NodeKeyframe *key) const;

  /**
   * @brief Gets a time range between the previous and next keyframes of index
   */
  TimeRange get_range_around_index(int index, int track) const;

  /**
   * @brief Convenience function - equivalent to calling `emit ValueChanged(range.in(), range.out())`
   */
  void emit_time_range(const TimeRange& range);

  /**
   * @brief Convenience function - equivalent to calling `emit_time_range(get_range_affected_by_keyframe(key))`
   */
  void emit_range_affected_by_keyframe(NodeKeyframe* key);

  /**
   * @brief Internal list of accepted data types
   *
   * Use can_accept_type() to check if a type is in this list
   */
  DataType data_type_;

  /**
   * @brief Internal keyframable value
   */
  bool keyframable_;

  /**
   * @brief Non-keyframed value
   */
  QVector<QVariant> standard_value_;

  /**
   * @brief Internal keyframe array
   *
   * If keyframing is enabled, this data is used instead of standard_value.
   */
  QVector< QList<NodeKeyframePtr> > keyframe_tracks_;

  /**
   * @brief Internal keyframing enabled setting
   */
  bool keyframing_;

  /**
   * @brief Internal properties variable
   */
  QHash<QString, QVariant> properties_;

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

OLIVE_NAMESPACE_EXIT

#endif // NODEINPUT_H
