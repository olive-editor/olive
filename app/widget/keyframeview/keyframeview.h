#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include <QGraphicsView>

#include "common/rational.h"
#include "keyframeviewitem.h"
#include "widget/timelinewidget/view/timelineviewbase.h"

class KeyframeView : public TimelineViewBase
{
  Q_OBJECT
public:
  KeyframeView(QWidget* parent = nullptr);

  void Clear();

public slots:
  void AddKeyframe(NodeKeyframePtr key, int y);

  void RemoveKeyframe(NodeKeyframePtr key);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

  virtual void ScaleChangedEvent(double scale) override;

private:
  QMap<NodeKeyframe*, KeyframeViewItem*> item_map_;

private slots:
  void ShowContextMenu();
};

#endif // KEYFRAMEVIEW_H
