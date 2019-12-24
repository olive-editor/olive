#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include <QGraphicsView>

#include "common/rational.h"
#include "keyframeviewitem.h"
#include "widget/timeruler/timeruler.h"

class KeyframeView : public QWidget
{
public:
  KeyframeView(QWidget* parent = nullptr);

  void AddKeyframe(const rational& time, int y);

  void SetTimebase(const rational& timebase);

private:
  TimeRuler* ruler_;
  QGraphicsView* view_;

  QGraphicsScene scene_;
};

#endif // KEYFRAMEVIEW_H
