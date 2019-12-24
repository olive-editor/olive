#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include <QGraphicsView>

#include "common/rational.h"
#include "keyframeviewitem.h"
#include "widget/timelinewidget/view/timelineviewbase.h"

class KeyframeView : public TimelineViewBase
{
public:
  KeyframeView(QWidget* parent = nullptr);

  void AddKeyframe(const rational& time, int y);

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;

private:

};

#endif // KEYFRAMEVIEW_H
