#include "multicamnode.h"

namespace olive {

#define super Node

const QString MultiCamNode::kCurrentInput = QStringLiteral("current_in");
const QString MultiCamNode::kSourcesInput = QStringLiteral("sources_in");

MultiCamNode::MultiCamNode()
{
  AddInput(kCurrentInput, NodeValue::kInt, InputFlags(kInputFlagStatic));

  AddInput(kSourcesInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable | kInputFlagArray));
}

QString MultiCamNode::Name() const
{
  return tr("Multi-Cam");
}

QString MultiCamNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.multicam");
}

QVector<Node::CategoryID> MultiCamNode::Category() const
{
  return {kCategoryTimeline};
}

QString MultiCamNode::Description() const
{
  return tr("Allows easy switching between multiple sources.");
}

Node::ActiveElements MultiCamNode::GetActiveElementsAtTime(const QString &input, const TimeRange &r) const
{
  if (input == kSourcesInput) {
    Node::ActiveElements a;
    a.add(GetStandardValue(kCurrentInput).toInt());
    return a;
  } else {
    return super::GetActiveElementsAtTime(input, r);
  }
}

void MultiCamNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  NodeValueArray arr = value[kSourcesInput].toArray();
  if (!arr.empty()) {
    table->Push(arr.begin()->second);
  }
}

void MultiCamNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kCurrentInput, tr("Current"));
  SetInputName(kSourcesInput, tr("Sources"));
}

}
