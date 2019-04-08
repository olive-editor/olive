#include "nodeui.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>
#include <QPainter>

NodeUI::NodeUI() :
  central_widget_(this)
{
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

void NodeUI::AddToScene(QGraphicsScene *scene)
{
  scene->addItem(this);

  QGraphicsProxyWidget* proxy = scene->addWidget(&central_widget_);
  proxy->setPos(pos());
  proxy->setParentItem(this);
}

void NodeUI::Resize(const QSize &s)
{
  QRectF rectangle = rect();

  rectangle.setSize(s);

  setRect(rectangle);
}

void NodeUI::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  QPalette palette = qApp->palette();

  if (option->state & QStyle::State_Selected) {
    painter->setPen(palette.highlight().color());
  } else {
    painter->setPen(palette.base().color());
  }
  painter->setBrush(palette.window());

  QRectF r = rect();
  r.setX(r.x() - 1);
  r.setY(r.y() - 1);
  r.setRight(r.right() + 1);
  r.setBottom(r.bottom() + 1);
  painter->drawRect(r);
}
