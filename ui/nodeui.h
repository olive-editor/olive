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
private:
  EffectUI* central_widget_;
  QGraphicsProxyWidget* proxy_;
  QPainterPath path_;
};

#endif // NODEUI_H
