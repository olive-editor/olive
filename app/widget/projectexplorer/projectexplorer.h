#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include <QWidget>

class ProjectExplorer : public QWidget
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

private:
  ViewType view_type_;

};

#endif // PROJECTEXPLORER_H
