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

  void SetMulticamNode(MultiCamNode *n)
  {
    node_ = n;
  }

protected:
  virtual RenderTicketPtr GetSingleFrame(const rational &t, bool dry = false) override;

private:
  MultiCamNode *node_;

private slots:
  void DisplayClicked(const QPoint &p);

};

}

#endif // MULTICAMWIDGET_H
