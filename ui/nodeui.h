#ifndef NODEUI_H
#define NODEUI_H

#include <QWidget>
#include <QGraphicsItem>

#include "ui/nodewidget.h"

class NodeUI : public QGraphicsRectItem {
public:
  NodeUI();
  void AddToScene(QGraphicsScene* scene);
  void Resize(const QSize& s);
protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  NodeWidget central_widget_;
};

#endif // NODEUI_H
