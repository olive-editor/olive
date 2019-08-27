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
  NodeInput(const QString &id);

  /**
   * @brief Returns kInput
   */
  virtual Type type() override;

  /**
   * @brief Add a data type that this input accepts
   *
   * While an input will usually only accept one data type, NodeInput supports several. Use this to add a data type that
   * this input can accept.
   */
  void add_data_input(const DataType& data_type);

  /**
   * @brief Return whether an input can accept a certain type based on its list of data types
   *
   * The input checks its list of acceptable data types (added by add_data_input()) to determine whether a certain
   * data type can be connected to this input.
   */
  bool can_accept_type(const DataType& data_type);

  /**
   * @brief Get the value at a given time
   *
   * This function will automatically retrieve the correct value for this input at the given time.
   *
   * If an output is connected to this input, a request is made to that output for its value at this time. If multiple
   * outputs are connected (\see can_accept_multiple_inputs()), a QList<QVariant> (casted to a QVariant) is returned
   * instead, listing all the outputs' values currently connected.
   *
   * If no output is connected, this will return a user-defined value, either a static value if this input is not
   * keyframed, or an interpolated value between the keyframes at this time.
   */
  QVariant get_value(const rational &time);

  /**
   * @brief Set the value at a given time
   *
   * This function will only work if there are no outputs connected.
   */
  void set_value(const QVariant& value);

  /**
   * @brief Return whether keyframing is enabled on this input or not
   */
  bool keyframing();

  /**
   * @brief Set whether keyframing is enabled on this input or not
   */
  void set_keyframing(bool k);

  /**
   * @brief A list of input data types accepted by this parameter
   */
  const QList<DataType>& inputs();

signals:
  void ValueChanged(const rational& start, const rational& end);

private:
  /**
   * @brief Internal list of accepted data types
   *
   * Use can_accept_type() to check if a type is in this list
   */
  QList<DataType> inputs_;

  /**
   * @brief Internal keyframe array
   *
   * All internal/user-defined data is stored in this array. Even if keyframing is not enabled, this array will contain
   * one entry which will be used, and its time value will be ignored.
   */
  QList<NodeKeyframe> keyframes_;

  /**
   * @brief Currently cached value
   */
  QVariant value_;

  /**
   * @brief Last timecode that a value was requested with
   */
  rational time_;

  /**
   * @brief Internal keyframing enabled setting
   */
  bool keyframing_;

};

#endif // NODEINPUT_H
