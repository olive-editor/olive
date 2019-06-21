#include "projectexplorer.h"

ProjectExplorer::ProjectExplorer(QWidget *parent) :
  QStackedWidget(parent),
  view_type_(TreeView),
  model_(this)
{
  tree_view_ = new QTreeView(this);
  tree_view_->setModel(&model_);
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

Project *ProjectExplorer::project()
{
  return model_.project();
}

void ProjectExplorer::set_project(Project *p)
{
  model_.set_project(p);
}
