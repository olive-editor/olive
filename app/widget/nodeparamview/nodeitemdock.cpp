#include "nodeitemdock.h"
#include "widget/nodeparamview/nodeparamviewitem.h"

OLIVE_NAMESPACE_ENTER

NodeItemDock::NodeItemDock(const QString &title, QWidget *parent) :
    QDockWidget(title, parent)
{
}

void NodeItemDock::closeEvent(QCloseEvent *event) {
  emit Closed(static_cast<NodeParamViewItem *>(this->widget())->GetNode());

  QDockWidget::closeEvent(event);
}
OLIVE_NAMESPACE_EXIT
