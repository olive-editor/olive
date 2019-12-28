#ifndef CURVEVIEW_H
#define CURVEVIEW_H

#include "node/keyframe.h"
#include "widget/keyframeview/keyframeviewitem.h"
#include "widget/timelinewidget/view/timelineviewbase.h"

class CurveView : public TimelineViewBase
{
public:
  CurveView(QWidget* parent = nullptr);

  void Clear();

  void SetXScale(const double& x_scale);

  void SetYScale(const double& y_scale);

public slots:
  void AddKeyframe(NodeKeyframePtr key);

  void RemoveKeyframe(NodeKeyframePtr key);

protected:
  virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
  QList<NodeKeyframe*> GetKeyframesSortedByTime();

  qreal GetItemYFromKeyframeValue(NodeKeyframe* key);

  void SetItemYFromKeyframeValue(NodeKeyframe* key, KeyframeViewItem* item);

  void AdjustLines();

  int text_padding_;

  double y_scale_;

  int minimum_grid_space_;

  QMap<NodeKeyframe*, KeyframeViewItem*> key_item_map_;

  QList<QGraphicsLineItem*> lines_;

private slots:
  void KeyframeValueChanged();

};

#endif // CURVEVIEW_H
