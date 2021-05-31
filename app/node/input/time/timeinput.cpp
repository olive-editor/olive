/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

namespace olive {

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
  return tr("Generates the time (in seconds) at this frame.");
}

NodeValueTable TimeInput::Value(const QString &output, NodeValueDatabase &value) const
{
  Q_UNUSED(output)

  NodeValueTable table = value.Merge();

  table.Push(NodeValue::kFloat,
             value[QStringLiteral("global")].Get(NodeValue::kFloat, QStringLiteral("time_in")),
             this,
             false,
             QStringLiteral("time"));

  return table;
}

void TimeInput::Hash(const QString &output, QCryptographicHash &hash, const rational &time, const VideoParams &video_params) const
{
  Node::Hash(output, hash, time, video_params);

  // Make sure time is hashed
  hash.addData(NodeValue::ValueToBytes(NodeValue::kRational, QVariant::fromValue(time)));
}

}
