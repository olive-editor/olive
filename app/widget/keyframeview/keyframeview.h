#ifndef KEYFRAMEVIEW_H
#define KEYFRAMEVIEW_H

#include "keyframeviewbase.h"

class KeyframeView : public KeyframeViewBase
{
  Q_OBJECT
public:
  KeyframeView(QWidget* parent = nullptr);

public slots:
  void AddKeyframe(NodeKeyframePtr key, int y);

};

#endif // KEYFRAMEVIEW_H
