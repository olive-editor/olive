#include "projectexplorer.h"

ProjectExplorer::ProjectExplorer(QWidget *parent) :
  QWidget(parent),
  view_type_(TreeView)
{
}

const ProjectExplorer::ViewType &ProjectExplorer::view_type()
{
  return view_type_;
}

void ProjectExplorer::set_view_type(const ProjectExplorer::ViewType &type)
{
  view_type_ = type;
}
