#ifndef NODEUI_H
#define NODEUI_H

#include <QWidget>
#include <QGraphicsItem>

class EffectUI;

class NodeUI : public QGraphicsRectItem {
public:
  NodeUI();
  virtual ~NodeUI() override;

  void AddToScene(QGraphicsScene* scene);
  void Resize(const QSize& s);
  void SetWidget(EffectUI* widget);
protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void mousePressEvent(QGraphicsSceneMouseEvent * event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
private:
  QVector<QRectF> GetNodeSocketRects();
  QPainterPath GetEdgePath(const QPointF& start_pos, const QPointF& end_pos);

  EffectUI* central_widget_;
  QGraphicsProxyWidget* proxy_;
  QPainterPath path_;

  QGraphicsPathItem* drag_line_;
  QPointF drag_line_start_;

  int clicked_socket_;
};

#endif // NODEUI_H
