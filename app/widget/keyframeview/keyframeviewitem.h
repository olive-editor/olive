#ifndef KEYFRAMEVIEWITEM_H
#define KEYFRAMEVIEWITEM_H

#include <QGraphicsRectItem>

#include "node/keyframe.h"

class KeyframeViewItem : public QGraphicsRectItem
{
public:
  KeyframeViewItem(QGraphicsItem *parent = nullptr);

  void SetKeyframe(NodeKeyframe* key);

  void SetVerticalCenter(int middle);

  void SetScale(double scale);

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  void UpdateRect();

  NodeKeyframe* key_;

  double scale_;

  int middle_;

  int keyframe_size_;
};

#endif // KEYFRAMEVIEWITEM_H
