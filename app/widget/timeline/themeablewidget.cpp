#include "widget/timeline/themeablewidget.h"

#include <QPaintEvent>
#include <QStyleOption>
#include <QStyle>
#include <QPainter>

namespace olive {

ThemeableWidget::ThemeableWidget(QWidget* parent, Qt::WindowFlags f) :
  QWidget(parent, f) {}

void ThemeableWidget::paintEvent(QPaintEvent* event) {
  QStyleOption opt;
  opt.init(this);
  QPainter p(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

  QWidget::paintEvent(event);
}

}