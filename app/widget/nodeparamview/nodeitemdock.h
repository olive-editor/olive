#ifndef NODEITEMDOCK_H
#define NODEITEMDOCK_H

#include <QDockWidget>
#include <QEvent>
#include <QToolBar>

#include "common/define.h"
#include "node/node.h"
#include "widget/nodeparamview/nodeparamviewitem.h"

OLIVE_NAMESPACE_ENTER

class NodeItemDockTitle : public QWidget {
  Q_OBJECT

  public:
    NodeItemDockTitle(Node* node, QWidget *parent = nullptr);
    void SetNode(Node* node);
    QPushButton* ReturnCloseButton();


  private:
    bool eventFilter(QObject *pObject, QEvent *pEvent);

    NodeParamViewItemTitleBar *title_bar_;

    QLabel *title_bar_lbl_;

    CollapseButton *title_bar_collapse_btn_;

    QPushButton *close_button_;

    Node* node_;

  private slots:
    void Retranslate();
};

class NodeItemDock : public QDockWidget {
  Q_OBJECT

 public:
  NodeItemDock(Node *node, QWidget *parent = nullptr);

 signals:
  void Closed(Node *node);

 protected:
  void closeEvent(QCloseEvent *event);

 private:
  Node *node_;
  NodeItemDockTitle *titlebar_;

 private slots:
  void Close();
};

OLIVE_NAMESPACE_EXIT

#endif
