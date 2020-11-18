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

#ifndef BEZIERCONTROLPOINTITEM_H
#define BEZIERCONTROLPOINTITEM_H

#include <QGraphicsRectItem>

#include "node/keyframe.h"

namespace olive {

class BezierControlPointItem : public QObject, public QGraphicsRectItem
{
public:
  BezierControlPointItem(NodeKeyframePtr key, NodeKeyframe::BezierType mode, QGraphicsItem* parent = nullptr);

  void SetXScale(double scale);

  void SetYScale(double scale);

  NodeKeyframePtr key() const;

  const NodeKeyframe::BezierType& mode() const;

  QPointF GetCorrespondingKeyframeHandle() const;

  void SetCorrespondingKeyframeHandle(const QPointF& handle);

  void SetOpposingKeyframeHandle(const QPointF& handle);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  NodeKeyframePtr key_;

  NodeKeyframe::BezierType mode_;

  double x_scale_;

  double y_scale_;

private slots:
  void UpdatePos();

};

}

#endif // BEZIERCONTROLPOINTITEM_H
