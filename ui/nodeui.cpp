#include "nodeui.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
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

  QVector<QRectF> sockets = GetNodeSocketRects();
  if (!sockets.isEmpty()) {
    painter->setPen(Qt::black);
    painter->setBrush(Qt::gray);

    for (int i=0;i<sockets.size();i++) {
      painter->drawEllipse(sockets.at(i));
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

void NodeUI::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QVector<QRectF> sockets = GetNodeSocketRects();

  bool clicked_socket = false;

  for (int i=0;i<sockets.size();i++) {
    if (sockets.at(i).contains(event->pos())) {
      clicked_socket = true;
      break;
    }
  }

  if (clicked_socket) {
    qDebug() << "CLICKED SOCKET!";
  } else {
    QGraphicsItem::mousePressEvent(event);
  }
}

QVector<QRectF> NodeUI::GetNodeSocketRects()
{
  QVector<QRectF> rects;

  if (proxy_ != nullptr) {
    Effect* e = central_widget_->GetEffect();

    for (int i=0;i<e->row_count();i++) {
      if (e->row(i)->CanConnectNodes()) {
        int y = central_widget_->GetRowY(i);

        rects.append(QRectF(rect().x(),
                            proxy_->pos().y() + y - kNodePlugSize/2,
                            kNodePlugSize,
                            kNodePlugSize));


      }
    }
  }

  return rects;
}
