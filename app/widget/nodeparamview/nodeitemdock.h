#ifndef NODEITEMDOCK_H
#define NODEITEMDOCK_H

#include <QDockWidget>
#include "common/define.h"
#include "node/node.h"

OLIVE_NAMESPACE_ENTER

class NodeItemDock : public QDockWidget {
  Q_OBJECT

 public:
  NodeItemDock(const QString &title = "", QWidget *parent = nullptr);

 protected:
  void closeEvent(QCloseEvent *event);
 signals:
  void Closed(Node* node);
};

OLIVE_NAMESPACE_EXIT

#endif
