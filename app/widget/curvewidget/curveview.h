#ifndef CURVEVIEW_H
#define CURVEVIEW_H

#include "node/keyframe.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/keyframeview/keyframeviewitem.h"

class CurveView : public KeyframeViewBase
{
public:
  CurveView(QWidget* parent = nullptr);

  virtual void Clear() override;

  void SetYScale(const double& y_scale);

public slots:
  void AddKeyframe(NodeKeyframePtr key);

protected:
  virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

  virtual void KeyframeAboutToBeRemoved(NodeKeyframe *key) override;

private:
  QList<NodeKeyframe*> GetKeyframesSortedByTime();

  qreal GetItemYFromKeyframeValue(NodeKeyframe* key);

  void SetItemYFromKeyframeValue(NodeKeyframe* key, KeyframeViewItem* item);

  void AdjustLines();

  int text_padding_;

  double y_scale_;

  int minimum_grid_space_;

  QList<QGraphicsLineItem*> lines_;

private slots:
  void KeyframeValueChanged();

};

#endif // CURVEVIEW_H
