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

#include "group.h"

#include "node/serializeddata.h"

namespace olive {

#define super Node

NodeGroup::NodeGroup() :
  output_passthrough_(nullptr)
{
  SetFlag(kDontShowInCreateMenu);
}

QString NodeGroup::Name() const
{
  return tr("Group");
}

QString NodeGroup::id() const
{
  return QStringLiteral("org.olivevideoeditor.Olive.group");
}

QVector<Node::CategoryID> NodeGroup::Category() const
{
  return {kCategoryUnknown};
}

QString NodeGroup::Description() const
{
  return tr("A group of nodes that is represented as a single node.");
}

void NodeGroup::Retranslate()
{
  super::Retranslate();

  for (auto it=GetContextPositions().cbegin(); it!=GetContextPositions().cend(); it++) {
    it.key()->Retranslate();
  }
}

bool NodeGroup::LoadCustom(QXmlStreamReader *reader, SerializedData *data)
{
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("inputpassthroughs")) {
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("inputpassthrough")) {
          SerializedData::GroupLink link;

          link.group = this;

          while (XMLReadNextStartElement(reader)) {
            if (reader->name() == QStringLiteral("node")) {
              link.input_node = reader->readElementText().toULongLong();
            } else if (reader->name() == QStringLiteral("input")) {
              link.input_id = reader->readElementText();
            } else if (reader->name() == QStringLiteral("element")) {
              link.input_element = reader->readElementText().toInt();
            } else if (reader->name() == QStringLiteral("id")) {
              link.passthrough_id = reader->readElementText();
            } else if (reader->name() == QStringLiteral("name")) {
              link.custom_name = reader->readElementText();
            } else if (reader->name() == QStringLiteral("flags")) {
              link.custom_flags = InputFlags(reader->readElementText().toULongLong());
            } else if (reader->name() == QStringLiteral("type")) {
              link.data_type = NodeValue::GetDataTypeFromName(reader->readElementText());
            } else if (reader->name() == QStringLiteral("default")) {
              link.default_val = NodeValue::StringToValue(link.data_type, reader->readElementText(), false);
            } else if (reader->name() == QStringLiteral("properties")) {
              while (XMLReadNextStartElement(reader)) {
                if (reader->name() == QStringLiteral("property")) {
                  QString key;
                  QString value;

                  while (XMLReadNextStartElement(reader)) {
                    if (reader->name() == QStringLiteral("key")) {
                      key = reader->readElementText();
                    } else if (reader->name() == QStringLiteral("value")) {
                      value = reader->readElementText();
                    } else {
                      reader->skipCurrentElement();
                    }
                  }

                  if (!key.isEmpty()) {
                    link.custom_properties.insert(key, value);
                  }
                } else {
                  reader->skipCurrentElement();
                }
              }
            } else {
              reader->skipCurrentElement();
            }
          }

          data->group_input_links.append(link);
        } else {
          reader->skipCurrentElement();
        }
      }
    } else if (reader->name() == QStringLiteral("outputpassthrough")) {
      data->group_output_links.insert(this, reader->readElementText().toULongLong());
    } else {
      reader->skipCurrentElement();
    }
  }

  return true;
}

void NodeGroup::SaveCustom(QXmlStreamWriter *writer) const
{
  writer->writeStartElement(QStringLiteral("inputpassthroughs"));

  foreach (const NodeGroup::InputPassthrough &ip, this->GetInputPassthroughs()) {
    writer->writeStartElement(QStringLiteral("inputpassthrough"));

    // Reference to inner input
    writer->writeTextElement(QStringLiteral("node"), QString::number(reinterpret_cast<quintptr>(ip.second.node())));
    writer->writeTextElement(QStringLiteral("input"), ip.second.input());
    writer->writeTextElement(QStringLiteral("element"), QString::number(ip.second.element()));

    // ID of passthrough
    writer->writeTextElement(QStringLiteral("id"), ip.first);

    // Passthrough-specific details
    const QString &input = ip.first;
    writer->writeTextElement(QStringLiteral("name"), this->Node::GetInputName(input));

    writer->writeTextElement(QStringLiteral("flags"), QString::number((GetInputFlags(input) & ~ip.second.GetFlags()).value()));

    NodeValue::Type data_type = GetInputDataType(input);
    writer->writeTextElement(QStringLiteral("type"), NodeValue::GetDataTypeName(data_type));

    writer->writeTextElement(QStringLiteral("default"), NodeValue::ValueToString(data_type, GetDefaultValue(input), false));

    writer->writeStartElement(QStringLiteral("properties"));
    auto p = GetInputProperties(input);
    for (auto it=p.cbegin(); it!=p.cend(); it++) {
      writer->writeStartElement(QStringLiteral("property"));
      writer->writeTextElement(QStringLiteral("key"), it.key());
      writer->writeTextElement(QStringLiteral("value"), it.value().toString());
      writer->writeEndElement(); // property
    }
    writer->writeEndElement(); // properties

    writer->writeEndElement(); // input
  }

  writer->writeEndElement(); // inputpassthroughs

  writer->writeTextElement(QStringLiteral("outputpassthrough"), QString::number(reinterpret_cast<quintptr>(this->GetOutputPassthrough())));
}

