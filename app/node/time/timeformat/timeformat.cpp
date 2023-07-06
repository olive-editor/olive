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
  AddInput(kTimeInput, TYPE_DOUBLE);
  AddInput(kFormatInput, TYPE_STRING, QStringLiteral("hh:mm:ss"));
  AddInput(kLocalTimeInput, TYPE_BOOL);
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

value_t TimeFormatNode::Value(const ValueParams &p) const
{
  qint64 ms_since_epoch = GetInputValue(p, kTimeInput).toDouble()*1000;
  bool time_is_local = GetInputValue(p, kLocalTimeInput).toBool();
  QDateTime dt = QDateTime::fromMSecsSinceEpoch(ms_since_epoch, time_is_local ? Qt::LocalTime : Qt::UTC);
  QString format = GetInputValue(p, kFormatInput).toString();
  return dt.toString(format);
}

}
