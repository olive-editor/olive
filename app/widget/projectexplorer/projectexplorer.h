#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include <QStackedWidget>
#include <QTreeView>

#include "project/project.h"
#include "project/projectviewmodel.h"

class ProjectExplorer : public QStackedWidget
{
public:
  enum ViewType {
    TreeView,
    ListView,
    IconView
  };

  ProjectExplorer(QWidget* parent);

  const ViewType& view_type();
  void set_view_type(const ViewType& type);

  Project* project();
  void set_project(Project* p);

private:
  QTreeView* tree_view_;

  ViewType view_type_;

  ProjectViewModel model_;
};

#endif // PROJECTEXPLORER_H
