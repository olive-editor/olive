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

#ifndef TIMEOFFSETNODE_H
#define TIMEOFFSETNODE_H

#include "node/node.h"

namespace olive {

class TimeOffsetNode : public Node
{
public:
  TimeOffsetNode();

  NODE_DEFAULT_FUNCTIONS(TimeOffsetNode)

  virtual QString Name() const override
  {
    return tr("Time Offset");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.timeoffset");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryTime};
  }

  virtual QString Description() const override
  {
    return tr("Offset time passing through the graph.");
  }

  virtual TimeRange InputTimeAdjustment(const QString& input, int element, const TimeRange& input_time, bool clamp) const override;
  virtual TimeRange OutputTimeAdjustment(const QString& input, int element, const TimeRange& input_time) const override;

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  static const QString kTimeInput;
  static const QString kInputInput;

private:
  rational GetRemappedTime(const rational& input) const;

};

}

#endif // TIMEOFFSETNODE_H
