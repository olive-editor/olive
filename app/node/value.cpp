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

#include "value.h"

OLIVE_NAMESPACE_ENTER

NodeValueTable& NodeValueDatabase::operator[](const QString &input_id)
{
  return tables_[input_id];
}

NodeValueTable& NodeValueDatabase::operator[](const NodeInput *input)
{
  return tables_[input->id()];
}

void NodeValueDatabase::Insert(const QString &key, const NodeValueTable &value)
{
  tables_.insert(key, value);
}

void NodeValueDatabase::Insert(const NodeInput *key, const NodeValueTable &value)
{
  tables_.insert(key->id(), value);
}

NodeValueTable NodeValueDatabase::Merge() const
{
  return NodeValueTable::Merge(tables_.values());
}

NodeValue::NodeValue() :
  type_(NodeParam::kNone),
  from_(nullptr)
{
}

NodeValue::NodeValue(const NodeParam::DataType &type, const QVariant &data, const Node *from, const QString &tag) :
  type_(type),
  data_(data),
  from_(from),
  tag_(tag)
{
}

const NodeParam::DataType &NodeValue::type() const
{
  return type_;
}

const QString &NodeValue::tag() const
{
  return tag_;
}

bool NodeValue::operator==(const NodeValue &rhs) const
{
  return type_ == rhs.type_ && tag_ == rhs.tag_ && data_ == rhs.data_;
}

const QVariant &NodeValue::data() const
{
  return data_;
}

QVariant NodeValueTable::Get(const NodeParam::DataType &type, const QString &tag) const
{
  return GetWithMeta(type, tag).data();
}

NodeValue NodeValueTable::GetWithMeta(const NodeParam::DataType &type, const QString &tag) const
{
  int value_index = GetInternal(type, tag);

  if (value_index >= 0) {
    return values_.at(value_index);
  }

  return NodeValue();
}

QVariant NodeValueTable::Take(const NodeParam::DataType &type, const QString &tag)
{
  return TakeWithMeta(type, tag).data();
}

NodeValue NodeValueTable::TakeWithMeta(const NodeParam::DataType &type, const QString &tag)
{
  int value_index = GetInternal(type, tag);

  if (value_index >= 0) {
    return values_.takeAt(value_index);
  }

  return NodeValue();
}

void NodeValueTable::Push(const NodeValue &value)
{
  values_.append(value);
}

void NodeValueTable::Push(const NodeParam::DataType &type, const QVariant &data, const Node* from, const QString &tag)
{
  Push(NodeValue(type, data, from, tag));
}

void NodeValueTable::Prepend(const NodeValue &value)
{
  values_.prepend(value);
}

void NodeValueTable::Prepend(const NodeParam::DataType &type, const QVariant &data, const Node* from, const QString &tag)
{
  Prepend(NodeValue(type, data, from, tag));
}

const NodeValue &NodeValueTable::at(int index) const
{
  return values_.at(index);
}

NodeValue NodeValueTable::TakeAt(int index)
{
  return values_.takeAt(index);
}

int NodeValueTable::Count() const
{
  return values_.size();
}

bool NodeValueTable::Has(const NodeParam::DataType &type) const
{
  for (int i=values_.size() - 1;i>=0;i--) {
    const NodeValue& v = values_.at(i);

    if (v.type() & type) {
      return true;
    }
  }

  return false;
}

void NodeValueTable::Remove(const NodeValue &v)
{
  for (int i=values_.size() - 1;i>=0;i--) {
    const NodeValue& compare = values_.at(i);

    if (compare == v) {
      values_.removeAt(i);
      return;
    }
  }
}

bool NodeValueTable::isEmpty() const
{
  return values_.isEmpty();
}

NodeValueTable NodeValueTable::Merge(QList<NodeValueTable> tables)
{


  if (tables.size() == 1) {
    return tables.first();
  }

  int row = 0;

  NodeValueTable merged_table;

  // Slipstreams all tables together
  // FIXME: I don't actually know if this is the right approach...
  foreach (const NodeValueTable& t, tables) {
    if (row >= t.Count()) {
      continue;
    }

    int row_index = t.Count() - 1 - row;

    merged_table.Prepend(t.at(row_index));
  }

  return merged_table;
}

int NodeValueTable::GetInternal(const NodeParam::DataType &type, const QString &tag) const
{
  int index = -1;

  for (int i=values_.size() - 1;i>=0;i--) {
    const NodeValue& v = values_.at(i);

    if (v.type() & type) {
      index = i;

      if (tag.isEmpty() || tag == v.tag()) {
        break;
      }
    }
  }

  return index;
}

OLIVE_NAMESPACE_EXIT
