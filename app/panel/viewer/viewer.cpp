#include "viewer.h"

#include "widget/viewer/viewer.h"

ViewerPanel::ViewerPanel(QWidget *parent) :
  PanelWidget(parent)
{
  SetTitle(tr("Viewer"));
  SetSubtitle(tr("(None)"));

  // QObject system handles deleting this
  ViewerWidget* viewer = new ViewerWidget(this);

  // Set ViewerWidget as the central widget
  setWidget(viewer);
}
