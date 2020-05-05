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

#include "metareader.h"

#include <QFile>

#include "common/xmlutils.h"
#include "config/config.h"
#include "node.h"

OLIVE_NAMESPACE_ENTER

NodeMetaReader::NodeMetaReader(const QString &xml_meta_filename) :
  xml_filename_(xml_meta_filename),
  iterations_(1),
  iteration_input_(nullptr)
{
  QFile metadata_file(xml_filename_);

  if (metadata_file.open(QFile::ReadOnly)) {
    QXmlStreamReader reader(&metadata_file);

    while (XMLReadNextStartElement(&reader)) {
      if (reader.name() == QStringLiteral("effect")) {
        XMLReadEffect(&reader);
      } else {
        reader.skipCurrentElement();
      }
    }

    metadata_file.close();
  } else {
    qWarning() << "Failed to load node metadata file" << xml_filename_;
  }
}

QString NodeMetaReader::Name() const
{
  return GetStringForCurrentLanguage(&names_);
}

QString NodeMetaReader::ShortName() const
{
  if (short_names_.isEmpty()) {
    return Name();
  } else {
    return GetStringForCurrentLanguage(&short_names_);
  }
}

const QString &NodeMetaReader::id() const
{
  return id_;
}

QList<Node::CategoryID> NodeMetaReader::Category() const
{
  return categories_;
}

QString NodeMetaReader::Description() const
{
  return GetStringForCurrentLanguage(&descriptions_);
}

const QString &NodeMetaReader::filename() const
{
  return xml_filename_;
}

const QString &NodeMetaReader::frag_code() const
{
  return frag_code_;
}

const QString &NodeMetaReader::vert_code() const
{
  return vert_code_;
}

const int &NodeMetaReader::iterations() const
{
  return iterations_;
}

NodeInput *NodeMetaReader::iteration_input() const
{
  return iteration_input_;
}

const QList<NodeInput *> &NodeMetaReader::inputs() const
{
  return inputs_;
}

void NodeMetaReader::Retranslate()
{
  {
    // Re-translate every parameter name
    QMap<QString, LanguageMap>::const_iterator iterator;

    // Iterate through parameter language tables that we have
    for (iterator=param_names_.begin();iterator!=param_names_.end();iterator++) {
      NodeInput* this_input = GetInputWithID(iterator.key());
      this_input->set_name(GetStringForCurrentLanguage(&iterator.value()));
    }
  }

  {
    // Re-translate any combobox items
    QMap<QString, QList<LanguageMap> >::const_iterator param_it;

    for (param_it=combo_names_.begin(); param_it!=combo_names_.end(); param_it++) {
      NodeInput* input = GetInputWithID(param_it.key());

      QStringList combo_items;

      foreach (const LanguageMap& lang_map, param_it.value()) {
        combo_items.append(GetStringForCurrentLanguage(&lang_map));
      }

      input->set_combobox_strings(combo_items);
    }
  }
}

void NodeMetaReader::XMLReadLanguageString(QXmlStreamReader* reader, LanguageMap* map)
{
  QString lang;

  // Traverse through name attributes for its language
  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("lang")) {
      lang = attr.value().toString();

      // We don't recognize any other "name" attributes at this time
      break;
    }
  }

  // Insert name with language into map
  map->insert(lang, reader->readElementText().trimmed());
}

void NodeMetaReader::XMLReadEffect(QXmlStreamReader* reader)
{
  // Traverse through effect attributes for an ID
  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("id")) {
      id_ = attr.value().toString();

      // We don't recognize any other "effect" attributes at this time
      break;
    }
  }

  if (id_.isEmpty()) {
    qWarning() << "Effect metadata" << xml_filename_ << "has no ID";
    return;
  }

  // Continue reading for other metadata
  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("name")) {
      // Pick up name
      XMLReadLanguageString(reader, &names_);
    } else if (reader->name() == QStringLiteral("shortnames")) {
      // Pick up short name
      XMLReadLanguageString(reader, &short_names_);
    } else if (reader->name() == QStringLiteral("category")) {
      // Pick up category
      QStringList category_ids = reader->readElementText().split(':');

      foreach (const QString& id, category_ids) {
        bool ok;

        int try_parse = id.toInt(&ok);

        if (!ok || try_parse < 0 || try_parse >= Node::kCategoryCount) {
          continue;
        }

        categories_.append(static_cast<Node::CategoryID>(try_parse));
      }
    } else if (reader->name() == QStringLiteral("description")) {
      // Pick up description
      XMLReadLanguageString(reader, &descriptions_);
    } else if (reader->name() == QStringLiteral("iterations")) {
      // Pick up iterations
      XMLReadIterations(reader);
    } else if (reader->name() == QStringLiteral("fragment")) {
      // Pick up fragment shader code
      XMLReadShader(reader, frag_code_);
    } else if (reader->name() == QStringLiteral("vertex")) {
      // Pick up vertex shader code
      XMLReadShader(reader, vert_code_);
    } else if (reader->name() == QStringLiteral("param")) {
      // Pick up a parameter
      XMLReadParam(reader);
    } else {
      reader->skipCurrentElement();
    }
  }
}

