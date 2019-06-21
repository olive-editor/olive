#ifndef PROJECT_PANEL_H
#define PROJECT_PANEL_H

#include "project/project.h"
#include "widget/panel/panel.h"
#include "widget/projectexplorer/projectexplorer.h"

class ProjectPanel : public PanelWidget
{
  Q_OBJECT
public:
  ProjectPanel(QWidget* parent);

  Project* project();
  void set_project(Project* p);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();

  ProjectExplorer* explorer_;
};

#endif // PROJECT_H
