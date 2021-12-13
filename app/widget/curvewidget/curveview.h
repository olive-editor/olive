/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef CURVEVIEW_H
#define CURVEVIEW_H

#include "beziercontrolpointitem.h"
#include "node/keyframe.h"
#include "widget/keyframeview/keyframeview.h"

namespace olive {

class CurveView : public KeyframeViewBase
{
  Q_OBJECT
public:
  CurveView(QWidget* parent = nullptr);

  void ConnectInput(const NodeKeyframeTrackReference &ref);

  void DisconnectInput(const NodeKeyframeTrackReference &ref);

  void SelectKeyframesOfInput(const NodeKeyframeTrackReference &ref);

  void ZoomToFitInput(const NodeKeyframeTrackReference &ref);

  void SetKeyframeTrackColor(const NodeKeyframeTrackReference& ref, const QColor& color);

public slots:
  void ZoomToFit();

  void ZoomToFitSelected();

  void ResetZoom();

protected:
  virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

  virtual void ScaleChangedEvent(const double &scale) override;

  virtual void VerticalScaleChangedEvent(double scale) override;

  virtual void ContextMenuEvent(Menu &m) override;

  virtual void SceneRectUpdateEvent(QRectF &r) override;

private:
  void ZoomToFitInternal(const QVector<NodeKeyframe *> &keys);

  qreal GetItemYFromKeyframeValue(NodeKeyframe* key);
  qreal GetItemYFromKeyframeValue(double value);

  QPointF ScalePoint(const QPointF& point);

  void AdjustLines();

  void CreateBezierControlPoints(NodeKeyframe *item);

  QPointF GetKeyframePosition(NodeKeyframe *key);

  QHash<NodeKeyframeTrackReference, QColor> keyframe_colors_;
  QHash<NodeKeyframeTrackReference, KeyframeViewInputConnection*> track_connections_;

  int text_padding_;

  int minimum_grid_space_;

  QVector<QGraphicsLineItem*> lines_;

  QVector<BezierControlPointItem*> bezier_control_points_;

  QVector<NodeKeyframeTrackReference> connected_inputs_;

private slots:
  void KeyframeValueChanged();

  void KeyframeTypeChanged();

  void SelectionChanged();

  void BezierControlPointDestroyed();

};

}

#endif // CURVEVIEW_H
