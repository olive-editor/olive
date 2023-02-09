#include "nodevaluetree.h"

#include <QEvent>

#include "node/traverser.h"

namespace olive {

#define super QTreeWidget

NodeValueTree::NodeValueTree(QWidget *parent) :
  super(parent)
{
  setColumnWidth(0, 0);
  setColumnCount(4);

  QSizePolicy p = sizePolicy();
  p.setHorizontalStretch(1);
  setSizePolicy(p);

  static const int kMinimumRows = 10;
  setMinimumHeight(fontMetrics().height() * kMinimumRows);

  Retranslate();
}

void NodeValueTree::SetNode(const NodeInput &input, const rational &time)
{
  clear();

  NodeTraverser traverser;

  Node *connected_node = input.GetConnectedOutput();

  NodeValueTable table = traverser.GenerateTable(connected_node, TimeRange(time, time));

  int index = traverser.GenerateRowValueElementIndex(input.node(), input.input(), input.element(), &table);

  for (int i=0; i<table.Count(); i++) {
    const NodeValue &value = table.at(i);
    QTreeWidgetItem *item = new QTreeWidgetItem(this);

    Node::ValueHint hint({value.type()}, table.Count()-1-i, value.tag());

    QRadioButton *radio = new QRadioButton(this);
    radio->setProperty("input", QVariant::fromValue(input));
    radio->setProperty("hint", QVariant::fromValue(hint));
    if (i == index) {
      radio->setChecked(true);
    }
    connect(radio, &QRadioButton::clicked, this, &NodeValueTree::RadioButtonChecked);

    setItemWidget(item, 0, radio);
    item->setText(1, NodeValue::GetPrettyDataTypeName(value.type()));
    item->setText(2, NodeValue::ValueToString(value, false));
    item->setText(3, value.source()->GetLabelAndName());
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
  setHeaderLabels({QString(), tr("Type"), tr("Value"), tr("Source")});
}

void NodeValueTree::RadioButtonChecked(bool e)
{
  if (e) {
    QRadioButton *btn = static_cast<QRadioButton*>(sender());
    Node::ValueHint hint = btn->property("hint").value<Node::ValueHint>();
    NodeInput input = btn->property("input").value<NodeInput>();

    input.node()->SetValueHintForInput(input.input(), hint, input.element());
  }
}

}
