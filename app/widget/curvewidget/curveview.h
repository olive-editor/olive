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

#ifndef CURVEVIEW_H
#define CURVEVIEW_H

#include "beziercontrolpointitem.h"
#include "node/keyframe.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/keyframeview/keyframeviewitem.h"

namespace olive {

class CurveView : public KeyframeViewBase
{
  Q_OBJECT
public:
  CurveView(QWidget* parent = nullptr);

  virtual ~CurveView() override;

  virtual void Clear() override;

  void ConnectInput(NodeInput* input, int element, int track);

  void DisconnectInput(NodeInput* input, int element, int track);

  void SelectKeyframesOfInput(NodeInput* input, int element, int track);

  void ZoomToFitInput(NodeInput* input, int element, int track);

  void SetKeyframeTrackColor(const NodeInput::KeyframeTrackReference& ref, const QColor& color);

public slots:
  virtual KeyframeViewItem* AddKeyframe(NodeKeyframe* key) override;

  void ZoomToFit();

  void ZoomToFitSelected();

  void ResetZoom();

protected:
  virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

  virtual void KeyframeAboutToBeRemoved(NodeKeyframe *key) override;

  virtual void ScaleChangedEvent(const double &scale) override;

  virtual void VerticalScaleChangedEvent(double scale) override;

  virtual void wheelEvent(QWheelEvent* event) override;

  virtual void ContextMenuEvent(Menu &m) override;

private:
  void ZoomToFitInternal(const QList<NodeKeyframe *> &keys);

  qreal GetItemYFromKeyframeValue(NodeKeyframe* key);
  qreal GetItemYFromKeyframeValue(double value);

  void SetItemYFromKeyframeValue(NodeKeyframe* key, KeyframeViewItem* item);

  QPointF ScalePoint(const QPointF& point);

  void AdjustLines();

  void CreateBezierControlPoints(KeyframeViewItem *item);

  QHash<NodeInput::KeyframeTrackReference, QColor> keyframe_colors_;

  int text_padding_;

  int minimum_grid_space_;

  QVector<QGraphicsLineItem*> lines_;

  QVector<BezierControlPointItem*> bezier_control_points_;

  QVector<NodeInput::KeyframeTrackReference> connected_inputs_;

private slots:
  void KeyframeValueChanged();

  void KeyframeTypeChanged();

  void SelectionChanged();

  void BezierControlPointDestroyed();

};

}

#endif // CURVEVIEW_H
