#ifndef PROJECTEXPLORER_H
#define PROJECTEXPLORER_H

#include <QStackedWidget>
#include <QTreeView>

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

private:
  ViewType view_type_;

  QTreeView* tree_view_;
};

#endif // PROJECTEXPLORER_H
