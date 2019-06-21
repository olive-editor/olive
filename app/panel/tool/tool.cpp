#include "tool.h"

#include "widget/toolbar/toolbar.h"

ToolPanel::ToolPanel(QWidget *parent) :
  PanelWidget(parent)
{
  Toolbar* t = new Toolbar(this);

  setWidget(t);

  Retranslate();
}

void ToolPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void ToolPanel::Retranslate()
{
  SetTitle(tr("Tools"));
}
