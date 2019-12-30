#ifndef CURVEVIEW_H
#define CURVEVIEW_H

#include "beziercontrolpointitem.h"
#include "node/keyframe.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/keyframeview/keyframeviewitem.h"

class CurveView : public KeyframeViewBase
{
public:
  CurveView(QWidget* parent = nullptr);

  virtual ~CurveView() override;

  virtual void Clear() override;

public slots:
  void AddKeyframe(NodeKeyframePtr key);

protected:
  virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

  virtual void KeyframeAboutToBeRemoved(NodeKeyframe *key) override;

  virtual void ScaleChangedEvent(double scale) override;

private:
  QList<NodeKeyframe*> GetKeyframesSortedByTime();

  qreal GetItemYFromKeyframeValue(NodeKeyframe* key);

  void SetItemYFromKeyframeValue(NodeKeyframe* key, KeyframeViewItem* item);

  QPointF ScalePoint(const QPointF& point);

  void AdjustLines();

  int text_padding_;

  int minimum_grid_space_;

  QList<QGraphicsLineItem*> lines_;

  QList<BezierControlPointItem*> bezier_control_points_;

private slots:
  void KeyframeValueChanged();

  void SelectionChanged();

  void BezierControlPointDestroyed();

};

#endif // CURVEVIEW_H
