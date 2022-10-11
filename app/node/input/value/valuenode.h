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

#ifndef VALUENODE_H
#define VALUENODE_H

#include "node/node.h"

namespace olive {

class ValueNode : public Node
{
  Q_OBJECT
public:
  ValueNode();

  NODE_DEFAULT_FUNCTIONS(ValueNode)

  virtual QString Name() const override
  {
    return tr("Value");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.value");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryGenerator};
  }

  virtual QString Description() const override
  {
    return tr("Create a single value that can be connected to various other inputs.");
  }

  static const QString kTypeInput;
  static const QString kValueInput;

  virtual void Retranslate() override;

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

protected:
  virtual void InputValueChangedEvent(const QString &input, int element) override;

private:
  static const QVector<NodeValue::Type> kSupportedTypes;

};

}

#endif // VALUENODE_H
