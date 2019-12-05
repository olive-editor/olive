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
  NodeInput(const QString &id);

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
  const DataType& data_type();
  void set_data_type(const DataType& type);

  /**
   * @brief If this input is connected to an output, retrieve the output parameter
   *
   * @return
   *
   * The output parameter if connected or nullptr if not
   */
  NodeOutput* get_connected_output();

  /**
   * @brief If this input is connected to an output, retrieve the Node whose output is connected
   *
   * @return
   *
   * The connected Node if connected or nullptr if not
   */
  Node* get_connected_node();

  /**
   * @brief Calculate what the stored value should be at a certain time
   */
  QVariant get_value_at_time(const rational& time);

  /**
   * @brief Sets what value should be seen at a specific time
   */
  void set_value_at_time(const rational& time, const QVariant& value);

  /**
   * @brief Return whether keyframing is enabled on this input or not
   */
  bool is_keyframing();

  /**
   * @brief Set whether keyframing is enabled on this input or not
   */
  void set_is_keyframing(bool k);

  /**
   * @brief Return whether the Node is dependent on this input or not
   *
   * \see set_dependent()
   */
  bool dependent();

  /**
   * @brief Set whether the Node is dependent on this input
   */
  void set_dependent(bool d);

  const QVariant& minimum();
  bool has_minimum();
  void set_minimum(const QVariant& min);

  const QVariant& maximum();
  bool has_maximum();
  void set_maximum(const QVariant& max);

  /**
   * @brief Copy all values including keyframe information and connections from another NodeInput
   */
  static void CopyValues(NodeInput* source, NodeInput* dest, bool include_connections = true);

signals:
  void ValueChanged(const rational& start, const rational& end);

private:
  /**
   * @brief Internal list of accepted data types
   *
   * Use can_accept_type() to check if a type is in this list
   */
  DataType data_type_;

  /**
   * @brief Internal keyframe array
   *
   * All internal/user-defined data is stored in this array. Even if keyframing is not enabled, this array will contain
   * one entry which will be used, and its time value will be ignored.
   */
  QList<NodeKeyframe> keyframes_;

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

};

#endif // NODEINPUT_H
