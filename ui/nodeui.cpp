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

const int kRoundedRectRadius = 5;

NodeUI::NodeUI() :
  central_widget_(nullptr)
{
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

NodeUI::~NodeUI()
{
  if (scene() != nullptr) {
    scene()->removeItem(proxy_);
    scene()->removeItem(this);
  }
}

void NodeUI::AddToScene(QGraphicsScene *scene)
{
  scene->addItem(this);

  if (central_widget_ != nullptr) {
    proxy_ = scene->addWidget(central_widget_);
    proxy_->setPos(pos() + QPoint(kRoundedRectRadius, kRoundedRectRadius));
    proxy_->setParentItem(this);
  }
}

void NodeUI::Resize(const QSize &s)
{
  QRectF rectangle = rect();

  rectangle.setSize(s + 2 * QSize(kRoundedRectRadius, kRoundedRectRadius));

  path_ = QPainterPath();
  path_.addRoundedRect(rectangle, kRoundedRectRadius, kRoundedRectRadius);

  setRect(rectangle);
}

void NodeUI::SetWidget(QWidget *widget)
{
  central_widget_ = widget;
}

void NodeUI::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)

  QPalette palette = qApp->palette();

  if (option->state & QStyle::State_Selected) {
    painter->setPen(palette.highlight().color());
  } else {
    painter->setPen(palette.base().color());
  }
  painter->setBrush(palette.window());
  painter->drawPath(path_);
}
