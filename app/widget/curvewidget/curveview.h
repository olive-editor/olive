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

#include "node/keyframe.h"
#include "widget/keyframeview/keyframeview.h"
#include "widget/slider/floatslider.h"

namespace olive {

class CurveView : public KeyframeView
{
  Q_OBJECT
public:
  CurveView(QWidget* parent = nullptr);

  void ConnectInput(const NodeKeyframeTrackReference &ref);

  void DisconnectInput(const NodeKeyframeTrackReference &ref);

  void SelectKeyframesOfInput(const NodeKeyframeTrackReference &ref);

  void SetKeyframeTrackColor(const NodeKeyframeTrackReference& ref, const QColor& color);

public slots:
  void ZoomToFit();

  void ZoomToFitSelected();

  void ResetZoom();

protected:
  virtual void drawBackground(QPainter* painter, const QRectF& rect) override;
  virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

  virtual void ContextMenuEvent(Menu &m) override;

  virtual void SceneRectUpdateEvent(QRectF &r) override;

  virtual qreal GetKeyframeSceneY(KeyframeViewInputConnection *track, NodeKeyframe *key) override;

  virtual void DrawKeyframe(QPainter *painter, NodeKeyframe *key, KeyframeViewInputConnection *track, const QRectF &key_rect) override;

  virtual bool FirstChanceMousePress(QMouseEvent *event) override;
  virtual void FirstChanceMouseMove(QMouseEvent *event) override;
  virtual void FirstChanceMouseRelease(QMouseEvent *event) override;

  virtual void KeyframeDragStart(QMouseEvent *event) override;
  virtual void KeyframeDragMove(QMouseEvent *event, QString &tip) override;
  virtual void KeyframeDragRelease(QMouseEvent *event, MultiUndoCommand *command) override;

private:
  void ZoomToFitInternal(bool selected_only);

  qreal GetItemYFromKeyframeValue(NodeKeyframe* key);
  qreal GetUnscaledItemYFromKeyframeValue(NodeKeyframe* key);

  QPointF ScalePoint(const QPointF& point);

  static FloatSlider::DisplayType GetFloatDisplayTypeFromKeyframe(NodeKeyframe *key);

  static double GetOffsetFromKeyframe(NodeKeyframe *key);

  void AdjustLines();

  QPointF GetKeyframePosition(NodeKeyframe *key);

  static QPointF GenerateBezierControlPosition(const NodeKeyframe::BezierType mode,
                                               const QPointF& start_point,
                                               const QPointF& scaled_cursor_diff);

  QPointF GetScaledCursorPos(const QPointF &cursor_pos);

  QHash<NodeKeyframeTrackReference, QColor> keyframe_colors_;
  QHash<NodeKeyframeTrackReference, KeyframeViewInputConnection*> track_connections_;

  int text_padding_;

  int minimum_grid_space_;

  QVector<NodeKeyframeTrackReference> connected_inputs_;

  struct BezierPoint
  {
    QRectF rect;
    NodeKeyframe *keyframe;
    NodeKeyframe::BezierType type;
  };

  QVector<BezierPoint> bezier_pts_;
  const BezierPoint *dragging_bezier_pt_;

  QPointF dragging_bezier_point_start_;
  QPointF dragging_bezier_point_opposing_start_;
  QPointF drag_start_;

  QVector<QVariant> drag_keyframe_values_;

};

}

#endif // CURVEVIEW_H
