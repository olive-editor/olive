#ifndef BEZIERCONTROLPOINTITEM_H
#define BEZIERCONTROLPOINTITEM_H

#include <QGraphicsRectItem>

#include "node/keyframe.h"

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

#endif // BEZIERCONTROLPOINTITEM_H
