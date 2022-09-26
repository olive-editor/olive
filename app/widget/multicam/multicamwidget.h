#ifndef MULTICAMWIDGET_H
#define MULTICAMWIDGET_H

#include "node/input/multicam/multicamnode.h"
#include "render/previewautocacher.h"
#include "widget/viewer/viewer.h"

namespace olive {

class MulticamWidget : public ViewerWidget
{
  Q_OBJECT
public:
  explicit MulticamWidget(QWidget *parent = nullptr);

private:
  //MultiCamNode *node_;

};

}

#endif // MULTICAMWIDGET_H
