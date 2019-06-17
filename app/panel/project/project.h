#ifndef PROJECT_H
#define PROJECT_H

#include "widget/panel/panel.h"
#include "widget/projectexplorer/projectexplorer.h"

class ProjectPanel : public PanelWidget
{
  Q_OBJECT
public:
  ProjectPanel(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();
};

#endif // PROJECT_H