void NodeGroup::PostLoadEvent(SerializedData *data)
{
  super::PostLoadEvent(data);

  foreach (const SerializedData::GroupLink &l, data->group_input_links) {
    if (Node *input_node = data->node_ptrs.value(l.input_node)) {
      NodeInput resolved(input_node, l.input_id, l.input_element);

      l.group->AddInputPassthrough(resolved, l.passthrough_id);

      l.group->SetInputFlag(l.passthrough_id, InputFlag(l.custom_flags.value()));

      if (!l.custom_name.isEmpty()) {
        l.group->SetInputName(l.passthrough_id, l.custom_name);
      }

      l.group->SetInputDataType(l.passthrough_id, l.data_type);

      l.group->SetDefaultValue(l.passthrough_id, l.default_val);

      for (auto it=l.custom_properties.cbegin(); it!=l.custom_properties.cend(); it++) {
        l.group->SetInputProperty(l.passthrough_id, it.key(), it.value());
      }
    }
  }

  for (auto it=data->group_output_links.cbegin(); it!=data->group_output_links.cend(); it++) {
    if (Node *output_node = data->node_ptrs.value(it.value())) {
      it.key()->SetOutputPassthrough(output_node);
    }
  }
}

QString NodeGroup::AddInputPassthrough(const NodeInput &input, const QString &force_id)
{
  Q_ASSERT(ContextContainsNode(input.node()));

  for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
    if (it->second == input) {
      // Already passing this input through
      return it->first;
    }
  }

  // Add input
  QString id;
  if (force_id.isEmpty()) {
    id = input.input();
    int i = 2;
    while (HasInputWithID(id)) {
      id = QStringLiteral("%1_%2").arg(input.name(), QString::number(i));
      i++;
    }
  } else {
    id = force_id;

    bool already_exists = false;
    for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
      if (it->first == id) {
        already_exists = true;
        break;
      }
    }

    Q_ASSERT(!already_exists);
  }

  AddInput(id, input.GetDataType(), input.GetDefaultValue(), input.GetFlags());

  input_passthroughs_.append({id, input});

  emit InputPassthroughAdded(this, input);

  return id;
}

void NodeGroup::RemoveInputPassthrough(const NodeInput &input)
{
  for (auto it=input_passthroughs_.begin(); it!=input_passthroughs_.end(); it++) {
    if (it->second == input) {
      RemoveInput(it->first);
      emit InputPassthroughRemoved(this, it->second);
      input_passthroughs_.erase(it);
      break;
    }
  }
}

void NodeGroup::SetOutputPassthrough(Node *node)
{
  Q_ASSERT(!node || ContextContainsNode(node));

  output_passthrough_ = node;

  emit OutputPassthroughChanged(this, output_passthrough_);
}

bool NodeGroup::ContainsInputPassthrough(const NodeInput &input) const
{
  for (auto it=input_passthroughs_.cbegin(); it!=input_passthroughs_.cend(); it++) {
    if (it->second == input) {
      return true;
    }
  }

  return false;
}

QString NodeGroup::GetInputName(const QString &id) const
{
  // If an override name was set, use that
  QString override = super::GetInputName(id);
  if (!override.isEmpty()) {
    return override;
  }

  // Call GetInputName of passed through node, which may be another group
  NodeInput pass = GetInputFromID(id);
  return pass.node()->GetInputName(pass.input());
}

NodeInput NodeGroup::ResolveInput(NodeInput input)
{
  while (GetInner(&input)) {}

  return input;
}

bool NodeGroup::GetInner(NodeInput *input)
{
  if (NodeGroup *g = dynamic_cast<NodeGroup*>(input->node())) {
    const NodeInput &passthrough = g->GetInputFromID(input->input());
    input->set_node(passthrough.node());
    input->set_input(passthrough.input());
    return true;
  } else {
    return false;
  }
}

void NodeGroupAddInputPassthrough::redo()
{
  if (!group_->ContainsInputPassthrough(input_)) {
    group_->AddInputPassthrough(input_, force_id_);
    actually_added_ = true;
  } else {
    actually_added_ = false;
  }
}

void NodeGroupAddInputPassthrough::undo()
{
  if (actually_added_) {
    group_->RemoveInputPassthrough(input_);
  }
}

void NodeGroupSetOutputPassthrough::redo()
{
  old_output_ = group_->GetOutputPassthrough();
  group_->SetOutputPassthrough(new_output_);
}

void NodeGroupSetOutputPassthrough::undo()
{
  group_->SetOutputPassthrough(old_output_);
}

}
