#include "nodevaluetree.h"

#include <QEvent>
#include <QPushButton>

#include "core.h"
#include "node/nodeundo.h"

namespace olive {

#define super QTreeWidget

NodeValueTree::NodeValueTree(QWidget *parent) :
  super(parent)
{
  setColumnWidth(0, 0);
  setColumnCount(2);

  QSizePolicy p = sizePolicy();
  p.setHorizontalStretch(1);
  setSizePolicy(p);

  static const int kMinimumRows = 10;
  setMinimumHeight(fontMetrics().height() * kMinimumRows);

  Retranslate();
}

void NodeValueTree::SetNode(const NodeInput &input)
{
  if (input_ == input) {
    return;
  }

  if (Node *n = input_.node()) {
    disconnect(n, &Node::InputValueHintChanged, this, &NodeValueTree::ValueHintChanged);
  }

  clear();

  input_ = input;

  NodeOutput output = input.GetConnectedOutput2();
  if (output.IsValid()) {
    Node *connected_node = output.node();
    connected_node->Retranslate();

    connect(input_.node(), &Node::InputValueHintChanged, this, &NodeValueTree::ValueHintChanged);

    const QVector<Node::Output> &outputs = connected_node->outputs();

    Node::ValueHint vh = input.node()->GetValueHintForInput(input.input(), input.element());

    for (const Node::Output &o : outputs) {
      // Add default output
      QTreeWidgetItem *item = new QTreeWidgetItem(this);

      item->setText(1, o.name);

      QRadioButton *radio = new QRadioButton(this);
      radio->setProperty("input", QVariant::fromValue(input));
      radio->setProperty("output", o.id);
      connect(radio, &QRadioButton::clicked, this, &NodeValueTree::RadioButtonChecked);

      if (output.output() == o.id) {
        radio->setChecked(true);
      }

      setItemWidget(item, 0, radio);

      QTreeWidgetItem *swizzler_items = new QTreeWidgetItem(item);
      swizzler_items->setFlags(Qt::NoItemFlags);

      ValueSwizzleWidget *b = new ValueSwizzleWidget();
      b->set(ValueParams(), input_);
      //connect(b, &ValueSwizzleWidget::value_changed, this, &NodeValueTree::SwizzleChanged);
      this->setItemWidget(swizzler_items, 1, b);
    }
  }
}

bool NodeValueTree::DeleteSelected()
{
  for (int i = 0; i < topLevelItemCount(); i++) {
    ValueSwizzleWidget *w = GetSwizzleWidgetFromTopLevelItem(i);
    if (w->DeleteSelected()) {
      return true;
    }
  }

  return false;
}

void NodeValueTree::changeEvent(QEvent *event)
{
  if (event->type() == QEvent::LanguageChange) {
    Retranslate();
  }

  super::changeEvent(event);
}

void NodeValueTree::Retranslate()
{
  setHeaderLabels({QString(), tr("Output")});
}

ValueSwizzleWidget *NodeValueTree::GetSwizzleWidgetFromTopLevelItem(int i)
{
  return static_cast<ValueSwizzleWidget *>(itemWidget(topLevelItem(i)->child(0), 1));
}

void NodeValueTree::RadioButtonChecked(bool e)
{
  if (e) {
    QRadioButton *btn = static_cast<QRadioButton*>(sender());
    QString tag = btn->property("output").toString();
    NodeInput input = btn->property("input").value<NodeInput>();

    NodeOutput old_output = input.GetConnectedOutput2();
    MultiUndoCommand *command = new MultiUndoCommand();
    command->add_child(new NodeEdgeRemoveCommand(old_output, input));

    old_output.set_output(tag);
    command->add_child(new NodeEdgeAddCommand(old_output, input));

    Core::instance()->undo_stack()->push(command, tr("Switched Connected Output Parameter"));
  }
}

void NodeValueTree::Update()
{
  SetNode(input_);
}

void NodeValueTree::SwizzleChanged(const SwizzleMap &map)
{
  if (input_.IsValid()) {
    Node::ValueHint hint = input_.node()->GetValueHintForInput(input_.input(), input_.element());
    hint.set_swizzle(map);
    Core::instance()->undo_stack()->push(new NodeSetValueHintCommand(input_, hint), tr("Edited Channel Swizzle For Input"));
  }
}

void NodeValueTree::ValueHintChanged(const NodeInput &input)
{
  if (input == input_) {
    Node::ValueHint vh = input.node()->GetValueHintForInput(input.input(), input.element());

    NodeOutput output = input.GetConnectedOutput2();

    for (int i = 0; i < this->topLevelItemCount(); i++) {
      QTreeWidgetItem *item = this->topLevelItem(i);
      QRadioButton *rb = static_cast<QRadioButton *>(this->itemWidget(item, 0));

      if (rb->property("output").toString() == output.output()) {
        rb->setChecked(true);
      }
    }
  }
}

}
