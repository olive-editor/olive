#include "projectexplorernavigation.h"

#include <QHBoxLayout>

#include "ui/icons/icons.h"

ProjectExplorerNavigation::ProjectExplorerNavigation(QWidget *parent) :
  QWidget(parent)
{
  QHBoxLayout* layout = new QHBoxLayout(this);

  dir_up_btn_ = new QPushButton(this);
  dir_up_btn_->setIcon(olive::icon::DirUp);
  layout->addWidget(dir_up_btn_);
}
