#include "projectexplorer.h"

ProjectExplorer::ProjectExplorer(QWidget *parent) :
  QStackedWidget(parent),
  view_type_(TreeView)
{
  tree_view_ = new QTreeView(this);
  addWidget(tree_view_);
}

const ProjectExplorer::ViewType &ProjectExplorer::view_type()
{
  return view_type_;
}

void ProjectExplorer::set_view_type(const ProjectExplorer::ViewType &type)
{
  view_type_ = type;
}
