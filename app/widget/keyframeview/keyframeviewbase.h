#ifndef KEYFRAMEVIEWBASE_H
#define KEYFRAMEVIEWBASE_H

#include "core.h"
#include "keyframeviewitem.h"
#include "node/keyframe.h"
#include "widget/curvewidget/beziercontrolpointitem.h"
#include "widget/timelinewidget/view/timelineviewbase.h"

class KeyframeViewBase : public TimelineViewBase
{
  Q_OBJECT
public:
  KeyframeViewBase(QWidget* parent = nullptr);

  virtual void Clear();

public slots:
  void RemoveKeyframe(NodeKeyframePtr key);

protected:
  virtual KeyframeViewItem* AddKeyframeInternal(NodeKeyframePtr key);

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(double scale) override;

  const QMap<NodeKeyframe*, KeyframeViewItem*>& item_map() const;

  virtual void KeyframeAboutToBeRemoved(NodeKeyframe* key);

private:
  rational CalculateNewTimeFromScreen(const rational& old_time, double cursor_diff);

  static QPointF GenerateBezierControlPosition(const NodeKeyframe::BezierType mode,
                                               const QPointF& start_point,
                                               const QPointF& scaled_cursor_diff);

  void ProcessBezierDrag(const QPointF& mouse_diff_scaled, bool include_opposing, bool undoable);

  QPointF GetScaledCursorPos(const QPoint& cursor_pos);

  struct KeyframeItemAndTime {
    KeyframeViewItem* key;
    qreal item_x;
    rational time;
  };

  QMap<NodeKeyframe*, KeyframeViewItem*> item_map_;

  Tool::Item active_tool_;

  QPoint drag_start_;

  BezierControlPointItem* dragging_bezier_point_;
  QPointF dragging_bezier_point_start_;
  QPointF dragging_bezier_point_opposing_start_;

  QVector<KeyframeItemAndTime> selected_keys_;

private slots:
  void ShowContextMenu();

  void ApplicationToolChanged(Tool::Item tool);

};

#endif // KEYFRAMEVIEWBASE_H
