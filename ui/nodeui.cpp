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
#include "ui/nodeedgeui.h"

const int kRoundedRectRadius = 5;
const int kNodePlugSize = 12;

NodeUI::NodeUI() :
  central_widget_(nullptr),
  proxy_(nullptr),
  drag_destination_(nullptr),
  clicked_socket_(-1)
{
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
}

NodeUI::~NodeUI()
{
  if (proxy_ != nullptr) {
    proxy_->setParentItem(nullptr);
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

  drag_path_ = QPainterPath();
  drag_path_.addRoundedRect(inner_rect, kRoundedRectRadius, kRoundedRectRadius);

  setRect(rectangle);
}

void NodeUI::SetWidget(EffectUI *widget)
{
  central_widget_ = widget;
}

EffectUI *NodeUI::Widget()
{
  return central_widget_;
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
  painter->drawPath(drag_path_);
}

void NodeUI::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  QVector<QRectF> sockets = GetNodeSocketRects();

  clicked_socket_ = -1;
  drag_destination_ = nullptr;

  for (int i=0;i<sockets.size();i++) {
    if (sockets.at(i).contains(event->pos())) {
      clicked_socket_ = i;
      break;
    }
  }

  if (clicked_socket_ > -1) {

    // See if this socket already has an edge connected
    QVector<NodeEdgePtr> edges = central_widget_->GetEffect()->row(clicked_socket_)->edges();
    if (!edges.isEmpty()) {

      NodeEdge* e = edges.last().get();

      EffectRow* other_row = (e->input()->GetParentEffect() == central_widget_->GetEffect()) ?
            e->output() : e->input();

      Node* other_node = other_row->GetParentEffect();

      int other_row_index = other_node->IndexOfRow(other_row);

      NodeUI* other_node_ui = FindUIFromNode(other_node);

      // Start an edge at the opposite end
      drag_line_start_ = other_node_ui->pos() + other_node_ui->GetNodeSocketRects().at(other_row_index).center();

      drag_source_ = other_row;

      // Disconnect the existing edge, and treat our dynamic one as an edit of that one
      EffectRow::DisconnectEdge(edges.last());

    } else {

      // Start a new edge here
      drag_line_start_ = pos() + sockets.at(clicked_socket_).center();

      drag_source_ = central_widget_->GetEffect()->row(clicked_socket_);

    }

    drag_line_ = scene()->addPath(NodeEdgeUI::GetEdgePath(drag_line_start_, event->scenePos()));

    if (!edges.isEmpty()) {
      drag_line_->setPen(QPen(Qt::white, 2));
    } else {
      drag_line_->setPen(QPen(Qt::gray, 2));
    }

    event->accept();
  } else {
    QGraphicsItem::mousePressEvent(event);
  }
}

void NodeUI::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  if (clicked_socket_ > -1) {

    bool line_is_touching_node = false;
    drag_destination_ = nullptr;

    QPointF drag_line_end = event->scenePos();

    // Get all the graphics items at this mouse position
    QList<QGraphicsItem *> mouse_items = scene()->items(event->scenePos());
    for (int j=0;j<mouse_items.size();j++) {

      // See if the graphics item here is a node
      NodeUI* mouse_node = dynamic_cast<NodeUI*>(mouse_items.at(j));
      if (mouse_node != nullptr) {

        // If so, get this node's sockets and loop through them
        QVector<QRectF> mouse_nodes_sockets = mouse_node->GetNodeSocketRects();
        for (int i=0;i<mouse_nodes_sockets.size();i++) {

          // See if this socket contains the mouse cursor
          if (mouse_nodes_sockets.at(i).contains(event->scenePos() - mouse_node->pos())) {

            // If so, see if the two rows can be connected

            EffectRow* local_row = drag_source_;
            EffectRow* remote_row = mouse_node->GetRowFromIndex(i);

            // Ensure one is an input and one is an output and whether their data types are compatible
            if (local_row->IsNodeInput() != remote_row->IsNodeInput()) {

              // Determine which row is the input and which row is the output
              EffectRow* input_row = (local_row->IsNodeInput()) ? local_row : remote_row;
              EffectRow* output_row = (local_row->IsNodeInput()) ? remote_row : local_row;

              if (input_row->CanAcceptDataType(output_row->OutputDataType())) {
                // This is a valid connection
                line_is_touching_node = true;

                // Snap drag line to this socket
                drag_line_end = mouse_node->pos() + mouse_nodes_sockets.at(i).center();

                // Cancel loop since we have a connection now
                j = mouse_items.size();

                // Cache destination row
                drag_destination_ = remote_row;
              }
            }

            break;
          }
        }
      }
    }

    if (line_is_touching_node) {
      drag_line_->setPen(QPen(Qt::white, 2));
    } else {
      drag_line_->setPen(QPen(Qt::gray, 2));
    }

    drag_line_->setPath(NodeEdgeUI::GetEdgePath(drag_line_start_, drag_line_end));

    event->accept();

  } else {
    QGraphicsItem::mouseMoveEvent(event);
  }
}

void NodeUI::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  if (clicked_socket_ > -1) {
    scene()->removeItem(drag_line_);
    delete drag_line_;

    event->accept();

    if (drag_destination_ != nullptr) {
      EffectRow::ConnectEdge(drag_source_, drag_destination_);
    }
  } else {
    QGraphicsItem::mouseReleaseEvent(event);
  }
}

QVector<QRectF> NodeUI::GetNodeSocketRects()
{
  QVector<QRectF> rects;

  if (proxy_ != nullptr) {
    Node* e = central_widget_->GetEffect();

    for (int i=0;i<e->row_count();i++) {

      EffectRow* row = e->row(i);
      qreal x = (row->IsNodeOutput()) ? rect().right() - kNodePlugSize : rect().x();
      int y = central_widget_->GetRowY(i);

      if (row->IsNodeInput() || row->IsNodeOutput()) {
        rects.append(QRectF(x,
                            proxy_->pos().y() + y - kNodePlugSize/2,
                            kNodePlugSize,
                            kNodePlugSize));
      }
    }
  }

  return rects;
}

EffectRow *NodeUI::GetRowFromIndex(int i)
{
  if (central_widget_ != nullptr) {
    Node* e = central_widget_->GetEffect();
    if (i < e->row_count()) {
      return e->row(i);
    }
  }
  return nullptr;
}

NodeUI *NodeUI::FindUIFromNode(Node* n)
{
  QList<QGraphicsItem*> all_items = scene()->items();

  for (int j=0;j<all_items.size();j++) {
    NodeUI* node = dynamic_cast<NodeUI*>(all_items.at(j));
    if (node != nullptr) {

      // Check if this node has the specified

      if (node->Widget()->GetEffect() == n) {
        return node;
      }
    }
  }

  return nullptr;
}
