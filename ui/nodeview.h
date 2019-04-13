#ifndef NODEVIEW_H
#define NODEVIEW_H

#include <QGraphicsView>

#include "timeline/tracktypes.h"

class NodeView : public QGraphicsView {
  Q_OBJECT
public:
  NodeView(QGraphicsScene *scene, QWidget* parent = nullptr);

signals:
  void RequestContextMenu();

protected:
  virtual void mousePressEvent(QMouseEvent *event) override;
  virtual void mouseMoveEvent(QMouseEvent *event) override;
  virtual void mouseReleaseEvent(QMouseEvent *event) override;
  virtual void wheelEvent(QWheelEvent *event) override;

private:
  bool hand_moving_;
  QPoint drag_start_;

};

#endif // NODEVIEW_H
