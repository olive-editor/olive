#ifndef CURVEVIEW_H
#define CURVEVIEW_H

#include "widget/timelinewidget/view/timelineviewbase.h"

class CurveView : public TimelineViewBase
{
public:
  CurveView(QWidget* parent = nullptr);

protected:
  virtual void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
  int text_padding_;

};

#endif // CURVEVIEW_H