void NodeMetaReader::XMLReadIterations(QXmlStreamReader* reader)
{
  int iteration_pickup = reader->readElementText().toInt();

  if (iterations_ > 0) {
    iterations_ = iteration_pickup;
  } else {
    // If the iteration value is invalid, don't set it, print an error instead
    qWarning() << "Invalid iteration number in" << xml_filename_ << "- setting to default (1)";
  }
}

void NodeMetaReader::XMLReadParam(QXmlStreamReader *reader)
{
  QString param_id;
  NodeParam::DataType param_type = NodeParam::kAny;
  bool is_iterative = false;

  // Traverse through parameter attributes for an ID
  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("id")) {
      param_id = attr.value().toString();
    } else if (attr.name() == QStringLiteral("type")) {
      param_type = NodeParam::StringToDataType(attr.value().toString());
    } else if (attr.name() == QStringLiteral("iterative_input")) {
      is_iterative = true;
    }
  }

  if (param_id.isEmpty()) {
    qWarning() << "Effect metadata" << xml_filename_ << "contains a parameter with no ID - parameter was not added";
    return;
  }

  QVector<QVariant> default_val;
  QHash<QString, QVariant> properties;
  LanguageMap param_names;
  QList<LanguageMap> combo_names;
  QList<LanguageMap> combo_descriptions;

  // Traverse through param contents for more data
  while (XMLReadNextStartElement(reader)) {
    // NOTE: readElementText() returns a string, but for number types (which min and max apply to), QVariant will
    // convert them automatically
    if (reader->name() == QStringLiteral("name")) {

      // Insert language into map
      XMLReadLanguageString(reader, &param_names);

    } else if (reader->name() == QStringLiteral("default")) {

      // Reads the default value
      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("value")) {
          default_val.append(NodeInput::StringToValue(param_type, reader->readElementText()));
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("option")) {

      // Read names and descriptions
      LanguageMap names;
      LanguageMap descriptions;

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("name")) {
          XMLReadLanguageString(reader, &names);
        } else if (reader->name() == QStringLiteral("description")) {
          XMLReadLanguageString(reader, &descriptions);
        } else {
          reader->skipCurrentElement();
        }
      }

      combo_names.append(names);
      combo_descriptions.append(descriptions);

    } else {
      properties.insert(reader->name().toString(), reader->readElementText());
    }
  }

  param_names_.insert(param_id, param_names);

  // Insert combo options if they exist
  if (!combo_names.isEmpty()) {
    combo_names_.insert(param_id, combo_names);
    combo_descriptions_.insert(param_id, combo_descriptions);
  }

  NodeInput* input = new NodeInput(param_id, param_type, default_val);

  QHash<QString, QVariant>::const_iterator iterator;

  for (iterator=properties.begin();iterator!=properties.end();iterator++) {
    input->set_property(iterator.key(), iterator.value());
  }

  if (is_iterative) {
    iteration_input_ = input;
  }

  inputs_.append(input);
}

void NodeMetaReader::XMLReadShader(QXmlStreamReader *reader, QString &destination)
{
  QString code_url;

  // Traverse through parameter attributes for an ID
  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("url")) {
      code_url = attr.value().toString();

      // We don't recognize any other "shader" attributes at this time
      break;
    }
  }

  // Add code in file from URL
  if (!code_url.isEmpty()) {
    destination.append(Node::ReadFileAsString(code_url));
  }

  // Add any code that's inline in the XML
  QString element_text = reader->readElementText().trimmed();
  if (!element_text.isEmpty()) {
    destination.append(element_text);
  }
}

QString NodeMetaReader::GetStringForCurrentLanguage(const LanguageMap *language_map)
{
  if (language_map->isEmpty()) {
    // There are no entries for this map, this must be an empty string
    return QString();
  }

  // Get current language config
  QString language = Config::Current()[QStringLiteral("Language")].toString();

  // See if our map has an exact language match
  QString str_for_lang = language_map->value(language);
  if (!str_for_lang.isEmpty()) {
    return str_for_lang;
  }

  // If not, try to find a match with the same language but not the same derivation
  QString base_lang = language.split('_').first();
  QList<QString> available_languages = language_map->keys();
  foreach (const QString& l, available_languages) {
    if (l.startsWith(base_lang)) {
      // This is the same language, so we can return this
      return language_map->value(l);
    }
  }

  // We couldn't find an exact or close match, just return the first in the list
  // (assume a string in the wrong language is better than no string at all)
  return language_map->first();
}

NodeInput *NodeMetaReader::GetInputWithID(const QString &id) const
{
  foreach (NodeInput* input, inputs_) {
    if (input->id() == id) {
      return input;
    }
  }
  return nullptr;
}

OLIVE_NAMESPACE_EXIT
