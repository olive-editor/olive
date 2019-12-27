#ifndef KEYFRAMEVIEWITEM_H
#define KEYFRAMEVIEWITEM_H

#include <QGraphicsRectItem>

#include "node/keyframe.h"

class KeyframeViewItem : public QObject, public QGraphicsRectItem
{
  Q_OBJECT
public:
  KeyframeViewItem(NodeKeyframePtr key, qreal vcenter, QGraphicsItem *parent = nullptr);

  void SetScale(double scale);

  NodeKeyframePtr key() const;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  NodeKeyframePtr key_;

  double scale_;

  qreal middle_;

  int keyframe_size_;

private slots:
  void UpdateRect();

  void Redraw();

};

#endif // KEYFRAMEVIEWITEM_H
