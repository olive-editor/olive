/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "timeformat.h"

#include <QDateTime>

namespace olive {

#define super Node

const QString TimeFormatNode::kTimeInput = QStringLiteral("time_in");
const QString TimeFormatNode::kFormatInput = QStringLiteral("format_in");
const QString TimeFormatNode::kLocalTimeInput = QStringLiteral("localtime_in");

TimeFormatNode::TimeFormatNode()
{
  AddInput(kTimeInput, NodeValue::kFloat);
  AddInput(kFormatInput, NodeValue::kText, QStringLiteral("hh:mm:ss"));
  AddInput(kLocalTimeInput, NodeValue::kBoolean);
}

QString TimeFormatNode::Name() const
{
  return tr("Time Format");
}

QString TimeFormatNode::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.timeformat");
}

QVector<Node::CategoryID> TimeFormatNode::Category() const
{
  return {kCategoryGenerator};
}

QString TimeFormatNode::Description() const
{
  return tr("Format time (in Unix epoch seconds) into a string.");
}

void TimeFormatNode::Retranslate()
{
  super::Retranslate();

  SetInputName(kTimeInput, tr("Time"));
  SetInputName(kFormatInput, tr("Format"));
  SetInputName(kLocalTimeInput, tr("Interpret time as local time"));
}

void TimeFormatNode::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const
{
  qint64 ms_since_epoch = value[kTimeInput].toDouble()*1000;
  bool time_is_local = value[kLocalTimeInput].toBool();
  QDateTime dt = QDateTime::fromMSecsSinceEpoch(ms_since_epoch, time_is_local ? Qt::LocalTime : Qt::UTC);
  QString format = value[kFormatInput].toString();
  QString output = dt.toString(format);
  table->Push(NodeValue(NodeValue::kText, output, this));
}

}
