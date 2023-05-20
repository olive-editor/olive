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

  if (Node *connected_node = input.GetConnectedOutput()) {
    connected_node->Retranslate();
    connect(connected_node, &Node::InputValueHintChanged, this, &NodeValueTree::ValueHintChanged);

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

      if (vh.tag() == o.id) {
        radio->setChecked(true);
      }

      setItemWidget(item, 0, radio);

      QTreeWidgetItem *swizzler_items = new QTreeWidgetItem(item);
      swizzler_items->setFlags(Qt::NoItemFlags);

      ValueSwizzleWidget *b = new ValueSwizzleWidget();
      b->set_channels(4, 4);
      b->set(vh.swizzle());
      connect(b, &ValueSwizzleWidget::value_changed, this, &NodeValueTree::SwizzleChanged);
      this->setItemWidget(swizzler_items, 1, b);
    }
  }
}

bool NodeValueTree::DeleteSelected()
{
  for (int i = 0; i < topLevelItemCount(); i++) {
    ValueSwizzleWidget *w = GetSwizzleWidgetFromTopLevelItem(i);
    if (w->delete_selected()) {
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

    Node::ValueHint hint = input.node()->GetValueHintForInput(input.input(), input.element());
    hint.set_tag(tag);
    Core::instance()->undo_stack()->push(new NodeSetValueHintCommand(input, hint), tr("Switched Connected Output Parameter"));
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
    for (int i = 0; i < this->topLevelItemCount(); i++) {
      QTreeWidgetItem *item = this->topLevelItem(i);
      QRadioButton *rb = static_cast<QRadioButton *>(this->itemWidget(item, 0));

      if (rb->property("output").toString() == vh.tag()) {
        rb->setChecked(true);

        ValueSwizzleWidget *b = GetSwizzleWidgetFromTopLevelItem(i);
        b->set(vh.swizzle());
      }
    }
  }
}

}
