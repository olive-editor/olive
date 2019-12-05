#include "value.h"

NodeValueDatabase::NodeValueDatabase()
{

}

NodeValueTable NodeValueDatabase::operator[](const QString &input_id) const
{
  return tables_.value(input_id);
}

NodeValueTable NodeValueDatabase::operator[](const NodeInput *input) const
{
  return tables_.value(input->id());
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

NodeValue::NodeValue(const NodeParam::DataType &type, const QVariant &data, const QString &tag) :
  type_(type),
  data_(data),
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

const QVariant &NodeValue::data() const
{
  return data_;
}

NodeValueTable::NodeValueTable()
{
}

QVariant NodeValueTable::Get(const NodeParam::DataType &type, const QString &tag) const
{
  int value_index = GetInternal(type, tag);

  if (value_index >= 0) {
    return values_.at(value_index).data();
  }

  return QVariant();
}

QVariant NodeValueTable::Take(const NodeParam::DataType &type, const QString &tag)
{
  int value_index = GetInternal(type, tag);

  if (value_index >= 0) {
    QVariant val = values_.at(value_index).data();
    values_.removeAt(value_index);
    return val;
  }

  return QVariant();
}

void NodeValueTable::Push(const NodeValue &value)
{
  values_.append(value);
}

void NodeValueTable::Push(const NodeParam::DataType &type, const QVariant &data, const QString &tag)
{
  Push(NodeValue(type, data, tag));
}

void NodeValueTable::Prepend(const NodeValue &value)
{
  values_.prepend(value);
}

void NodeValueTable::Prepend(const NodeParam::DataType &type, const QVariant &data, const QString &tag)
{
  Prepend(NodeValue(type, data, tag));
}

const NodeValue &NodeValueTable::At(int index) const
{
  return values_.at(index);
}

int NodeValueTable::Count() const
{
  return values_.size();
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

    merged_table.Prepend(t.At(row_index));
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
