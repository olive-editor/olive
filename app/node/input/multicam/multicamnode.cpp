#include "multicamnode.h"

namespace olive {

#define super Node

const QString MultiCamNode::kCurrentInput = QStringLiteral("current_in");
const QString MultiCamNode::kSourcesInput = QStringLiteral("sources_in");

MultiCamNode::MultiCamNode()
{
  AddInput(kCurrentInput, NodeValue::kInt, InputFlags(kInputFlagStatic));

  // Make current index start at 1 instead of 0
  SetInputProperty(kCurrentInput, QStringLiteral("offset"), 1);
  SetInputProperty(kCurrentInput, QStringLiteral("min"), 0);

  AddInput(kSourcesInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable | kInputFlagArray));
  SetInputProperty(kSourcesInput, QStringLiteral("arraystart"), 1);
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
    int src = GetCurrentSource();
    if (src >= 0 && src < InputArraySize(kSourcesInput)) {
      Node::ActiveElements a;
      a.add(src);
      return a;
    } else {
      return ActiveElements::kNoElements;
    }
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

void MultiCamNode::IndexToRowCols(int index, int total_rows, int total_cols, int *row, int *col)
{
  Q_UNUSED(total_rows)

  *col = index%total_cols;
  *row = index/total_cols;
}

void MultiCamNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kCurrentInput, tr("Current"));
  SetInputName(kSourcesInput, tr("Sources"));
}

void MultiCamNode::GetRowsAndColumns(int sources, int *rows_in, int *cols_in)
{
  int &rows = *rows_in;
  int &cols = *cols_in;

  rows = 1;
  cols = 1;
  while (rows*cols < sources) {
    if (rows < cols) {
      rows++;
    } else {
      cols++;
    }
  }
}

}
