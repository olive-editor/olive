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

#ifndef NODEKEYFRAME_H
#define NODEKEYFRAME_H

#include <QVariant>

#include "common/rational.h"

/**
 * @brief A point of data to be used at a certain time and interpolated with other data
 */
class NodeKeyframe
{
public:

  /**
   * @brief Methods of interpolation to use with this keyframe
   */
  enum Type {
    kLinear,
    kHold,
    kBezier
  };

  /**
   * @brief NodeKeyframe Constructor
   */
  NodeKeyframe();

  /**
   * @brief The time this keyframe is set at
   */
  const rational& time();
  void set_time(const rational& time);

  /**
   * @brief The value of this keyframe (i.e. the value to use at this keyframe's time)
   */
  const QVariant& value();
  void set_value(const QVariant &value);

  /**
   * @brief The method of interpolation to use with this keyframe
   */
  const Type& type();
  void set_type(const Type& type);

private:
  rational time_;

  QVariant value_;

  Type type_;
};

#endif // NODEKEYFRAME_H
