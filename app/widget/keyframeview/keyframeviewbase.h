#ifndef KEYFRAMEVIEWBASE_H
#define KEYFRAMEVIEWBASE_H

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

  const double& GetYScale() const;
  void SetYScale(const double& y_scale);

public slots:
  void RemoveKeyframe(NodeKeyframePtr key);

protected:
  virtual KeyframeViewItem* AddKeyframeInternal(NodeKeyframePtr key);

  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(const double& scale) override;

  virtual void VerticalScaleChangedEvent(double scale);

  const QMap<NodeKeyframe*, KeyframeViewItem*>& item_map() const;

  virtual void KeyframeAboutToBeRemoved(NodeKeyframe* key);

  void SetYAxisEnabled(bool e);

  double y_scale_;

private:
  rational CalculateNewTimeFromScreen(const rational& old_time, double cursor_diff);

  static QPointF GenerateBezierControlPosition(const NodeKeyframe::BezierType mode,
                                               const QPointF& start_point,
                                               const QPointF& scaled_cursor_diff);

  void ProcessBezierDrag(QPointF mouse_diff_scaled, bool include_opposing, bool undoable);

  QPointF GetScaledCursorPos(const QPoint& cursor_pos);

  struct KeyframeItemAndTime {
    KeyframeViewItem* key;
    qreal item_x;
    rational time;
    double value;
  };

  QMap<NodeKeyframe*, KeyframeViewItem*> item_map_;

  Tool::Item active_tool_;

  QPoint drag_start_;

  BezierControlPointItem* dragging_bezier_point_;
  QPointF dragging_bezier_point_start_;
  QPointF dragging_bezier_point_opposing_start_;

  QVector<KeyframeItemAndTime> selected_keys_;

  bool y_axis_enabled_;

private slots:
  void ShowContextMenu();

  void ShowKeyframePropertiesDialog();

};

#endif // KEYFRAMEVIEWBASE_H
