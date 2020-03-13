#include "waveformview.h"

#include <QPainter>
#include <QtMath>

WaveformView::WaveformView(QWidget *parent) :
  TimeBasedWidget(parent)
{
  setAutoFillBackground(true);
  setStyleSheet("background: black;");
}

void WaveformView::paintEvent(QPaintEvent *event)
{
  QWidget::paintEvent(event);

  QPainter p(this);

  p.setPen(Qt::green);

  for (int i=0;i<width();i++) {
    p.drawPoint(i, (height() / 2) + qSin(static_cast<double>(i) * 0.025) * (height() / 2));
  }
}
