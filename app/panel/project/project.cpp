#include "project.h"

#include <QVBoxLayout>

#include "widget/projecttoolbar/projecttoolbar.h"

ProjectPanel::ProjectPanel(QWidget *parent) :
  PanelWidget(parent)
{
  // Create main widget and its layout
  QWidget* central_widget = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central_widget);
  layout->setMargin(0);
  layout->setSpacing(0);
  setWidget(central_widget);

  // Set up project toolbar
  ProjectToolbar* toolbar = new ProjectToolbar(this);
  layout->addWidget(toolbar);

  // Set up main explorer object
  ProjectExplorer* explorer = new ProjectExplorer(this);
  layout->addWidget(explorer);

  // Set strings
  Retranslate();
}

void ProjectPanel::changeEvent(QEvent *e)
{
  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
  QDockWidget::changeEvent(e);
}

void ProjectPanel::Retranslate()
{
  SetTitle(tr("Project"));
  SetSubtitle(tr("(untitled)"));
}
