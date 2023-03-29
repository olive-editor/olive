#include "nodevaluetree.h"

#include <QEvent>

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
  clear();

  input_ = input;

  if (Node *connected_node = input.GetConnectedOutput()) {
    connected_node->Retranslate();

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
    }
  }
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

void NodeValueTree::RadioButtonChecked(bool e)
{
  if (e) {
    QRadioButton *btn = static_cast<QRadioButton*>(sender());
    Node::ValueHint hint = btn->property("output").toString();
    NodeInput input = btn->property("input").value<NodeInput>();

    Core::instance()->undo_stack()->push(new NodeSetValueHintCommand(input, hint), tr("Switched Connected Output Parameter"));
  }
}

void NodeValueTree::Update()
{
  SetNode(input_);
}

}
