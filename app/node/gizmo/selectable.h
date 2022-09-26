#ifndef SELECTABLE_H
#define SELECTABLE_H
#include "node/gizmo/point.h"

namespace olive {
class SelectableGizmo: public PointGizmo {
Q_OBJECT    
public:
  explicit SelectableGizmo(QObject *parent = nullptr);
};
}

#endif
