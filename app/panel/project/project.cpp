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
  explorer_ = new ProjectExplorer(this);
  layout->addWidget(explorer_);

  // Set strings
  Retranslate();
}

Project *ProjectPanel::project()
{
  return explorer_->project();
}

void ProjectPanel::set_project(Project *p)
{
  explorer_->set_project(p);
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
