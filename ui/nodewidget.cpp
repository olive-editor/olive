#include "nodewidget.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QResizeEvent>
#include <QGraphicsScene>

#include "ui/nodeui.h"

NodeWidget::NodeWidget(NodeUI *parent) :
  parent_(parent)
{
  QVBoxLayout* layout = new QVBoxLayout(this);

  QLabel* title = new QLabel("Node");
  layout->addWidget(title);

  QGridLayout* grid_layout = new QGridLayout();

  grid_layout->addWidget(new QLabel("Option 1"), 0, 0);
  grid_layout->addWidget(new QLineEdit("Value 1"), 0, 1);

  grid_layout->addWidget(new QLabel("Option 2"), 1, 0);
  grid_layout->addWidget(new QLineEdit("Value 2"), 1, 1);

  layout->addLayout(grid_layout);
}

void NodeWidget::resizeEvent(QResizeEvent *event)
{
  parent_->Resize(event->size());
}

bool NodeWidget::event(QEvent *event)
{
  if ((event->type() == QEvent::MouseButtonPress
      || event->type() == QEvent::MouseButtonRelease
      || event->type() == QEvent::MouseMove
      || event->type() == QEvent::MouseButtonDblClick)
      && parent_->scene()->sendEvent(parent_, event)) {
    return true;
  }
  return QWidget::event(event);
}
