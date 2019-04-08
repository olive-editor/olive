#ifndef NODEUI_H
#define NODEUI_H

#include <QWidget>
#include <QGraphicsItem>

class NodeUI : public QGraphicsRectItem {
public:
  NodeUI();
  virtual ~NodeUI() override;

  void AddToScene(QGraphicsScene* scene);
  void Resize(const QSize& s);
  void SetWidget(QWidget* widget);
protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
private:
  QWidget* central_widget_;
  QGraphicsProxyWidget* proxy_;
  QPainterPath path_;
};

#endif // NODEUI_H
