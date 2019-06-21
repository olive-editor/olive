#include "viewer.h"

#include "widget/viewer/viewer.h"

ViewerPanel::ViewerPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // QObject system handles deleting this
  ViewerWidget* viewer = new ViewerWidget(this);

  // Set ViewerWidget as the central widget
  setWidget(viewer);

  // Set strings
  Retranslate();
}

void ViewerPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void ViewerPanel::Retranslate()
{
  SetTitle(tr("Viewer"));
  SetSubtitle(tr("(none)"));
}
