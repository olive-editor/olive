#include "tool.h"

#include "core.h"
#include "widget/toolbar/toolbar.h"

ToolPanel::ToolPanel(QWidget *parent) :
  PanelWidget(parent)
{
  Toolbar* t = new Toolbar(this);

  setWidget(t);

  connect(t, SIGNAL(ToolChanged(const olive::tool::Tool&)), &olive::core, SLOT(SetTool(const olive::tool::Tool&)));
  connect(&olive::core, SIGNAL(ToolChanged(const olive::tool::Tool&)), t, SLOT(SetTool(const olive::tool::Tool&)));

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
