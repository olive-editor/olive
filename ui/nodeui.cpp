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

#include "ui/effectui.h"

const int kRoundedRectRadius = 5;
const int kNodePlugSize = 6;

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
    proxy_->setPos(pos() + QPoint(1 + kRoundedRectRadius, 1 + kRoundedRectRadius));
    proxy_->setParentItem(this);
  }
}

void NodeUI::Resize(const QSize &s)
{
  QRectF rectangle;
  rectangle.setTopLeft(pos());
  rectangle.setSize(s + 2 * QSize(kRoundedRectRadius, kRoundedRectRadius));

  QRectF inner_rect = rectangle;
  inner_rect.translate(kNodePlugSize / 2, 0);

  rectangle.setWidth(rectangle.width() + kNodePlugSize);

  path_ = QPainterPath();
  path_.addRoundedRect(inner_rect, kRoundedRectRadius, kRoundedRectRadius);

  setRect(rectangle);
}

void NodeUI::SetWidget(EffectUI *widget)
{
  central_widget_ = widget;
}

void NodeUI::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget)

  if (proxy_ != nullptr) {
    Effect* e = central_widget_->GetEffect();

    for (int i=0;i<e->row_count();i++) {
      if (e->row(i)->CanConnectNodes()) {
        int y = central_widget_->GetRowY(i);

        painter->setPen(Qt::black);
        painter->setBrush(Qt::gray);
        painter->drawEllipse(QPointF(rect().x() + kNodePlugSize / 2, proxy_->pos().y() + y), kNodePlugSize, kNodePlugSize);
      }
    }
  }

  QPalette palette = qApp->palette();

  if (option->state & QStyle::State_Selected) {
    painter->setPen(palette.highlight().color());
  } else {
    painter->setPen(palette.base().color());
  }
  painter->setBrush(palette.window());
  painter->drawPath(path_);
}
