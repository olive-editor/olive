/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "timeinput.h"

OLIVE_NAMESPACE_ENTER

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
  return QStringLiteral("org.olivevideoeditor.Olive.time");
}

QVector<Node::CategoryID> TimeInput::Category() const
{
  return {kCategoryInput};
}

QString TimeInput::Description() const
{
  return tr("Generates the time (in seconds) at this frame");
}

NodeValueTable TimeInput::Value(NodeValueDatabase &value) const
{
  NodeValueTable table = value.Merge();

  table.Push(NodeParam::kFloat,
             value[QStringLiteral("global")].Get(NodeParam::kFloat, QStringLiteral("time_in")),
             this,
             QStringLiteral("time"));

  return table;
}

void TimeInput::Hash(QCryptographicHash &hash, const rational &time) const
{
  Node::Hash(hash, time);

  // Make sure time is hashed
  hash.addData(NodeParam::ValueToBytes(NodeParam::kRational, QVariant::fromValue(time)));
}

OLIVE_NAMESPACE_EXIT
