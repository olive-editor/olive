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

#ifndef NODEKEYFRAME_H
#define NODEKEYFRAME_H

#include <memory>
#include <QPointF>
#include <QVariant>

#include "common/rational.h"
#include "node/param.h"

namespace olive {

class Node;

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
  NodeKeyframe(const rational& time, const QVariant& value, Type type, int track, int element, const QString& input, QObject* parent = nullptr);

  virtual ~NodeKeyframe() override;

  NodeKeyframe* copy(int element, QObject* parent = nullptr) const;
  NodeKeyframe* copy(QObject* parent = nullptr) const;

  Node* parent() const;
  const QString& input() const
  {
    return input_;
  }

  NodeKeyframeTrackReference key_track_ref() const
  {
    return NodeKeyframeTrackReference(NodeInput(parent(), input(), element()), track());
  }

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
  * @brief Returns a known good bezier that should be used in actual animation
  *
  * While users can move the bezier controls wherever they want, we have to limit their usage
  * internally to prevent a situation where the animation overlaps (i.e. there can only be one Y
  * value for any given X in the bezier line). This returns a value that is known good.
  */
  QPointF valid_bezier_control_in() const;
  QPointF valid_bezier_control_out() const;

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
  int track() const
  {
    return track_;
  }

  int element() const
  {
    return element_;
  }

  /**
   * @brief Convenience function for getting the opposite handle type (e.g. kInHandle <-> kOutHandle)
   */
  static BezierType get_opposing_bezier_type(BezierType type);

  NodeKeyframe* previous() const
  {
    return previous_;
  }

  void set_previous(NodeKeyframe* keyframe)
  {
    previous_ = keyframe;
  }

  NodeKeyframe* next() const
  {
    return next_;
  }

  void set_next(NodeKeyframe* keyframe)
  {
    next_ = keyframe;
  }

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

  QString input_;

  int track_;

  int element_;

  NodeKeyframe* previous_;

  NodeKeyframe* next_;

};

using NodeKeyframeTrack = QVector<NodeKeyframe*>;

}

Q_DECLARE_METATYPE(olive::NodeKeyframe::Type)

#endif // NODEKEYFRAME_H
