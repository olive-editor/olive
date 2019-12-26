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

#include "keyframe.h"
#include "param.h"

/**
 * @brief A node parameter designed to take either user input or data from another node
 */
class NodeInput : public NodeParam
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
  NodeInput(const QString &id, const DataType& type, const QVariant& default_value = 0);

  virtual bool IsArray();

  /**
   * @brief Returns kInput
   */
  virtual Type type() override;

  virtual QString name() override;

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
   */
  QVariant get_value_at_time(const rational& time) const;

  /**
   * @brief Retrieve the keyframe object at a given time
   *
   * @return
   *
   * The keyframe object at this time or nullptr if there isn't one or if is_keyframing() is false.
   */
  NodeKeyframePtr get_keyframe_at_time(const rational& time) const;

  /**
   * @brief Gets the closest keyframe to a time
   *
   * If is_keyframing() is false or keyframes_ is empty, this will return nullptr.
   */
  NodeKeyframePtr get_closest_keyframe_to_time(const rational& time) const;

  /**
   * @brief A heuristic to determine what type a keyframe should be if it's inserted at a certain time (between keyframes)
   */
  NodeKeyframe::Type get_best_keyframe_type_for_time(const rational& time) const;

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
   * If is_keyframing() is false, this will always return false.
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
  const QVariant& get_standard_value() const;

  /**
   * @brief Set non-keyframed value
   */
  void set_standard_value(const QVariant& value);

  /**
   * @brief Return list of keyframes in this parameter
   */
  const QList<NodeKeyframePtr>& keyframes() const;

  /**
   * @brief Set whether this input can be keyframed or not
   */
  void set_is_keyframable(bool k);

  const QVariant& minimum() const;
  bool has_minimum() const;
  void set_minimum(const QVariant& min);

  const QVariant& maximum() const;
  bool has_maximum() const;
  void set_maximum(const QVariant& max);

  /**
   * @brief Copy all values including keyframe information and connections from another NodeInput
   */
  static void CopyValues(NodeInput* source, NodeInput* dest, bool include_connections = true, bool lock_connections = true);

signals:
  void ValueChanged(const rational& start, const rational& end);

  void KeyframeEnableChanged(bool);

  void KeyframeAdded(NodeKeyframePtr key);

  void KeyframeRemoved(NodeKeyframePtr key);

private:
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
  QVariant standard_value_;

  /**
   * @brief Internal keyframe array
   *
   * If keyframing is enabled, this data is used instead of standard_value.
   */
  QList<NodeKeyframePtr> keyframes_;

  /**
   * @brief Internal keyframing enabled setting
   */
  bool keyframing_;

  /**
   * @brief Internal dependent setting
   */
  bool dependent_;

  /**
   * @brief Sets whether this param has a minimum value or not
   */
  bool has_minimum_;

  /**
   * @brief Internal minimum value
   */
  QVariant minimum_;

  /**
   * @brief Sets whether this param has a maximum value or not
   */
  bool has_maximum_;

  /**
   * @brief Internal maximum value
   */
  QVariant maximum_;

private slots:
  /**
   * @brief Slot when a keyframe's time changes to keep the keyframes correctly sorted by time
   */
  void KeyframeTimeChanged();

  /**
   * @brief Slot when a keyframe's value changes to signal that the cache needs updating
   */
  void KeyframeValueChanged();

};

#endif // NODEINPUT_H
