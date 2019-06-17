#include "panel.h"

PanelWidget::PanelWidget(QWidget *parent) :
  QDockWidget(parent)
{
}

void PanelWidget::SetTitle(const QString &t)
{
  title_ = t;
  UpdateTitle();
}

void PanelWidget::SetSubtitle(const QString &t)
{
  subtitle_ = t;
  UpdateTitle();
}

void PanelWidget::UpdateTitle()
{
  if (subtitle_.isEmpty()) {
    setWindowTitle(title_);
  } else {
    setWindowTitle(tr("%1: %2").arg(title_, subtitle_));
  }
}
