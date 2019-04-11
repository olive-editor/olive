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

  EffectUI* central_widget_;
  QGraphicsProxyWidget* proxy_;
  QPainterPath path_;

  bool clicked_socket_;
};

#endif // NODEUI_H
