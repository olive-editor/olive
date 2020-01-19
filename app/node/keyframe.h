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

#include <memory>
#include <QPointF>
#include <QVariant>

#include "common/rational.h"

class NodeKeyframe;
using NodeKeyframePtr = std::shared_ptr<NodeKeyframe>;

/**
 * @brief A point of data to be used at a certain time and interpolated with other data
 */
class NodeKeyframe : public QObject
{
  Q_OBJECT
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
   * @brief The two types of bezier handles that are available on bezier keyframes
   */
  enum BezierType {
    kInHandle,
    kOutHandle
  };

  static const Type kDefaultType;

  /**
   * @brief NodeKeyframe Constructor
   */
  NodeKeyframe(const rational& time, const QVariant& value, const Type& type, const int& track);

  static NodeKeyframePtr Create(const rational& time, const QVariant& value, const Type& type, const int &track);

  NodeKeyframePtr copy() const;

  /**
   * @brief The time this keyframe is set at
   */
  const rational& time() const;
  void set_time(const rational& time);

  /**
   * @brief The value of this keyframe (i.e. the value to use at this keyframe's time)
   */
  const QVariant& value() const;
  void set_value(const QVariant &value);

  /**
   * @brief The method of interpolation to use with this keyframe
   */
  const Type& type() const;
  void set_type(const Type& type);

  /**
   * @brief For bezier interpolation, the control point leading into this keyframe
   */
  const QPointF &bezier_control_in() const;
  void set_bezier_control_in(const QPointF& control);

  /**
   * @brief For bezier interpolation, the control point leading out of this keyframe
   */
  const QPointF& bezier_control_out() const;
  void set_bezier_control_out(const QPointF& control);

  /**
   * @brief Convenience functions for retrieving/setting bezier handle information with a BezierType
   */
  const QPointF& bezier_control(BezierType type) const;
  void set_bezier_control(BezierType type, const QPointF& control);

  /**
   * @brief The track that this keyframe belongs to
   *
   * For the majority of keyfreames, this will be 0, but for some types, such as kVec2, this will be 0 for X keyframes
   * and 1 for Y keyframes, etc.
   */
  const int& track() const;

  /**
   * @brief Convenience function for getting the opposite handle type (e.g. kInHandle <-> kOutHandle)
   */
  static BezierType get_opposing_bezier_type(BezierType type);

signals:
  /**
   * @brief Signal emitted when this keyframe's time is changed
   */
  void TimeChanged(const rational& time);

  /**
   * @brief Signal emitted when this keyframe's value is changed
   */
  void ValueChanged(const QVariant& value);

  /**
   * @brief Signal emitted when this keyframe's value is changed
   */
  void TypeChanged(const Type& type);

  /**
   * @brief Signal emitted when this keyframe's bezier in control point is changed
   */
  void BezierControlInChanged(const QPointF& d);

  /**
   * @brief Signal emitted when this keyframe's bezier out control point is changed
   */
  void BezierControlOutChanged(const QPointF& d);

private:
  rational time_;

  QVariant value_;

  Type type_;

  QPointF bezier_control_in_;

  QPointF bezier_control_out_;

  int track_;

};

Q_DECLARE_METATYPE(NodeKeyframe::Type)

#endif // NODEKEYFRAME_H
