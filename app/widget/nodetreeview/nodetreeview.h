#ifndef NODETREEVIEW_H
#define NODETREEVIEW_H

#include <QTreeWidget>

#include "node/node.h"

OLIVE_NAMESPACE_ENTER

class NodeTreeView : public QTreeWidget
{
  Q_OBJECT
public:
  NodeTreeView(QWidget *parent = nullptr);

  bool IsNodeEnabled(Node* n) const;

  bool IsInputEnabled(NodeInput* i) const;

  void SetOnlyShowKeyframable(bool e)
  {
    only_show_keyframable_ = e;
  }

public slots:
  void SetNodes(const QList<Node*>& nodes);

signals:
  void NodeEnableChanged(Node* n, bool e);

  void InputEnableChanged(NodeInput* i, bool e);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();

  enum ItemType {
    kItemTypeNode,
    kItemTypeInput
  };

  static const int kItemType = Qt::UserRole;
  static const int kItemPointer = Qt::UserRole + 1;

  QList<Node*> nodes_;

  QList<Node*> disabled_nodes_;

  QList<NodeInput*> disabled_inputs_;

  bool only_show_keyframable_;

private slots:
  void ItemCheckStateChanged(QTreeWidgetItem* item, int column);

};

OLIVE_NAMESPACE_EXIT

#endif // NODETREEVIEW_H
