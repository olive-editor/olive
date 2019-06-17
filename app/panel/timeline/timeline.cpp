#include "timeline.h"

TimelinePanel::TimelinePanel(QWidget *parent) :
  PanelWidget(parent)
{
  Retranslate();
}

void TimelinePanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void TimelinePanel::Retranslate()
{
  SetTitle(tr("Timeline"));
  SetSubtitle(tr("(none)"));
}
