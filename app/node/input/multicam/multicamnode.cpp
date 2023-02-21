#include "multicamnode.h"

#include "node/project/sequence/sequence.h"

namespace olive {

#define super Node

const QString MultiCamNode::kCurrentInput = QStringLiteral("current_in");
const QString MultiCamNode::kSourcesInput = QStringLiteral("sources_in");
const QString MultiCamNode::kSequenceInput = QStringLiteral("sequence_in");
const QString MultiCamNode::kSequenceTypeInput = QStringLiteral("sequence_type_in");

MultiCamNode::MultiCamNode()
{
  AddInput(kCurrentInput, NodeValue::kCombo, InputFlags(kInputFlagStatic));

  AddInput(kSourcesInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable | kInputFlagArray));
  SetInputProperty(kSourcesInput, QStringLiteral("arraystart"), 1);

  AddInput(kSequenceInput, NodeValue::kNone, InputFlags(kInputFlagNotKeyframable));
  AddInput(kSequenceTypeInput, NodeValue::kCombo, InputFlags(kInputFlagStatic | kInputFlagHidden));

  sequence_ = nullptr;
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
    if (src >= 0 && src < GetSourceCount()) {
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

Node *MultiCamNode::GetConnectedRenderOutput(const QString &input, int element) const
{
  if (sequence_ && input == kSourcesInput && element >= 0 && element < GetSourceCount()) {
    return GetTrackList()->GetTrackAt(element);
  } else {
    return Node::GetConnectedRenderOutput(input, element);
  }
}

bool MultiCamNode::IsInputConnectedForRender(const QString &input, int element) const
{
  if (sequence_ && input == kSourcesInput && element >= 0 && element < GetSourceCount()) {
    return true;
  } else {
    return Node::IsInputConnectedForRender(input, element);
  }
}

QVector<QString> MultiCamNode::IgnoreInputsForRendering() const
{
  return {kSequenceInput};
}

void MultiCamNode::InputConnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kSequenceInput) {
    if (Sequence *s = dynamic_cast<Sequence*>(output)) {
      SetInputFlag(kSequenceTypeInput, kInputFlagHidden, false);
      sequence_ = s;
    }
  }
}

void MultiCamNode::InputDisconnectedEvent(const QString &input, int element, Node *output)
{
  if (input == kSequenceInput) {
    SetInputFlag(kSequenceTypeInput, kInputFlagHidden, true);
    sequence_ = nullptr;
  }
}

TrackList *MultiCamNode::GetTrackList() const
{
  return sequence_->track_list(static_cast<Track::Type>(GetStandardValue(kSequenceTypeInput).toInt()));
}

void MultiCamNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kCurrentInput, tr("Current"));
  SetInputName(kSourcesInput, tr("Sources"));
  SetInputName(kSequenceInput, tr("Sequence"));
  SetInputName(kSequenceTypeInput, tr("Sequence Type"));
  SetComboBoxStrings(kSequenceTypeInput, {tr("Video"), tr("Audio")});

  QStringList names;
  int name_count = GetSourceCount();
  names.reserve(name_count);
  for (int i=0; i<name_count; i++) {
    QString src_name;
    if (Node *n = GetConnectedRenderOutput(kSourcesInput, i)) {
      src_name = n->Name();
    }
    names.append(tr("%1: %2").arg(QString::number(i+1), src_name));
  }
  SetComboBoxStrings(kCurrentInput, names);
}

int MultiCamNode::GetSourceCount() const
{
  if (sequence_) {
    return GetTrackList()->GetTrackCount();
  } else {
    return InputArraySize(kSourcesInput);
  }
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
