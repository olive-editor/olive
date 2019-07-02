#ifndef PROJECTEXPLORERLISTVIEWTOOLBAR_H
#define PROJECTEXPLORERLISTVIEWTOOLBAR_H

#include <QPushButton>
#include <QWidget>

class ProjectExplorerNavigation : public QWidget
{
public:
  ProjectExplorerNavigation(QWidget* parent);

private:
  QPushButton* dir_up_btn_;
};

#endif // PROJECTEXPLORERLISTVIEWTOOLBAR_H
