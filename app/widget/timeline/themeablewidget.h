#ifndef _OLIVE_THEMEABLE_WIDGET_H_
#define _OLIVE_THEMEABLE_WIDGET_H_

#include <Qt>
#include <QWidget>

namespace olive {

class ThemeableWidget : public QWidget {
  Q_OBJECT

protected:
  void ThemeableWidget::paintEvent(QPaintEvent* event);

public:
  ThemeableWidget(QWidget* parent = (QWidget *)nullptr, Qt::WindowFlags f = Qt::WindowFlags());

};

}

#endif