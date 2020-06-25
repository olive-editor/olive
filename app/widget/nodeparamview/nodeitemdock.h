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

  /**
   * @brief Return a pointer to the close button
   */
  QPushButton* ReturnCloseButton();

  /**
   * @brief Return a pointer to the collapse button
   */
  CollapseButton* ReturnCollapseButton();


private:
  /**
   * @brief Custom event filter to pass mouse events onto the QDockWidget
   */
  bool eventFilter(QObject *pObject, QEvent *pEvent);

  /**
   * @brief Title bar base widget
   */
  NodeParamViewItemTitleBar *title_bar_;

  /**
   * @brief QDockWidget's label, either node type or custom label set by the user
   */
  QLabel* title_bar_lbl_;

  /**
   * @brief Button to collapse the contents of the QDockWidget
   */
  CollapseButton* title_bar_collapse_btn_;

  /**
   * @brief Button to close the QDockwidget
   */
  QPushButton* close_button_;

  /**
   * @brief Node asscoiated with the QDockWidget
   */
  Node* node_;

private slots:
  void Retranslate();
};

class NodeItemDock : public QDockWidget {
  Q_OBJECT

public:
  NodeItemDock(Node *node, QWidget *parent = nullptr);

  /**
   * @brief Return a pointer to the title bar widget
   */
  NodeItemDockTitle *GetTitleBar();

signals:
  /**
   * @brief Connected to NodeParamView RemoveNode. This cleans everything up when a dock item is removed
   */ 
  void Closed(Node *node);

private:
  /**
   * @brief Node associated with this dock item
   */
  Node *node_;

  /**
   * @brief Custom title bar for a NodeItemDock
   */
  NodeItemDockTitle *titlebar_;

private slots:
  /**
   * @brief Connected to the close button on the titlebar. Closes the widget.
   */
  void Close();
};

OLIVE_NAMESPACE_EXIT

#endif
