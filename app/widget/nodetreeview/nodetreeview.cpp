#include "nodetreeview.h"

OLIVE_NAMESPACE_ENTER

NodeTreeView::NodeTreeView(QWidget *parent) :
  QTreeWidget(parent),
  only_show_keyframable_(false)
{
  connect(this, &NodeTreeView::itemChanged, this, &NodeTreeView::ItemCheckStateChanged);

  Retranslate();
}

bool NodeTreeView::IsNodeEnabled(Node *n) const
{
  return !disabled_nodes_.contains(n);
}

bool NodeTreeView::IsInputEnabled(NodeInput *i) const
{
  return !disabled_inputs_.contains(i);
}

void NodeTreeView::SetNodes(const QVector<Node *> &nodes)
{
  nodes_ = nodes;

  this->clear();

  foreach (Node* n, nodes_) {
    QTreeWidgetItem* node_item = new QTreeWidgetItem();
    node_item->setText(0, n->Name());
    node_item->setCheckState(0, disabled_nodes_.contains(n) ? Qt::Unchecked : Qt::Checked);
    node_item->setData(0, kItemType, kItemTypeNode);
    node_item->setData(0, kItemPointer, reinterpret_cast<quintptr>(n));

    QVector<NodeInput*> inputs = n->GetInputsIncludingArrays();
    foreach (NodeInput* i, inputs) {
      if (only_show_keyframable_ && !i->is_keyframable()) {
        continue;
      }

      QTreeWidgetItem* input_item = new QTreeWidgetItem(node_item);
      input_item->setText(0, i->name());
      input_item->setCheckState(0, disabled_inputs_.contains(i) ? Qt::Unchecked : Qt::Checked);
      input_item->setData(0, kItemType, kItemTypeInput);
      input_item->setData(0, kItemPointer, reinterpret_cast<quintptr>(i));
    }

    // Add at the end to prevent unnecessary signalling while we're setting these objects up
    if (node_item->childCount() > 0) {
      this->addTopLevelItem(node_item);
    } else {
      delete node_item;
    }
  }
}

void NodeTreeView::changeEvent(QEvent *e)
{
  QTreeWidget::changeEvent(e);

  if (e->type() == QEvent::LanguageChange) {
    Retranslate();
  }
}

void NodeTreeView::Retranslate()
{
  setHeaderLabel(tr("Nodes"));
}

void NodeTreeView::ItemCheckStateChanged(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column)

  switch (item->data(0, kItemType).toInt()) {
  case kItemTypeNode:
  {
    Node* n = reinterpret_cast<Node*>(item->data(0, kItemPointer).value<quintptr>());

    if (item->checkState(0) == Qt::Checked) {
      if (disabled_nodes_.contains(n)) {
        disabled_nodes_.removeOne(n);
        emit NodeEnableChanged(n, true);
      }
    } else if (!disabled_nodes_.contains(n)) {
      disabled_nodes_.append(n);
      emit NodeEnableChanged(n, false);
    }
    break;
  }
  case kItemTypeInput:
  {
    NodeInput* i = reinterpret_cast<NodeInput*>(item->data(0, kItemPointer).value<quintptr>());

    if (item->checkState(0) == Qt::Checked) {
      if (disabled_inputs_.contains(i)) {
        disabled_inputs_.removeOne(i);
        emit InputEnableChanged(i, true);
      }
    } else if (!disabled_inputs_.contains(i)) {
      disabled_inputs_.append(i);
      emit InputEnableChanged(i, false);
    }
    break;
  }
  }
}

OLIVE_NAMESPACE_EXIT
