#include "timeinput.h"

TimeInput::TimeInput()
{
}

Node *TimeInput::copy() const
{
  return new TimeInput();
}

QString TimeInput::Name() const
{
  return tr("Time");
}

QString TimeInput::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.videoinput");
}

QString TimeInput::Category() const
{
  return tr("Input");
}

QString TimeInput::Description() const
{
  return tr("Generates the time (in seconds) at this frame");
}

NodeValueTable TimeInput::Value(const NodeValueDatabase &value) const
{
  NodeValueTable table = value.Merge();

  table.Push(NodeParam::kFloat,
             value[QStringLiteral("global")].Get(NodeParam::kFloat, QStringLiteral("time_in")),
             QStringLiteral("time"));

  return table;
}
