#ifndef KEYFRAMEVIEWITEM_H
#define KEYFRAMEVIEWITEM_H

#include <QGraphicsRectItem>

#include "node/keyframe.h"
#include "timetargetobject.h"

class KeyframeViewItem : public QObject, public QGraphicsRectItem, public TimeTargetObject
{
  Q_OBJECT
public:
  KeyframeViewItem(NodeKeyframePtr key, QGraphicsItem *parent = nullptr);

  void SetOverrideY(qreal vertical_center);

  void SetScale(double scale);

  NodeKeyframePtr key() const;

protected:
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

  virtual void TimeTargetChangedEvent(Node* ) override;

private:
  NodeKeyframePtr key_;

  double scale_;

  qreal vert_center_;

private slots:
  void UpdatePos();

  void Redraw();

};

#endif // KEYFRAMEVIEWITEM_H
