#ifndef KEYFRAMEVIEWITEM_H
#define KEYFRAMEVIEWITEM_H

#include <QGraphicsRectItem>

#include "node/keyframe.h"

class KeyframeViewItem : public QObject, public QGraphicsRectItem
{
  Q_OBJECT
public:
  KeyframeViewItem(NodeKeyframePtr key, QGraphicsItem *parent = nullptr);

  void SetOverrideY(qreal vertical_center);

  void SetScale(double scale);

  NodeKeyframePtr key() const;

  QPointF center_pos() const;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
  NodeKeyframePtr key_;

  double scale_;

  qreal vert_center_;

  int keyframe_size_;

private slots:
  void UpdatePos();

  void Redraw();

};

#endif // KEYFRAMEVIEWITEM_H
